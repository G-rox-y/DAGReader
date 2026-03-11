#include "selection.hpp"

void selectionWindow::draw() {
    // fetch selection data that will be written
    auto indices = channel->renderer->getGroupIDs(groups::SELECTION);
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
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Selection HUD", NULL, flags)){
        if (indices.size() == 1){
            size_t idx = indices[0];
            auto props = channel->file_data->retrieveEdgeData(idx, m_verboseMode);
            if (!props.empty()) {
                auto* type = std::get_if<int>(&props["Type_I"]);
                if (*type == datatype::mapType::SEGMENT) {
                    if (auto* name = std::get_if<std::string_view>(&props["Name_SW"]))
                        LabeledSV("Segment:", *name);
                    if (auto* length = std::get_if<long long>(&props["Length_LLI"]))
                        ImGui::Text("Length: %lld", *length);
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
                            ImGui::Text("Sequence: %s", *seq ? "available" : "unavailable");
                            // TODO: add an buttoned option to show sequence in another window
                        }
                    }
                }
                else if (*type == datatype::mapType::LINK) {
                    auto* fromName = std::get_if<std::string_view>(&props["FromName_SW"]);
                    auto* fromOri = std::get_if<std::string_view>(&props["FromOrientation_SW"]);
                    auto* toName = std::get_if<std::string_view>(&props["ToName_SW"]);
                    auto* toOri = std::get_if<std::string_view>(&props["ToOrientation_SW"]);

                    if (fromName && fromOri && toName && toOri) {
                        ImGui::Text("Link:");
                        ImGui::SameLine();
                        ImGui::TextUnformatted(fromName->data(), fromName->data() + fromName->size());
                        ImGui::SameLine(0, 0);
                        ImGui::TextUnformatted(fromOri->data(), fromOri->data() + fromOri->size());
                        ImGui::SameLine();
                        ImGui::Text("->");
                        ImGui::SameLine();
                        ImGui::TextUnformatted(toName->data(), toName->data() + toName->size());
                        ImGui::SameLine(0, 0);
                        ImGui::TextUnformatted(toOri->data(), toOri->data() + toOri->size());
                    }

                    if (m_verboseMode){
                        if (auto* edgeId = std::get_if<std::string_view>(&props["EdgeIdentifier_SW"]))
                            if (!edgeId->empty()) LabeledSV("Edge ID:", *edgeId);
                        if (auto* cigar = std::get_if<std::string_view>(&props["Cigar_SW"]))
                            if (!cigar->empty()) LabeledSV("CIGAR:", *cigar);
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
            size_t segmentCount = 0, linkCount = 0;

            double sumLength = 0, sumDepth = 0;
            size_t countLength = 0, countDepth = 0;

            double sumMismatchGaps = 0, sumMappingQuality = 0;
            size_t countMismatchGaps = 0, countMappingQuality = 0;

            // Gather stats
            for (const auto& idx : indices) {
                auto props = channel->file_data->retrieveEdgeData(idx, m_verboseMode);
                if (props.empty()) continue;

                auto* type = std::get_if<int>(&props["Type_I"]);
                if (!type) continue;

                if (*type == groups::SEGMENT) {
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
                else if (*type == groups::LINK) {
                    linkCount++;
                    if (!m_verboseMode) continue;
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

        if (m_verboseMode){
            if (ImGui::Button("See less")) m_verboseMode = false;
        } else{
            if (ImGui::Button("See more")) m_verboseMode = true;
        }

        ImGui::End();
    }
    
}