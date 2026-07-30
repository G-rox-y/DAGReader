#include "controller.hpp"

// the includes below will be used for determining what is the path of the binary
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

void Controller::handleFile(tasks::controllerTask t){
    fs::path path;
    if (t == tasks::OPEN_NFD){
        spdlog::info("Task: OPEN_NFD");
        getPathNFD(path, "GFA", "gfa");
    }
    else{
        spdlog::info("Task: OPEN_PATH");
        path = channel->getControllerTaskPath();
    }
    if (path.empty()){
        spdlog::info("Recieved an empty path");
        return;
    }
    if (!fs::exists(path)){
        spdlog::warn("Recieved a path that doesnt exist: {}", path.string());
        return;
    }
    
    // save the path to the recently used paths
    spdlog::info("Updating the .ini ...");
    {   // now we need to use recent_paths data
        std::scoped_lock lk2(channel->recent_paths_mut);
        
        // remove previous occurences
        channel->recent_paths.erase(
            std::remove(channel->recent_paths.begin(), channel->recent_paths.end(), path),
            channel->recent_paths.end()
        );

        // then add our path
        channel->recent_paths.push_back(path);
        
        // update the ini file
        auto& section = ini.sections.at("recentPaths");
        
        // save 5 (or less) elements from the back
        for(int i = (int)channel->recent_paths.size() - 1; i >= 0 && (int)channel->recent_paths.size() - i <= 5; i--)
            section[std::to_string((int)channel->recent_paths.size() - i - 1)] = channel->recent_paths[i].string();
    }
    
    std::ofstream iniFileOut(iniPath, std::ios::trunc);
    if (iniFileOut.is_open()) ini.generate(iniFileOut);
    else spdlog::warn("Failed to open the ini file (for updating) at: {}", iniPath.string());
    iniFileOut.close();

    spdlog::info("Running the parser on the file");
    channel->loading_file_in_progress.store(true);
    data = std::make_shared<GFA>(path.string(), channel->minimal_memory_load.load());

    gc.clear();
    std::vector<Vertex> v;
    std::vector<Edge> e;
    data->fillData(v, e);
    gc.setGraphs(v, e);

    channel->clearSubGraphData();
    for(size_t graphGroupID = 0; graphGroupID < gc.graphs.size(); graphGroupID++){
        auto& G = gc.graphs.at(graphGroupID);
        channel->addSubgraphData(infoExchange::SubgraphData{
            G.vertices.size(), G.edges.size()
        });
    }
    
    channel->file_data = data;
    channel->graph_name_set(path.filename().string());
    channel->graph_loaded.store(true);
    channel->graph_param_change.store(true); // new things to draw now available
    channel->loading_file_in_progress.store(false);
}

void Controller::handleCSV(tasks::controllerTask t){
    fs::path path;
    if (t == tasks::OPEN_CSV_NFD){
        spdlog::info("Task: OPEN_CSV_NFD");
        getPathNFD(path, "CSV", "csv");
    }
    else{
        spdlog::info("Task: OPEN_CSV_PATH");
        path = channel->getControllerTaskPath();
    }
    if (path.empty()){
        spdlog::info("Recieved an empty path for CSV");
        return;
    }
    if (!fs::exists(path)){
        spdlog::warn("Recieved a CSV path that doesnt exist: {}", path.string());
        return;
    }

    if (!channel->graph_loaded.load()){
        spdlog::warn("Cannot load CSV: no graph is currently loaded");
        return;
    }

    spdlog::info("Loading CSV label data from: {}", path.string());
    auto csv = std::make_shared<CSV>(path.string());
    if (!csv->isValid()){
        spdlog::warn("Failed to load CSV file: {}", path.string());
        return;
    }

    auto gfa = std::dynamic_pointer_cast<GFA>(data);
    if (!gfa){
        spdlog::warn("Cannot attach CSV: current data is not a GFA file");
        return;
    }

    gfa->attachCSV(csv);
    spdlog::info("CSV labels attached successfully");
    channel->addControllerTask(tasks::REFRESH_GRAPH);
}

void Controller::handleExportCSV(tasks::controllerTask t)
{
    fs::path outPath;
    bool merging = false;

    if (t == tasks::EXPORT_CSV_NEW){
        getPathNFD(outPath, "CSV", "csv", true);
        if (outPath.empty()) return;
        if (outPath.extension() != ".csv") outPath += ".csv";
    }
    else if (t == tasks::EXPORT_CSV_OVERWRITE){
        auto gfa = std::dynamic_pointer_cast<GFA>(data);
        if (!gfa || !gfa->hasAttachedCSV()){
            spdlog::warn("Cannot export: no CSV is currently attached");
            return;
        }
        outPath = gfa->getAttachedCSV()->getPath();
        merging = true;
    }

    // ---- collect name → hex color for every segment that has a custom color ----
    std::vector<std::pair<std::string, std::string>> rows;
    {
        std::lock_guard<std::mutex> lk(channel->color_storage_mut);
        for (const auto& [eid, packedColor] : channel->segment_color_map){
            glm::u8vec4 c = std::bit_cast<glm::u8vec4>(packedColor);
            char hex[16];
            std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", c.a, c.r, c.g, c.b);

            auto prop = data->retrieveEdgeData(eid, false);
            auto* type = std::get_if<char>(&prop["Type_C"]);
            if (!type || *type != GFA::mapType::SEGMENT) [[unlikely]] continue;

            auto* nameSv = std::get_if<std::string_view>(&prop["Name_SW"]);
            if (!nameSv) continue;

            rows.emplace_back(std::string(*nameSv), std::string(hex));
        }
    }

    if (rows.empty()){
        spdlog::info("No custom segment colors to export");
        return;
    }

    // ---- write / merge ----
    if (merging){
        try {
            // detect delimiter from the first line (same logic as CSV.cpp)
            char sep = ',';
            {
                std::ifstream sniff(outPath);
                if (sniff.is_open()){
                    std::string firstLine;
                    if (std::getline(sniff, firstLine)){
                        size_t commas = std::count(firstLine.begin(), firstLine.end(), ',');
                        size_t tabs   = std::count(firstLine.begin(), firstLine.end(), '\t');
                        size_t semis  = std::count(firstLine.begin(), firstLine.end(), ';');
                        if (tabs > commas && tabs > semis) sep = '\t';
                        else if (semis > commas && semis > tabs) sep = ';';
                    }
                }
            }

            rapidcsv::Document doc(
                outPath.string(),
                rapidcsv::LabelParams(0, -1),
                rapidcsv::SeparatorParams(sep, true, true)
            );

            auto headers = doc.GetColumnNames();
            size_t colorCol = std::string::npos;
            for (size_t i = 0; i < headers.size(); ++i){
                std::string lower = headers[i];
                std::transform(lower.begin(), lower.end(), lower.begin(),
                               [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });
                if (lower == "colour" || lower == "color"){
                    colorCol = i;
                    break;
                }
            }
            if (colorCol == std::string::npos){
                colorCol = headers.size();
                doc.SetColumnName(colorCol, "Color");
            }

            for (const auto& [name, color] : rows) {
                bool found = false;
                for (size_t r = 0; r < doc.GetRowCount(); ++r) {
                    if (doc.GetCell<std::string>(0, r) == name) {
                        doc.SetCell<std::string>(colorCol, r, color);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::vector<std::string> newRow(doc.GetColumnCount(), "");
                    newRow[0]         = name;
                    newRow[colorCol]  = color;
                    doc.InsertRow(doc.GetRowCount(), newRow);
                }
            }

            doc.Save();
            spdlog::info("Custom colors merged into existing CSV: {}", outPath.string());
        }
        catch (const std::exception& e){
            spdlog::error("Failed to overwrite CSV '{}': {}", outPath.string(), e.what());
        }
    }
    else {
        std::ofstream out(outPath, std::ios::trunc);
        if (!out.is_open()){
            spdlog::error("Failed to open '{}' for writing", outPath.string());
            return;
        }
        out << "Name,Color\n";
        for (const auto& [name, color] : rows) out << name << "," << color << "\n";
        spdlog::info("Custom colors exported to new CSV: {}", outPath.string());
    }
}

void Controller::getPathNFD(std::filesystem::path& path, const char* filterName, const char* filterExt, bool save) const
{
    // TODO: nfd init can throw an error, you should catch it
    NFD_Init();

    nfdu8char_t *outPath;
    nfdu8filteritem_t filters[1] = { { filterName, filterExt } };
    nfdresult_t result;
    if (save){
        nfdsavedialogu8args_t args = {0};
        args.filterList = filters;
        args.filterCount = 1;
        args.defaultName = "custom_colors.csv";
        result = NFD_SaveDialogU8_With(&outPath, &args);
    }
    else{
        nfdopendialogu8args_t args = {0};
        args.filterList = filters;
        args.filterCount = 1;
        result = NFD_OpenDialogU8_With(&outPath, &args);
    }
    if (result == NFD_OKAY){
        if (save) spdlog::info("NFD save path fetched: {}", outPath);
        else spdlog::info("NFD path fetched: {}", outPath);
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
        spdlog::info("NFD cancelled");
    else 
        spdlog::error("NFD Error: {}", NFD_GetError());

    NFD_Quit();
}

void Controller::layoutGraph(){
    spdlog::info("Task: LAYOUT_GRAPH");
    if (!gc.empty()){
        channel->layout_in_progress.store(true);

        // Normalize segment lengths
        // TODO: make an actually functional segment length system
        long long int maxSize = std::numeric_limits<long long int>::min();
        for (auto& G:gc.graphs)
            for(auto& e:G.edges)
                if (maxSize < e.originalLength) maxSize = e.originalLength;
        long long int segLen = maxSize / 20;

        // adjust the segment lengths
        for (auto& G:gc.graphs)
            for(auto& e:G.edges)
                e.length = (e.originalLength / segLen) + 1;

        // layout the graphs, multithreading ahead!
        #ifdef HAS_OPENMP
            unsigned int available_threads = std::max<unsigned int>(std::thread::hardware_concurrency()-2 , 2);
            spdlog::info("splitting the workload, max threads: {}", available_threads);
            #pragma omp parallel for num_threads(available_threads) schedule(dynamic, 1)
        #endif
        for(size_t i = 0; i < gc.graphs.size(); i++){
            GRIP layout(gc.graphs.at(i));
            layout.setFRscaling(channel->grip_scalingFactor.load());
            layout.setRoundsNumber(channel->grip_roundsNum.load());
            layout.setTempGain(channel->grip_tempGain.load());
            layout.setTempNarrowGainInc(channel->grip_tempNarrowGain.load());
            layout.run();
        }

        channel->layout_in_progress.store(false); // layout phase done
        channel->graph_param_change.store(false); // update has been applied, bool false now
    }
    else spdlog::warn("No graph found!");
}

void Controller::resetGraph(){
    spdlog::info("Task: RESET_GRAPH");
    if (gc.empty()){
        spdlog::warn("No graph found!");
        return;
    }

    // place the graphs on correct places
    double totalRadius = 0.0;
    std::vector<std::pair<double, int>> graphRadii; // pairs of nodsu graph id and its radius
    for(size_t graphGroupID = 0; graphGroupID < gc.graphs.size(); graphGroupID++){
        if (channel->isGroupHidden(graphGroupID)) continue; // if the graph is hidden just skip it
        auto& G = gc.graphs.at(graphGroupID);

        // recenter the graph
        glm::dvec3 bc = G.calculateBarycenter();
        G.translate(-bc);

        // remember its radius
        double rad = G.radius;
        totalRadius += rad;
        graphRadii.emplace_back(std::make_pair(rad, graphGroupID));
    }
    
    if (!graphRadii.empty()){
        sort(graphRadii.begin(), graphRadii.end());
        
        // now allign then all
        double sqdim = std::max<double>(std::sqrt(totalRadius * 2.0), graphRadii.back().first * 2.0);
        double ypos = 0.0, xpos = 0.0;
        for(auto& [radius, GraphID]:graphRadii){      
            if (xpos + radius > sqdim && xpos != 0.0){
                xpos = 0.0;
                ypos += radius;
            }
            gc.graphs.at(GraphID).translate(glm::dvec3(xpos + radius, ypos + radius, 0.0));
            xpos += 2.0 * radius;

            channel->setSubGraphPosition(GraphID, gc.graphs.at(GraphID).calculateBarycenter());
            channel->setSubGraphRadius(GraphID, radius);
        }
    }

    // render the graphs
    if (!channel->renderer) [[unlikely]] {
        spdlog::warn("Shared datastructure pointer is not defined");
        return;
    }

    channel->renderer->clearAll();

    // save these variables for use in the loop
    auto segColor = channel->segment_color_packed.load();
    auto linkColor = channel->link_color_packed.load();
    auto errColor = channel->err_color.load();
    auto segWidth = glm::vec2(channel->segment_widths.load());
    auto linkWidth = glm::vec2(channel->link_widths.load());
    for(size_t graphGroupID = 0; graphGroupID < gc.graphs.size(); graphGroupID++){
        if (channel->isGroupHidden(graphGroupID)) continue;

        auto& G = gc.graphs.at(graphGroupID);

        std::vector<BezierBox> boxes;
        std::vector<bool> boxSelector; // if true at [id] then boxes[id] is selected ; first we will set true for all segments
        std::unordered_map<size_t, size_t> boxEdgeIndices; // first: edge id, second: boxes id
        std::unordered_map<size_t, std::set<size_t>> vertexTouchpoints; // first: vertex id, second: set of edges that have it

        // first step: segbox layout (only positions)
        for(const auto& e:G.edges){
            vertexTouchpoints[e.start].insert(e.lid);
            vertexTouchpoints[e.end].insert(e.lid);
            boxEdgeIndices[e.lid] = boxes.size();
            if (e.segPart){
                boxSelector.push_back(true);
                auto S = G.vertices.at(e.start).pos, E = G.vertices.at(e.end).pos;
                boxes.emplace_back(BezierBox{
                    S, E, 
                    glm::normalize(S-E), glm::normalize(E-S), // we will fill orientations later
                    segWidth, errColor, e.gid
                });
            }
            else {
                boxSelector.push_back(false);
                auto S = G.vertices.at(e.start).pos, E = G.vertices.at(e.end).pos;
                boxes.emplace_back(BezierBox{
                    S, E,
                    glm::normalize(S-E), glm::normalize(E-S), // we will fill orientations later
                    linkWidth, errColor, e.gid
                });
            }
        }

        // next step: adjust orientations
        for(auto [v, touchPointVector]:vertexTouchpoints){
            // first extract these variables
            glm::dvec3 linkCenter(0.0, 0.0, 0.0);
            std::vector<size_t> segInds;
            size_t linkNum = 0;
            for(auto el:touchPointVector){
                const auto& E = G.edges.at(el);
                if (E.segPart) segInds.push_back(el);
                else{
                    linkNum++;
                    if (v != E.start) linkCenter += G.vertices.at(E.start).pos;
                    else linkCenter += G.vertices.at(E.end).pos;
                }
            }
            size_t segNum = segInds.size();

            // then do checking if touchpoint is valid and modify the orientations
            if (segNum == 2 && linkNum > 0)
                spdlog::warn("Link(s) connected to two segments at the segbox connection! Skipping orientation adjusting");
            else if (segNum == 0)
                spdlog::warn("No segments at the segbox connection! Skipping orientation adjusting");
            else if (segNum > 2)
                spdlog::warn("More than two segments at the segbox connection! Skipping orientation adjusting");
            else {
                // if this is a segment - link(s) connection
                if (linkNum > 0){
                    // calculate the orientation
                    linkCenter /= linkNum;
                    glm::dvec3 ori = glm::normalize(G.vertices.at(v).pos - linkCenter);

                    // apply it to the segment box
                    auto& TheSegBox = boxes.at(boxEdgeIndices.at(segInds[0]));
                    TheSegBox.color = segColor;
                    if (v == G.edges.at(segInds[0]).start) TheSegBox.startOri = ori;
                    else TheSegBox.endOri = ori;

                    // apply it to link boxes
                    for(auto el:touchPointVector){
                        const auto& E = G.edges.at(el);
                        if (E.segPart) continue;
                        auto& TheLinkBox = boxes.at(boxEdgeIndices.at(el));
                        TheLinkBox.color = linkColor;
                        if (v == E.start) TheLinkBox.startOri = -ori;
                        else TheLinkBox.endOri = -ori;
                    }
                }
                // if this is a segment - segment connection
                else if (segNum == 2){
                    // calculate the orientation
                    size_t Seg1FarInd, Seg2FarInd;
                    if (G.edges.at(segInds[0]).start == v) Seg1FarInd = G.edges.at(segInds[0]).end;
                    else Seg1FarInd = G.edges.at(segInds[0]).start;
                    if (G.edges.at(segInds[1]).start == v) Seg2FarInd = G.edges.at(segInds[1]).end;
                    else Seg2FarInd = G.edges.at(segInds[1]).start;
                    glm::dvec3 ori = glm::normalize(G.vertices.at(Seg1FarInd).pos - G.vertices.at(Seg2FarInd).pos);

                    // apply to segbox 1
                    auto& TheSegBox1 = boxes.at(boxEdgeIndices.at(segInds[0]));
                    TheSegBox1.color = segColor;
                    if (v == G.edges.at(segInds[0]).start) TheSegBox1.startOri = ori;
                    else TheSegBox1.endOri = ori;

                    // apply to segbox 2
                    auto& TheSegBox2 = boxes.at(boxEdgeIndices.at(segInds[1]));
                    TheSegBox2.color = segColor;
                    if (v == G.edges.at(segInds[1]).start) TheSegBox2.startOri = ori;
                    else TheSegBox2.endOri = ori;
                }
                // if this is a Segment end with no connections
                else{
                    // calculate the orientation
                    size_t SegFarInd;
                    if (G.edges.at(segInds[0]).start == v) SegFarInd = G.edges.at(segInds[0]).end;
                    else SegFarInd = G.edges.at(segInds[0]).start;
                    glm::dvec3 ori = glm::normalize(G.vertices.at(SegFarInd).pos - G.vertices.at(v).pos);

                    // apply to segbox
                    auto& TheSegBox = boxes.at(boxEdgeIndices.at(segInds[0]));
                    TheSegBox.color = segColor;
                    if (v == G.edges.at(segInds[0]).start) TheSegBox.startOri = ori;
                    else TheSegBox.endOri = ori;
                }
            }
        }

        // and write the boxes to buffer
        channel->renderer->activateGroup(graphGroupID);
        
        channel->renderer->activateGroup(groups::SEGMENT);
        channel->renderer->addBoxes(boxes, boxSelector);
        channel->renderer->deactivateGroup(groups::SEGMENT);

        for(size_t i = 0; i < boxSelector.size(); i++) boxSelector[i] = !boxSelector[i]; // invert selection to select links

        channel->renderer->activateGroup(groups::LINK);
        channel->renderer->addBoxes(boxes, boxSelector);
        channel->renderer->deactivateGroup(groups::LINK);
        
        channel->renderer->deactivateGroup(graphGroupID);
    }

    // camera framing
    auto vertices = std::views::iota(size_t{0}, gc.graphs.size())
        | std::views::filter([&](size_t i){ return !channel->isGroupHidden(i); })
        | std::views::transform([&](size_t i) -> const auto& { return gc.graphs[i].vertices; })
        | std::views::join;

    auto first = vertices.begin();
    if (first == vertices.end()) return;

    auto [xMin, xMax] = std::ranges::minmax_element(vertices, {}, [](const auto& v){ return v.pos.x; });
    auto [yMin, yMax] = std::ranges::minmax_element(vertices, {}, [](const auto& v){ return v.pos.y; });

    double cx  = (xMin->pos.x + xMax->pos.x) * 0.5;
    double cy  = (yMin->pos.y + yMax->pos.y) * 0.5;
    double ext = std::max(xMax->pos.x - xMin->pos.x, yMax->pos.y - yMin->pos.y) * 0.5;

    float camZ = static_cast<float>(ext * 1.25 / std::tan(channel->cam->getFOV() * 0.5f));

    channel->cam->setDefPos(glm::vec3(static_cast<float>(cx), static_cast<float>(cy), camZ));
    channel->cam->setDefScale(channel->cam->getFarCP() / camZ / 150.f);
    channel->cam->resetView();
}

void Controller::refreshGraph(){
    spdlog::info("Task: REFRESH_GRAPH");
    
    static float prevSegWidth = 0.f, prevLinkWidth = 0.f;

    glm::u8vec4 segColor = channel->segment_color_packed.load();
    glm::u8vec4 linkColor = channel->link_color_packed.load();
    glm::u8vec4 selectionColor = channel->selected_color_packed.load();

    float segWidth = channel->segment_widths.load(), linkWidth = channel->link_widths.load();

    auto segScheme = channel->segment_color_scheme.load();
    auto segRule = channel->segment_color_rule.load();
    auto linkScheme = channel->link_color_scheme.load();
    

    // generic scalar coloring: fetch a double per edge, apply segRule/segColor gradient
    auto colorByScalar = [&](const std::vector<size_t>& eids, auto&& fetchScalar) {
        std::vector<glm::u8vec4> colorVec(eids.size());
        std::vector<double> values(eids.size(), 0.0);
        std::vector<double> sortedValues;
        
        double maxValue = 0.0;
        for (size_t i = 0; i < eids.size(); ++i) {
            if (auto v = fetchScalar(eids[i])) values[i] = *v;
            maxValue = std::max(maxValue, values[i]);
        }

        if (segRule == infoExchange::colRule::PROGRESSIVE) {
            sortedValues = values;
            std::sort(sortedValues.begin(), sortedValues.end());
        }

        if (maxValue != 0.0) {
            for (size_t i = 0; i < values.size(); ++i) {
                double ratio = 1.0;
                if (segRule == infoExchange::colRule::NORMAL) ratio = values[i] / maxValue;
                else if (segRule == infoExchange::colRule::SQRT) ratio = std::sqrt(values[i] / maxValue);
                else if (segRule == infoExchange::colRule::CBRT) ratio = std::cbrt(values[i] / maxValue);
                else if (segRule == infoExchange::colRule::PROGRESSIVE) {
                    size_t index = sortedValues.size() - 1;
                    for (size_t b = sortedValues.size() / 2;
                         b > 0 && sortedValues[index] != values[i]; b /= 2)
                        while (index > b && sortedValues[index - b] >= values[i])
                            index -= b;
                    ratio = static_cast<double>(index) /
                            static_cast<double>(sortedValues.size() - 1);
                }
                int scale = std::min(static_cast<int>(ratio * 255 * 2), 255 * 2);
                if (scale > 255) colorVec[i] = glm::u8vec4(scale - 255, 255 * 2 - scale, 0, segColor.a);
                else colorVec[i] = glm::u8vec4(0, scale, 255 - scale, segColor.a);
            }
        }
        channel->renderer->colorBulkByVector(eids, colorVec);
    };

    // segment updates
    channel->renderer->activateGroup(groups::SEGMENT);

    if (segScheme == infoExchange::colScheme::NONE)
        channel->renderer->changeGroupColors(segColor);
    else if (segScheme == infoExchange::colScheme::RANDOM)
        channel->renderer->randomizeGroupColors();
    else if (segScheme == infoExchange::colScheme::DEPTH) {
        auto eids = channel->renderer->getGroupIDs(groups::SEGMENT);
        colorByScalar(eids, [&](size_t eid) -> std::optional<double> {
            auto d = data->retrieveEdgeData(eid, false);
            if (auto* depth = std::get_if<double>(&d["Depth_D"])) return *depth;
            return std::nullopt;
        });
    }
    else if (segScheme == infoExchange::colScheme::LENGTH) {
        auto eids = channel->renderer->getGroupIDs(groups::SEGMENT);
        colorByScalar(eids, [&](size_t eid) -> std::optional<double> {
            auto d = data->retrieveEdgeData(eid, false);
            if (auto* length = std::get_if<long long int>(&d["Length_LLI"]))
                return static_cast<double>(*length);
            return std::nullopt;
        });
    }
    else if (segScheme == infoExchange::colScheme::CSV) {
        channel->renderer->changeGroupColors(segColor);
        auto gfa = dynamic_cast<GFA*>(data.get());
        if (gfa && gfa->hasAttachedCSV()) {
            const CSV* csv = gfa->getAttachedCSV();
            std::vector<size_t> eids;
            std::vector<glm::u8vec4> colors;
            eids.reserve(csv->getNodeNames().size());
            colors.reserve(csv->getNodeNames().size());

            for (const auto& name : csv->getNodeNames()) {
                // Lookup the segment by name and fetch its renderer eid.
                auto results = gfa->searchStrictForName(name, GFA::mapType::SEGMENT);
                for (const auto& [oid, type, eidOpt] : results) {
                    if (!eidOpt.has_value()) continue;

                    if (auto col = csv->getNodeColorParsed(name)) {
                        eids.push_back(*eidOpt);
                        colors.push_back(*col);
                    }
                }
            }

            if (!eids.empty()) channel->renderer->colorBulkByVector(eids, colors);
        }
    }

    if (channel->show_custom_colors.load()){
        std::vector<size_t> eids;
        std::vector<glm::u8vec4> colors;
        {
            std::lock_guard<std::mutex> lk(channel->color_storage_mut);
            for(auto& [eid,packedColor]:channel->segment_color_map){
                glm::u8vec4 col = std::bit_cast<glm::u8vec4>(packedColor);
                eids.emplace_back(eid);
                colors.emplace_back(col);
            }
        }
        if (!eids.empty()) channel->renderer->colorBulkByVector(eids, colors);
    }

    if (segWidth != prevSegWidth)
        channel->renderer->changeGroupDims(glm::vec2(channel->segment_widths.load()));
    
    prevSegWidth = segWidth;
    
    channel->renderer->deactivateGroup(groups::SEGMENT);

    // link updates
    channel->renderer->activateGroup(groups::LINK);

    if (linkScheme == infoExchange::colScheme::NONE)
        channel->renderer->changeGroupColors(linkColor);
    else if (linkScheme == infoExchange::colScheme::RANDOM)
        channel->renderer->randomizeGroupColors();
    
    if (linkWidth != prevLinkWidth)
        channel->renderer->changeGroupDims(glm::vec2(channel->link_widths.load()));
    
    prevLinkWidth = linkWidth;
    
    channel->renderer->deactivateGroup(groups::LINK);

    // refresh selected groups
    channel->renderer->activateGroup(groups::SELECTION);
    channel->renderer->changeGroupColors(selectionColor);
    channel->renderer->deactivateGroup(groups::SELECTION);
}

Controller::Controller(infoExchange* c) : channel(c)
{   // Get the path of the DAGReader executable
#if defined(_WIN32)
    std::string buffer(MAX_PATH, '\0');
    DWORD size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0){
        spdlog::error("Controller error: GerModuleFileName failed");
        throw std::system_error(GetLastError(), std::system_category(), "GetModuleFileName failed");
    }
    buffer.resize(size);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size); // Get required size
    std::string buffer(size, '\0');
    _NSGetExecutablePath(buffer.data(), &size);
#else // Linux
    std::string buffer(PATH_MAX, '\0');
    ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (size == -1){
        spdlog::error("Controller error: readlink failed");
        throw std::system_error(errno, std::system_category(), "readlink failed");
    }
    buffer.resize(size);
#endif
    m_binaryPath = buffer;
    spdlog::info("Executable ran from: {}", m_binaryPath.string());

    iniPath = m_binaryPath.parent_path() / "DAGReader.ini";
    
    // check if .ini file exists, if not, create it
    if (!fs::exists(iniPath)){
        std::ofstream create(iniPath, std::ios::out);
        create.close();
    }
    
    // Now we load the .ini file
    spdlog::info("Loading the .ini file from {}", iniPath.string());
	std::ifstream iniFile(iniPath);
    if (!iniFile.is_open()){
        spdlog::error("Controller error: failed to open the .ini file");
        throw std::runtime_error("failed to open the .ini file");
    }

	ini.parse(iniFile);
    iniFile.close();

    // if the recentPaths section doesnt exist, create it
    if (ini.sections.find("recentPaths") == ini.sections.end())
        ini.sections["recentPaths"];

    auto& section = ini.sections.at("recentPaths");
    {   // now lets load those paths in
        std::scoped_lock lk(channel->recent_paths_mut);
        for(int i = 4; i >= 0; i--){ // there should be max 5 entries here
            if (section.find(std::to_string(i)) == section.end()) // if there is less, skip
                continue;
            channel->recent_paths.emplace_back(section.at(std::to_string(i)));
            // the entries are loaded backwards, this is so that upon adding a path (with push_back)
            // the paths get a new order, and only the last 5 paths get saved so new ones get saved as first (most recent)
        }
    }

    // update the ini file and close
    std::ofstream iniFileOut(iniPath, std::ios::trunc);
    if (iniFileOut.is_open()) ini.generate(iniFileOut);
    else spdlog::warn("Failed to open the ini file (for updating) at: {}", iniPath.string());
    iniFileOut.close();
}

void Controller::run()
{
    bool shouldExit = false;
    while(!shouldExit){
        spdlog::debug("Controller waiting for a task...");
        tasks::controllerTask t = channel->waitForTask();
        spdlog::debug("Controller recieved a task");

        if (t == tasks::OPEN_NFD || t == tasks::OPEN_PATH)
            handleFile(t);
        else if (t == tasks::OPEN_CSV_NFD || t == tasks::OPEN_CSV_PATH)
            handleCSV(t);
        else if (t == tasks::UNLOAD_CSV) {
            spdlog::info("Task: UNLOAD_CSV");
            auto gfa = std::dynamic_pointer_cast<GFA>(data);
            if (gfa && gfa->hasAttachedCSV()) {
                gfa->detachCSV();
                spdlog::info("CSV labels detached");
            }
        }
        else if (t == tasks::EXPORT_CSV_NEW || t == tasks::EXPORT_CSV_OVERWRITE)
            handleExportCSV(t);
        else if (t == tasks::LAYOUT_GRAPH)
            layoutGraph();
        else if (t == tasks::RESET_GRAPH)
            resetGraph();
        else if (t == tasks::REFRESH_GRAPH)
            refreshGraph();
        else if (t == tasks::EXIT){
            spdlog::info("Task: EXIT");
            shouldExit = true;
        } // exit shouldnt be checked like this but by some atomic bool in infochannel that shuts down the while

        spdlog::info("TASK DONE");
    }
}
