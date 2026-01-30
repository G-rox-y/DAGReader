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
        getPathNFD(path);
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
    GFA gfa(path.string());

    gc.clear();
    std::vector<Vertex> v;
    std::vector<Edge> e;
    gfa.fillData(v, e);
    gc.setGraphs(v, e);

    channel->clearSubGraphData();
    for(size_t graphGroupID = 0; graphGroupID < gc.graphs.size(); graphGroupID++){
        auto& G = gc.graphs.at(graphGroupID);
        channel->addSubgraphData(infoExchange::SubgraphData{
            G.vertices.size(), G.edges.size()
        });
    }
    channel->graph_data_seg_num.store(gfa.segmentNum());
    channel->graph_data_link_num.store(gfa.linkNum());
    channel->graph_name_set(path.filename().string());
    channel->graph_loaded.store(true);
    channel->graph_param_change.store(true); // new things to draw now available
    channel->loading_file_in_progress.store(false);
}

void Controller::getPathNFD(std::filesystem::path& path) const
{
    // TODO: nfd init can throw an error, you should catch it
    NFD_Init();

    nfdu8char_t *outPath;
    nfdu8filteritem_t filters[1] = { { "GFA", "gfa" } };
    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = 1;
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY){
        spdlog::info("NFD path fetched: {}", outPath);
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

    struct BezierBoxMetadata{
        // barycenter of connection endpoints to calculate orientation
        glm::dvec3 startBc, endBc;
        int startBcCounter, endBcCounter;

        BezierBoxMetadata() 
        : startBc(0.0, 0.0, 0.0), endBc(0.0, 0.0, 0.0), startBcCounter(0), endBcCounter(0) {}
    };

    for(size_t graphGroupID = 0; graphGroupID < gc.graphs.size(); graphGroupID++){
        if (channel->isGroupHidden(graphGroupID)) continue;

        auto& G = gc.graphs.at(graphGroupID);
        std::vector<BezierBox> segBoxes;
        std::vector<BezierBoxMetadata> metadata;
        std::unordered_map<int, size_t> boxIndices;

        for(auto& e:G.edges){
            if (!e.segPart) continue;
            
            segBoxes.emplace_back();
            metadata.emplace_back();
            BezierBox& b = segBoxes.back();
            boxIndices[e.start] = segBoxes.size() - 1;
            boxIndices[e.end] = segBoxes.size() - 1;
            b.start = G.vertices.at(e.start).pos;
            b.end = G.vertices.at(e.end).pos;
            b.startOri = glm::normalize(b.start - b.end);
            b.endOri = -b.startOri;
        }
        for(auto& e:G.edges){
            if (e.segPart) continue;

            BezierBoxMetadata& bFirst = metadata.at(boxIndices[e.start]);
            BezierBoxMetadata& bSecond = metadata.at(boxIndices[e.end]);

            // set barycenter of edge start box and end box
            if (e.startOri){
                bFirst.startBcCounter++;
                bFirst.startBc += G.vertices.at(e.end).pos;
            }
            else{
                bFirst.endBcCounter++;
                bFirst.endBc += G.vertices.at(e.end).pos;
            }
            if (e.endOri){
                bSecond.startBcCounter++;
                bSecond.startBc += G.vertices.at(e.start).pos;
            }
            else{
                bSecond.endBcCounter++;
                bSecond.endBc += G.vertices.at(e.start).pos;
            }
        }
        
        // calculate the orientation out of baryceters
        for(size_t i = 0; i < segBoxes.size(); i++){
            auto& md = metadata.at(i);
            auto& b = segBoxes.at(i);
            if (md.startBcCounter == 0 && md.endBcCounter == 0) continue;

            if (md.startBcCounter != 0){
                md.startBc /= static_cast<double>(md.startBcCounter);
                glm::dvec3 l_startOri = b.start - md.startBc;
                if (glm::length(l_startOri) > 1e-3) b.startOri = glm::normalize(l_startOri);
            }
            if (md.endBcCounter != 0){
                md.endBc /= static_cast<double>(md.endBcCounter);
                glm::dvec3 l_endOri = b.end - md.endBc;
                if (glm::length(l_endOri) > 1e-3) b.endOri = glm::normalize(l_endOri);
            }
        }

        // create boxes for links
        std::vector<BezierBox> linkBoxes;
        for(auto& e:G.edges)
            if (!e.segPart)
                linkBoxes.emplace_back(BezierBox{
                    G.vertices.at(e.start).pos,
                    G.vertices.at(e.end).pos,
                    (e.startOri) ? -segBoxes.at(boxIndices[e.start]).startOri : -segBoxes.at(boxIndices[e.start]).endOri,
                    (e.endOri) ? segBoxes.at(boxIndices[e.end]).endOri : segBoxes.at(boxIndices[e.end]).startOri
                });

        // and write the boxes to buffer
        channel->renderer->activateGroup(graphGroupID);
        
        channel->renderer->activateGroup(groups::SEGMENT);
        channel->renderer->addBoxes(segBoxes);
        channel->renderer->deactivateGroup(groups::SEGMENT);

        channel->renderer->activateGroup(groups::LINK);
        channel->renderer->addBoxes(linkBoxes);
        channel->renderer->deactivateGroup(groups::LINK);
        
        channel->renderer->deactivateGroup(graphGroupID);
    }

    // set the colors and sizes for the links and segments
    channel->renderer->activateGroup(groups::SEGMENT);
    channel->renderer->changeGroupColors(channel->segment_color_packed.load());
    channel->renderer->changeGroupDims(glm::vec2(channel->segment_widths.load()));
    channel->renderer->deactivateGroup(groups::SEGMENT);

    channel->renderer->activateGroup(groups::LINK);
    channel->renderer->changeGroupColors(channel->link_color_packed.load());
    channel->renderer->changeGroupDims(glm::vec2(channel->link_widths.load()));
    channel->renderer->deactivateGroup(groups::LINK);

    // now just set the camera to look at the right place
    double maxX = 0.0, maxY = 0.0;
    for(size_t graphGroupID = 0; graphGroupID < gc.graphs.size(); graphGroupID++){
        if (channel->isGroupHidden(graphGroupID)) continue;

        auto& G = gc.graphs.at(graphGroupID);
        for(auto& v:G.vertices){
            maxX = std::max<double>(maxX, std::abs(v.pos.x));
            maxY = std::max<double>(maxY, std::abs(v.pos.y));
        }
    }
    float halfFov = channel->cam->getFOV() / 2.f;
    float camZ = glm::sin(glm::radians(90.f) - halfFov) * maxX / 2.f / std::sin(halfFov);
    float scale = channel->cam->getFarCP() / camZ / 150.f;
    channel->cam->setDefPos(glm::vec3(maxX/2.f, maxX/2.f, camZ * 1.5f));
    channel->cam->setDefScale(scale);
    channel->cam->resetView();
}

void Controller::refreshGraph(){
    spdlog::info("Task: REFRESH_GRAPH");
    // memory
    static glm::u8vec4 prevSegColor, prevLinkColor;
    static float prevSegWidth = 0.f, prevLinkWidth = 0.f;
    static bool prevSegRandom = false, prevLinkRandom = false;

    glm::u8vec4 segColor = channel->segment_color_packed.load();
    glm::u8vec4 linkColor = channel->link_color_packed.load();
    glm::u8vec4 selectionColor = channel->selected_color_packed.load();
    float segWidth = channel->segment_widths.load(), linkWidth = channel->link_widths.load();
    bool segRandom = channel->randomize_segment_colors.load();
    bool linkRandom = channel->randomize_link_colors.load();
    
    // segment updates
    channel->renderer->activateGroup(groups::SEGMENT);

    channel->renderer->changeGroupColors(segColor);
    if (segRandom && segRandom != prevSegRandom)
        channel->renderer->randomizeGroupColors();

    if (segWidth != prevSegWidth)
        channel->renderer->changeGroupDims(glm::vec2(channel->segment_widths.load()));
    
    prevSegRandom = segRandom;
    prevSegColor = segColor;
    prevSegWidth = segWidth;
    
    channel->renderer->deactivateGroup(groups::SEGMENT);

    // link updates
    channel->renderer->activateGroup(groups::LINK);

    channel->renderer->changeGroupColors(linkColor);
    if (linkRandom && linkRandom != prevLinkRandom)
        channel->renderer->randomizeGroupColors();
    
    if (linkWidth != prevLinkWidth)
        channel->renderer->changeGroupDims(glm::vec2(channel->link_widths.load()));
    
    prevLinkRandom = linkRandom;
    prevLinkWidth = linkWidth;
    prevLinkColor = linkColor;
    
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
