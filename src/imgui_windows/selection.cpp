#include "selection.hpp"

void selectionWindow::draw() {
    // selection data that will be written
    static std::vector<size_t> indices;

    // since a fetch can be expensive, we will do it most once every half second
    static auto start = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    bool change = false;

    if (std::chrono::milliseconds(delta).count() > 500){
        auto newIndices = channel->renderer->getGroupIDs(groups::SELECTION); // fetch selection data
        if (newIndices != indices){
            change = true;
            indices = newIndices;
        }
        start = std::chrono::steady_clock::now(); //reset timer
    }

    if (indices.empty() || !channel->selection_window_allowed.load()) return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport(); // get the viewport

    // Helper for "Label: value" pattern with string_view
    auto LabeledSV = [](const char* label, std::string_view sv) {
        ImGui::Text("%s", label);
        ImGui::SameLine();
        ImGui::TextUnformatted(sv.data(), sv.data() + sv.size());
    };

    // force the panel to be in the top left corner
    float margin = 6.f;
    ImVec2 ll(viewport->WorkPos.x + margin, viewport->WorkPos.y + margin);
    ImGui::SetNextWindowPos(ll, ImGuiCond_Always);

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;;

    if (ImGui::Begin("Selection HUD", NULL, flags)){
        if (indices.size() == 1){
            size_t idx = indices[0];
            auto props = channel->file_data->retrieveEdgeData(idx, m_verboseMode);
            if (!props.empty()) {
                auto* type = std::get_if<char>(&props["Type_C"]);
                if (*type == GFA::mapType::SEGMENT) {
                    if (auto* name = std::get_if<std::string_view>(&props["Name_SW"]))
                        LabeledSV("Segment:", *name);
                    if (auto* length = std::get_if<long long>(&props["Length_LLI"]))
                        ImGui::Text("Length: %lld bp", *length);
                    if (auto* depth = std::get_if<double>(&props["Depth_D"]))
                        ImGui::Text("Depth: %.2f", *depth);
                    if (m_verboseMode){
                        if (auto* kc = std::get_if<long long>(&props["KmerCount_LLI"]))
                            ImGui::Text("K-mer count: %lld", *kc);
                        if (auto* rc = std::get_if<long long>(&props["ReadCount_LLI"]))
                            ImGui::Text("Read count: %lld", *rc);
                        if (auto* fc = std::get_if<long long>(&props["FragmentCount_LLI"]))
                            ImGui::Text("Fragment count: %lld", *fc);
                        if (auto* seq = std::get_if<bool>(&props["SequenceAvailable_B"])){
                            if (*seq){
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Sequence exists >");
                                ImGui::SameLine();
                                if (ImGui::Button("View")) {
                                    if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get())) {
                                        if (auto result = gfa->retrieveSequence(idx)) {
                                            auto& [path, pos] = *result;
                                            seqWin.setSequence(path, pos);
                                        }
                                    } else {
                                        spdlog::warn("Filetype not implemented yet");
                                    }
                                }
                            }
                            else ImGui::Text("No sequence");
                        }
                    }

                    // CSV label display
                    if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get())) {
                        if (gfa->hasAttachedCSV()) {
                            if (auto* name = std::get_if<std::string_view>(&props["Name_SW"])) {
                                std::string nodeName(name->data(), name->size());
                                if (const auto* csvData = gfa->getAttachedCSV()->getNodeData(nodeName)) {
                                    ImGui::Separator();
                                    ImGui::TextUnformatted("CSV Labels");
                                    for (const auto& [colName, value] : csvData->columns) {
                                        LabeledSV(colName.c_str(), std::string_view(value));
                                    }
                                }
                                else {
                                    ImGui::Separator();
                                    ImGui::TextDisabled("No CSV labels for this node");
                                }
                            }
                        }
                    }
                }
                else if (*type == GFA::mapType::LINK) {
                    auto* fromName = std::get_if<std::string_view>(&props["FromName_SW"]);
                    auto* fromOri = std::get_if<char>(&props["FromOrientation_C"]);
                    auto* toName = std::get_if<std::string_view>(&props["ToName_SW"]);
                    auto* toOri = std::get_if<char>(&props["ToOrientation_C"]);

                    if (fromName && fromOri && toName && toOri) {
                        ImGui::Text("Link:");
                        ImGui::SameLine();
                        ImGui::TextUnformatted(fromName->data(), fromName->data() + fromName->size());
                        ImGui::SameLine(0, 0);
                        ImGui::Text("%c", *fromOri);
                        ImGui::SameLine();
                        ImGui::Text("->");
                        ImGui::SameLine();
                        ImGui::TextUnformatted(toName->data(), toName->data() + toName->size());
                        ImGui::SameLine(0, 0);
                        ImGui::Text("%c", *fromOri);
                    }

                    if (m_verboseMode){
                        if (auto* edgeId = std::get_if<std::string_view>(&props["EdgeIdentifier_SW"]))
                            if (!edgeId->empty()) LabeledSV("Edge ID:", *edgeId);
                        if (auto* cigar = std::get_if<bool>(&props["CigarAvailable_B"])){
                            if (*cigar){
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("CIGAR exists >");
                                ImGui::SameLine();
                                if (ImGui::Button("View##cigar")) {
                                    if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get())) {
                                        if (auto result = gfa->retrieveCIGAR(idx)) {
                                            auto& [path, pos] = *result;
                                            seqWin.setSequence(path, pos, true);
                                        }
                                    } else {
                                        spdlog::warn("Filetype not implemented yet");
                                    }
                                }
                            }
                        }
                        if (auto* kc = std::get_if<long long>(&props["KmerCount_LLI"]))
                            ImGui::Text("K-mer count: %lld", *kc);
                        if (auto* rc = std::get_if<long long>(&props["ReadCount_LLI"]))
                            ImGui::Text("Read count: %lld", *rc);
                        if (auto* fc = std::get_if<long long>(&props["FragmentCount_LLI"]))
                            ImGui::Text("Fragment count: %lld", *fc);
                        if (auto* mm = std::get_if<long long>(&props["MismatchGaps_LLI"]))
                            ImGui::Text("Mismatches/Gaps: %lld", *mm);
                        if (auto* mq = std::get_if<long long>(&props["MappingQuality_LLI"]))
                            ImGui::Text("Mapping quality: %lld", *mq);
                    }
                }
            }
            else ImGui::Text("No data on selection");
        }
        else{ // more than one object to show -> show summary
            static size_t segmentCount = 0, linkCount = 0;

            static double sumLength = 0, sumDepth = 0;
            static size_t countLength = 0, countDepth = 0;

            static double sumMismatchGaps = 0, sumMappingQuality = 0;
            static size_t countMismatchGaps = 0, countMappingQuality = 0;

            // Gather stats if data is new
            if (change){
                // reset all values
                segmentCount = linkCount = countLength = countDepth = countMismatchGaps = countMappingQuality = 0;
                sumLength = sumDepth = sumMismatchGaps = sumMappingQuality = 0.0;

                for (const auto& idx : indices) {
                    // get data and always be verbose for this mode since we filter data later to save on processing
                    auto props = channel->file_data->retrieveEdgeData(idx, true);
                    if (props.empty()) continue;
    
                    auto* type = std::get_if<char>(&props["Type_C"]);
                    if (!type) continue;
    
                    if (*type == GFA::mapType::SEGMENT) {
                        segmentCount++;
                        if (auto* v = std::get_if<long long>(&props["Length_LLI"])) {
                            sumLength += *v;
                            countLength++;
                        }
                        if (auto* v = std::get_if<double>(&props["Depth_D"])) {
                            sumDepth += *v;
                            countDepth++;
                        }
                    }
                    else if (*type == GFA::mapType::LINK) {
                        linkCount++;
                        if (auto* v = std::get_if<long long>(&props["MismatchGaps_LLI"])) {
                            sumMismatchGaps += *v;
                            countMismatchGaps++;
                        }
                        if (auto* v = std::get_if<long long>(&props["MappingQuality_LLI"])) {
                            sumMappingQuality += *v;
                            countMappingQuality++;
                        }
                    }
                }
            }

            // Display summary
            ImGui::Text("Selected: %zu elements", indices.size());

            if (segmentCount > 0) {
                ImGui::Text("Segments: %zu", segmentCount);
                if (countLength > 0) ImGui::Text("  Avg length: %.1f", sumLength / countLength);
                if (countDepth > 0) ImGui::Text("  Avg depth: %.2f", sumDepth / countDepth);
            }

            if (linkCount > 0) {
                ImGui::Text("Links: %zu", linkCount);
                if (m_verboseMode){
                    if (countMismatchGaps > 0) ImGui::Text("  Avg mismatches/gaps: %.1f", sumMismatchGaps / countMismatchGaps);
                    if (countMappingQuality > 0) ImGui::Text("  Avg mapping quality: %.1f", sumMappingQuality / countMappingQuality);
                }
            }
        }

        ImGui::Separator();
        if (m_verboseMode){
            if (ImGui::Button("See less")) m_verboseMode = false;
        } else{
            if (ImGui::Button("See more")) m_verboseMode = true;
        }
        
        // ---- Custom color ----
        if (!indices.empty()) {
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Custom Coloring")){
                ImGui::Text("Color Selected");
                static ImGuiColorEditFlags colorFlags = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview;
    
                ImGui::ColorPicker4("##CustomColor", m_customColor, colorFlags);
                float itemWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                ImGui::ColorButton("##Selection_preview",
                    ImVec4(m_customColor[0], m_customColor[1], m_customColor[2], m_customColor[3]), 0, ImVec2(itemWidth, 0)
                );
                ImGui::SameLine();
                if (ImGui::Button("Apply Color", ImVec2(itemWidth, 0))) {
                    glm::u8vec4 tmp(
                        static_cast<uint8_t>(m_customColor[0] * 255), static_cast<uint8_t>(m_customColor[1] * 255),
                        static_cast<uint8_t>(m_customColor[2] * 255), static_cast<uint8_t>(m_customColor[3] * 255)
                    );
                    uint32_t col = std::bit_cast<uint32_t>(tmp);
                    auto eids = channel->renderer->getGroupIDs(groups::SELECTION);
                    std::sort(eids.begin(), eids.end()); // to keep the map sorted
                    {
                        std::lock_guard<std::mutex> lk(channel->color_storage_mut);
                        auto& vec = channel->color_storage[col];
                        size_t split = vec.size();
                        // combine the vectors using inplace_merge (will keep the result sorted under the assumption that vec and eids are sorted)
                        vec.insert(vec.begin(), eids.begin(), eids.end());
                        std::inplace_merge(vec.begin(), vec.begin() + split, vec.end());
                    }
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }
            }
        }

        std::lock_guard<std::mutex> lk(channel->color_storage_mut);
        if (!channel->color_storage.empty()){
            ImGui::Separator();
            if (ImGui::Button("Reset All Custom Colors")) {
                channel->color_storage.clear();
                channel->addControllerTask(tasks::REFRESH_GRAPH);
            }
        }

        ImGui::End();
    }
    
    // draw the sequence window
    seqWin.draw();
}