#include "sidePanel.hpp"

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
            ImGui::Text("Segments: %i", channel->graph_data_seg_num.load());
            ImGui::Text("Links: %i", channel->graph_data_link_num.load());

            size_t subgraphNum = channel->subgraphAmount();
            ImGui::Text("Subgraphs: %zu", subgraphNum);
            
            ImGui::Separator();

            if (channel->layout_in_progress.load()){
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(w, 0.0f), "Laying out the graph...");
            }
            else{
                if (ImGui::Button("Layout the graph!")){
                    channel->renderer->clearAll(); // has to be cleared here cause this thread has the opengl context
                    channel->addControllerTask(tasks::LAYOUT_GRAPH);
                    channel->addControllerTask(tasks::RESET_GRAPH);
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

            bool openLayoutSettings = ImGui::CollapsingHeader("Layout settings");
            if(openLayoutSettings)
            {
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
            bool openAppearance = ImGui::CollapsingHeader("Appearance");
            if (openAppearance)
            {
                float sw = channel->segment_widths.load();

                if (ImGui::DragFloat("Segment width", &sw, 0.001f, 0.f, 1000.f, "%6.3f", ImGuiSliderFlags_AlwaysClamp)){
                    channel->segment_widths.store(sw);
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }

                float lw = channel->link_widths.load();
                if (ImGui::DragFloat("Link width", &lw, 0.001f, 0.f, 1000.f, "%6.3f", ImGuiSliderFlags_AlwaysClamp)){
                    channel->link_widths.store(lw);
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }
                
                bool rsc = channel->randomize_segment_colors.load();
                if (ImGui::Checkbox("Randomize segment colors", &rsc)){
                    channel->randomize_segment_colors.store(rsc);
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }

                bool rlc = channel->randomize_link_colors.load();
                if(ImGui::Checkbox("Randomize link colors", &rlc)){
                    channel->randomize_link_colors.store(rlc);
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }

                
                // ---

                static ImGuiItemFlags colorFlags = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar 
                    | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview;

                // static because i need this only to init on loading and later on its free to change
                static glm::u8vec4 colors1vec = channel->segment_color_packed.load();
                static float colors1[] = {
                    static_cast<float>(colors1vec.x)/255.f , static_cast<float>(colors1vec.y)/255.f, 
                    static_cast<float>(colors1vec.z)/255.f, static_cast<float>(colors1vec.w)/255.f
                };
                ImGui::SeparatorText("Segment color");
                ImGui::BeginDisabled(rsc);
                ImGui::SetNextItemWidth(w);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - w) / 2);
                if (ImGui::ColorPicker4("##Segment_colors", (float*)&colors1, colorFlags)){
                    channel->segment_color_packed.store(
                        glm::u8vec4(255 * colors1[0], 255 * colors1[1], 255 * colors1[2], 255 * colors1[3])
                    );
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }
                ImGui::EndDisabled();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - w) / 2);
                ImGui::ColorButton("##Segment_preview",
                    ImVec4(colors1[0], colors1[1], colors1[2], colors1[3]), ImGuiColorEditFlags_None, ImVec2(w, 0)
                );
                
                // ----
                
                static glm::u8vec4 colors2vec = channel->link_color_packed.load();
                static float colors2[] = {
                    static_cast<float>(colors2vec.x)/255.f , static_cast<float>(colors2vec.y)/255.f, 
                    static_cast<float>(colors2vec.z)/255.f, static_cast<float>(colors2vec.w)/255.f
                };
                ImGui::SeparatorText("Link color");
                ImGui::BeginDisabled(rlc);
                ImGui::SetNextItemWidth(w);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - w) / 2);
                if (ImGui::ColorPicker4("##Link_colors", (float*)&colors2, colorFlags)){
                    channel->link_color_packed.store(
                        glm::u8vec4(255 * colors2[0], 255 * colors2[1], 255 * colors2[2], 255 * colors2[3])
                    );
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }
                ImGui::EndDisabled();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - w) / 2);
                ImGui::ColorButton("##Link_preview", 
                    ImVec4(colors2[0], colors2[1], colors2[2], colors2[3]), ImGuiColorEditFlags_None, ImVec2(w, 0)
                );
            }

            bool openSubgraphSelection = ImGui::CollapsingHeader("Subgraph selection");
            if (openSubgraphSelection)
            {
                if(ImGui::Button("Hide All")){
                    for(size_t i = 0; i < subgraphNum; i++)
                        channel->hideGroup(i);
                    channel->addControllerTask(tasks::RESET_GRAPH);
                }
                ImGui::SameLine();
                if(ImGui::Button("Unhide All")){
                    for(size_t i = 0; i < subgraphNum; i++)
                        channel->unhideGroup(i);
                    channel->addControllerTask(tasks::RESET_GRAPH);
                }
                ImGui::SameLine();
                if(ImGui::Button("Select All")){
                    for(size_t i = 0; i < subgraphNum; i++)
                        channel->selectSubGraph(i);
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }
                ImGui::SameLine();
                if(ImGui::Button("Unselect All")){
                    for(size_t i = 0; i < subgraphNum; i++)
                        channel->unselectSubGraph(i);
                    channel->addControllerTask(tasks::REFRESH_GRAPH);
                }

                bool tableCausedReset = false, tableCausedRefresh = false;
                static ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY;
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
                if (tableCausedReset) channel->addControllerTask(tasks::RESET_GRAPH);
                if (tableCausedRefresh) channel->addControllerTask(tasks::REFRESH_GRAPH);
            }
        }
        else{
            ImGui::SeparatorText("Top Menu > File > Open File");
            ImGui::Separator();
            ImGui::TextWrapped("Hi, this project is still very much in development");
            ImGui::TextWrapped("if you have any suggestions feel free to submit a feature request on the following link:");
            ImGui::TextLinkOpenURL("Github Issues Page", "https://github.com/G-rox-y/DAGReader/issues");
            ImGui::Separator();
            ImGui::TextWrapped("I will add a brief tutorial here soon");
        }
    }

    ImGui::End();
}