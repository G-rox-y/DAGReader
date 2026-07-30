#include "sidePanel.hpp"

using enum tasks::controllerTask;
using enum infoExchange::colScheme;
using enum infoExchange::colRule;

void sidePanel::drawLayout(float availX, float w)
{
    if(!ImGui::CollapsingHeader("Layout settings")) return;

    int copy_grm = channel->grip_roundsNum.load();
    if (ImGui::SliderInt("Number of Rounds", &copy_grm, 3, 50, "%d")){
        channel->grip_roundsNum.store(copy_grm);
        if (channel->graph_loaded.load()) channel->graph_param_change.store(true);
    }

    float copy_gtg = channel->grip_tempGain.load();
    if (ImGui::SliderFloat("Temp Gain", &copy_gtg, 0.f, 1.f, "%.2f")){
        channel->grip_tempGain.store(copy_gtg);
        if (channel->graph_loaded.load()) channel->graph_param_change.store(true);
    }

    float copy_gtng = channel->grip_tempNarrowGain.load();
    if (ImGui::SliderFloat("Temp Narrow Gain", &copy_gtng, 1.f, 3.f, "%.2f")){
        channel->grip_tempNarrowGain.store(copy_gtng);
        if (channel->graph_loaded.load()) channel->graph_param_change.store(true);
    }

    float copy_gsf = channel->grip_scalingFactor.load();
    if (ImGui::SliderFloat("Scaling Factor", &copy_gsf, 0.f, 2.f, "%.3f")){
        channel->grip_scalingFactor.store(copy_gsf);
        if (channel->graph_loaded.load()) channel->graph_param_change.store(true);
    }
}

void sidePanel::drawAppearance(float availX, float w)
{
    if (!ImGui::CollapsingHeader("Appearance")) return;

    bool scc = channel->show_custom_colors.load();
    if (ImGui::Checkbox("Show Custom Color", &scc)){
        channel->show_custom_colors.store(scc);
        channel->addControllerTask(REFRESH_GRAPH);
    }

    ImGui::SameLine();
    HelpMarker("Allows you to hide the custom colors you can assign to the segments through the selection dialogue");

    ImGui::SeparatorText("Dynamic coloring");

    // segment coloring
    const char* segItems[] = {"None", "Random", "Depth", "Length", "CSV"};

    bool csvAvailable = false;
    if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get()))
        csvAvailable = gfa->hasAttachedCSV();

    if (m_segColorScheme == 4 && !csvAvailable) { // Safety: if CSV mode was active but CSV is gone, reset to NONE
        m_segColorScheme = 0;
        channel->segment_color_scheme.store(NONE);
    }
    if (ImGui::Combo("Segment color scheme", &m_segColorScheme, segItems, IM_ARRAYSIZE(segItems))) {
        if (m_segColorScheme == 0) channel->segment_color_scheme.store(NONE);
        else if (m_segColorScheme == 1) channel->segment_color_scheme.store(RANDOM);
        else if (m_segColorScheme == 2) channel->segment_color_scheme.store(DEPTH);
        else if (m_segColorScheme == 3) channel->segment_color_scheme.store(LENGTH);
        else{
            if (!csvAvailable) channel->segment_color_scheme.store(NONE);
            else channel->segment_color_scheme.store(CSV);
        }
        channel->addControllerTask(REFRESH_GRAPH);
    }
    if (!csvAvailable) {
        ImGui::SameLine();
        HelpMarker("CSV coloring requires a CSV to be loaded via File > Load CSV labels");
    }

    // segment coloring rules
    if(m_segColorScheme == 2 || m_segColorScheme == 3){
        const char* segRules[] = {"Normal", "Sqrt", "Cbrt", "Progressive"};
        if(ImGui::Combo("Segment coloring rule", &m_segColorRule, segRules, IM_ARRAYSIZE(segRules))){
            if (m_segColorRule == 0) channel->segment_color_rule.store(NORMAL);
            else if (m_segColorRule == 1) channel->segment_color_rule.store(SQRT);
            else if (m_segColorRule == 2) channel->segment_color_rule.store(CBRT);
            else channel->segment_color_rule.store(PROGRESSIVE);
            channel->addControllerTask(REFRESH_GRAPH);
        }
    }

    // link coloring
    const char* linkItems[] = {"None", "Random"};
    if(ImGui::Combo("Link color scheme", &m_linkColorScheme, linkItems, IM_ARRAYSIZE(linkItems))){
        if (m_linkColorScheme == 0) channel->link_color_scheme.store(NONE);
        else channel->link_color_scheme.store(RANDOM);
        channel->addControllerTask(REFRESH_GRAPH);
    }


    ImGui::SeparatorText("Dimensions");

    float sw = channel->segment_widths.load();
    if (ImGui::DragFloat("Segment width", &sw, 0.001f, 0.f, 1000.f, "%6.3f", ImGuiSliderFlags_AlwaysClamp)){
        channel->segment_widths.store(sw);
        channel->addControllerTask(REFRESH_GRAPH);
    }
    float lw = channel->link_widths.load();
    if (ImGui::DragFloat("Link width", &lw, 0.001f, 0.f, 1000.f, "%6.3f", ImGuiSliderFlags_AlwaysClamp)){
        channel->link_widths.store(lw);
        channel->addControllerTask(REFRESH_GRAPH);
    }


    ImGui::SeparatorText("Transparencies");

    if(ImGui::SliderFloat("Segment Transparency", (float*)&m_segColors[3], 0.f, 1.f)){
        channel->segment_color_packed.store(
            glm::u8vec4(255 * m_segColors[0], 255 * m_segColors[1], 255 * m_segColors[2], 255 * m_segColors[3])
        );
        channel->addControllerTask(REFRESH_GRAPH);
    }
    if(ImGui::SliderFloat("Link Transparency", (float*)&m_linkColors[3], 0.f, 1.f)){
        channel->link_color_packed.store(
            glm::u8vec4(255 * m_linkColors[0], 255 * m_linkColors[1], 255 * m_linkColors[2], 255 * m_linkColors[3])
        );
        channel->addControllerTask(REFRESH_GRAPH);
    }


    // color pickers
    static ImGuiItemFlags colorFlags = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar 
        | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview;

    auto colorpicker = [&](const char* name, std::array<float, 4>& colors, std::atomic<glm::u8vec4>& storage){
        ImGui::SeparatorText(fmt::format("{} color", name).c_str());
        ImGui::SetNextItemWidth(2*w/3);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - w) / 2);
        if (ImGui::ColorPicker4(fmt::format("##{}_colors", name).c_str(), colors.data(), colorFlags)){
            storage.store(glm::u8vec4(255 * colors[0], 255 * colors[1], 255 * colors[2], 255 * colors[3]));
            channel->addControllerTask(REFRESH_GRAPH);
        }
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - w) / 2);
        ImGui::ColorButton(fmt::format("##{}_preview", name).c_str(), 
            ImVec4(colors[0], colors[1], colors[2], colors[3]), ImGuiColorEditFlags_None, ImVec2(2*w/3, 0)
        );
    };

    if (channel->selection_mode.load())
        colorpicker("Selection", m_selectionColors, channel->selected_color_packed);

    if (m_segColorScheme == 0 || m_segColorScheme == 4)
        colorpicker("Segment", m_segColors, channel->segment_color_packed);
    
    if (m_linkColorScheme == 0)
        colorpicker("Link", m_linkColors, channel->link_color_packed);
}

void sidePanel::drawSubgraphSelection(float availX, float w, size_t subgraphNum)
{
    if (!ImGui::CollapsingHeader("Subgraph selection")) return;

    if(ImGui::Button("Hide All")){
        for(size_t i = 0; i < subgraphNum; i++) channel->hideGroup(i);
        channel->addControllerTask(RESET_GRAPH);
    }
    ImGui::SameLine();
    if(ImGui::Button("Unhide All")){
        for(size_t i = 0; i < subgraphNum; i++) channel->unhideGroup(i);
        channel->addControllerTask(RESET_GRAPH);
    }
    ImGui::SameLine();
    if(ImGui::Button("Select All")){
        for(size_t i = 0; i < subgraphNum; i++) channel->selectSubGraph(i);
        channel->addControllerTask(REFRESH_GRAPH);
    }
    ImGui::SameLine();
    if(ImGui::Button("Unselect All")){
        for(size_t i = 0; i < subgraphNum; i++) channel->unselectSubGraph(i);
        channel->addControllerTask(REFRESH_GRAPH);
    }

    bool tableCausedReset = false, tableCausedRefresh = false;
    static ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("subgraphTable", 5, table_flags)){
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("segments");
        ImGui::TableSetupColumn("edges");
        ImGui::TableSetupColumn("show");
        ImGui::TableSetupColumn("select");
        ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
        ImGui::TableHeadersRow();

        for (size_t graphID = 0; graphID < subgraphNum; graphID++){
            auto& D = channel->getSubgraphData(graphID);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", graphID);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", D.segment_num);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", D.edge_num);
            ImGui::TableSetColumnIndex(3);
            bool val = !channel->isGroupHidden(graphID);
            if (ImGui::Checkbox(fmt::format("##Hide{}", graphID).c_str(), &val)){
                if (!val) channel->hideGroup(graphID);
                else channel->unhideGroup(graphID);
                tableCausedReset = true;
            }
            ImGui::TableSetColumnIndex(4);
            val = D.selected;
            if (ImGui::Checkbox(fmt::format("##Select{}", graphID).c_str(), &val)){
                if (!val) channel->unselectSubGraph(graphID);
                else channel->selectSubGraph(graphID);
                tableCausedRefresh = true;
            }
        }
        ImGui::EndTable();
    }
    if (tableCausedReset) channel->addControllerTask(RESET_GRAPH);
    if (tableCausedRefresh) channel->addControllerTask(REFRESH_GRAPH);
}

void sidePanel::drawSearch(float availX, float w)
{
    if (!ImGui::CollapsingHeader("Search")) return;
    static char searchBuffer[256] = "";
    static bool filterSegments = true, filterLinks = true, filterPaths = false, filterContainments = false, fuzzy = false;
    static std::vector<std::tuple<size_t, GFA::mapType, std::optional<size_t>>> results;
    
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    
    bool enterPressed = ImGui::InputTextWithHint(
        "##searchBar", 
        "Search...", 
        searchBuffer, 
        sizeof(searchBuffer),
        ImGuiInputTextFlags_EnterReturnsTrue
    );
    
    ImGui::PopStyleVar(2);

    // clear button
    if (searchBuffer[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::Button("clear##clear")) {
            searchBuffer[0] = '\0';
        }
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    ImGui::Checkbox("Segments", &filterSegments);
    ImGui::SameLine();
    ImGui::Checkbox("Links", &filterLinks);

    if (auto* _ = std::get_if<size_t>(&data["PathNumber_ULLI"])){
        ImGui::SameLine();
        ImGui::Checkbox("Paths", &filterPaths);
    }
    if (auto* _ = std::get_if<size_t>(&data["ContainmentNumber_ULLI"])){
        ImGui::SameLine();
        ImGui::Checkbox("Containments", &filterContainments);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Fuzzy [WIP]", &fuzzy);

    ImGui::Spacing();
    int filters = filterSegments * GFA::mapType::SEGMENT + filterLinks * GFA::mapType::LINK
        + filterContainments * GFA::mapType::CONTAINMENT + filterPaths * GFA::mapType::PATH;
    if (ImGui::Button("Search##Button") || enterPressed){
        if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get())){
            if (fuzzy) gfa->searchFuzzyForName(searchBuffer, filters);
            else results = gfa->searchStrictForName(searchBuffer, filters);
        }
    }
    
    ImGui::SameLine();
    auto resultCount = results.size();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("(%zu results)", resultCount);
    ImGui::SameLine();
    HelpMarker("You can click on a result for more actions");

    // here we display results
    if (!results.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto typeLabel = [](GFA::mapType t) -> const char* {
            switch (t) {
                case GFA::mapType::SEGMENT:     return "SEG";
                case GFA::mapType::LINK:        return "LNK";
                case GFA::mapType::PATH:        return "PTH";
                case GFA::mapType::CONTAINMENT: return "CNT";
                default:                        return "???";
            }
        };

        auto typeColor = [](GFA::mapType t) -> ImU32 {
            switch (t) {
                case GFA::mapType::SEGMENT:     return IM_COL32(100, 180, 100, 255);
                case GFA::mapType::LINK:        return IM_COL32(100, 150, 200, 255);
                case GFA::mapType::PATH:        return IM_COL32(200, 150, 100, 255);
                case GFA::mapType::CONTAINMENT: return IM_COL32(180, 100, 180, 255);
                default:                        return IM_COL32(150, 150, 150, 255);
            }
        };

        float maxHeight = ImGui::GetTextLineHeightWithSpacing() * 10;  // ~10 rows visible
        if (ImGui::BeginChild("##SearchResults", ImVec2(0, std::min(maxHeight, ImGui::GetTextLineHeightWithSpacing() * resultCount + 8)), true))
        { 
            auto* gfa = dynamic_cast<GFA*>(channel->file_data.get());
            
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(results.size()));
            
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    auto& [objectId, type, edgeId] = results[i];
                    
                    ImGui::PushID(static_cast<int>(i));
                    
                    // Type badge
                    ImVec2 badgeSize = ImGui::CalcTextSize("SEG");
                    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    
                    float padding = 4.0f;
                    ImVec2 badgeMin = cursorPos;
                    ImVec2 badgeMax = ImVec2(cursorPos.x + badgeSize.x + padding * 2, cursorPos.y + badgeSize.y + padding);
                    
                    drawList->AddRectFilled(badgeMin, badgeMax, typeColor(type), 3.0f);
                    drawList->AddText(ImVec2(cursorPos.x + padding, cursorPos.y + padding * 0.5f), IM_COL32(255, 255, 255, 255), typeLabel(type));
                    
                    ImGui::Dummy(ImVec2(badgeSize.x + padding * 2 + 8, badgeSize.y));
                    ImGui::SameLine();

                    // Get name based on type
                    std::string name;
                    if (gfa) {
                        auto props = gfa->retrieveObjectData(objectId, type, false);
                        if (auto* sv = std::get_if<std::string_view>(&props["Name_SW"])) {
                            name = std::string(*sv);
                        } else if (auto* fromSv = std::get_if<std::string_view>(&props["FromName_SW"])) {
                            auto* toSv = std::get_if<std::string_view>(&props["ToName_SW"]);
                            name = std::string(*fromSv) + " -> " + (toSv ? std::string(*toSv) : "?");
                        } else if (auto* fromSv = std::get_if<std::string_view>(&props["ContainerName_SW"])) {
                            auto* toSv = std::get_if<std::string_view>(&props["ContainedName_SW"]);
                            name = std::string(*fromSv) + " << " + (toSv ? std::string(*toSv) : "?");
                        } else {
                            name = "Object ID: " + std::to_string(objectId);
                        }
                    }

                    // Selectable row
                    if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                        ImGui::OpenPopup("Actions");
                    }
                    
                    if (ImGui::BeginPopup("Actions")) {
                        if (edgeId.has_value()){
                            if (ImGui::MenuItem("Focus on item")) {
                                if (auto ret = channel->renderer->getIDOrbitData(edgeId.value())){
                                    auto[pos, d] = ret.value();
                                    channel->cam->engageOrbit(pos, d*5);
                                    channel->cam->disengageOrbit();
                                    channel->cam->shouldRecalc(); // 31337 |-|4><
                                } 
                            }
                            if (ImGui::MenuItem("Add to selection")){
                                channel->renderer->addID2Group(edgeId.value(), groups::rendererGroup::SELECTION);
                                channel->addControllerTask(REFRESH_GRAPH);
                            }
                        }
                        if (ImGui::MenuItem("Show details [WIP]")) {
                            // TODO
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }
            }
            clipper.End();
        }
        ImGui::EndChild();
        
        if (ImGui::Button("Select All Results")){
            for (auto& [_, __, eid] : results)
                if (eid.has_value()) channel->renderer->addID2Group(eid.value(), groups::rendererGroup::SELECTION);
            channel->addControllerTask(REFRESH_GRAPH);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Results")) results.clear();
    }
}

sidePanel::sidePanel(infoExchange* c) : channel(c) {
    auto unpackColor = [](const glm::u8vec4& c) -> std::array<float, 4> {
        return { c.x / 255.f, c.y / 255.f, c.z / 255.f, c.w / 255.f };
    };
    m_segColors       = unpackColor(channel->segment_color_packed.load());
    m_linkColors      = unpackColor(channel->link_color_packed.load());
    m_selectionColors = unpackColor(channel->selected_color_packed.load());
}

void sidePanel::draw()
{
    if(!channel->sidebar_window_shown.load()) return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport(); // get the viewport

    // force the panel to be under the menu and one the right side
    ImVec2 ll(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y);
    ImGui::SetNextWindowPos(ll, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(viewport->WorkSize.x*0.55, viewport->WorkSize.y*0.9));

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings 
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Side Panel", NULL, flags))
    {
        float availX = ImGui::GetContentRegionAvail().x;
        float w = (availX - ImGui::GetStyle().ItemSpacing.y) * 0.75f;

        if (channel->loading_file_in_progress.load()){
            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(w, 0.0f), "Loading a file...");
        }
        else if(channel->graph_loaded.load())
        {
            std::string name{channel->graph_name_get()};
            ImGui::SeparatorText(name.c_str());

            data = channel->file_data->retrieveGeneralData();

            if (auto* segnum = std::get_if<size_t>(&data["SegmentNumber_ULLI"]))
                ImGui::Text("Segments: %zu", *segnum);
            if (auto* linknum = std::get_if<size_t>(&data["LinkNumber_ULLI"]))
                ImGui::Text("Links: %zu", *linknum);
            if (auto* pathnum = std::get_if<size_t>(&data["PathNumber_ULLI"]))
                ImGui::Text("Paths: %zu", *pathnum);
            if (auto* contnum = std::get_if<size_t>(&data["ContainmentNumber_ULLI"]))
                ImGui::Text("Containments: %zu", *contnum);

            size_t subgraphNum = channel->subgraphAmount();
            ImGui::Text("Subgraphs: %zu", subgraphNum);
            
            ImGui::Separator();

            if (channel->layout_in_progress.load()){
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(w, 0.0f), "Laying out the graph...");
            }
            else{
                if (ImGui::Button("Layout the graph!")){
                    channel->renderer->clearAll(); // has to be cleared here cause this thread has the opengl context
                    channel->addControllerTask(LAYOUT_GRAPH);
                    channel->addControllerTask(RESET_GRAPH);
                    channel->addControllerTask(REFRESH_GRAPH);
                }
                if(channel->graph_param_change.load() && channel->graph_loaded.load()){
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "*");
                    if (ImGui::BeginItemTooltip()){
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted("You have made updates, click the button to redraw");
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }
                ImGui::SameLine();
                HelpMarker("This will calculate (or recalculate) the graph layout using the parameters and data from the input file");
            }

            drawLayout(availX, w);
            drawAppearance(availX, w);
            drawSubgraphSelection(availX, w, subgraphNum);
            drawSearch(availX, w);
        }
        else{
            ImGui::SeparatorText("Top Menu > File > Open File");
            ImGui::Separator();
            ImGui::TextWrapped("Hi, this project is still very much in development");
            ImGui::TextWrapped("if you have any suggestions or bugs to report feel free to submit a feature request on the following link:");
            ImGui::TextLinkOpenURL("Github Issues Page", "https://github.com/G-rox-y/DAGReader/issues");
        }
    }

    ImGui::End();
}