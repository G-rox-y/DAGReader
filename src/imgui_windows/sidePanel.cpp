#include "sidePanel.hpp"

void sidePanel::draw()
{
    if(!channel->sidebar_window_shown.load()) return;

    ImGuiIO& io = ImGui::GetIO();

    const ImGuiViewport* viewport = ImGui::GetMainViewport(); // get the viewport

    // force the panel to be under the menu and one the right side
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - m_width, viewport->WorkPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(m_width, 0), ImVec2(m_width, viewport->WorkSize.y));

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings 
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
    if (ImGui::Begin("Side Panel", NULL, flags))
    {
        float availX = ImGui::GetContentRegionAvail().x;
        float w = (availX - ImGui::GetStyle().ItemSpacing.y) * 0.75f;

        ImGui::Text("Hi :D");
        if (ImGui::Button("Layout the graph!")){
            channel->renderer->clearAll(); // has to be cleared here cause this thread has the opengl context
            channel->addControllerTask(tasks::LAYOUT_GRAPH);
        }
        if(channel->graph_param_change.load() && !channel->graph_auto_update.load() && channel->graph_loaded.load()){
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

        bool copy_gau = channel->graph_auto_update.load();
        if (ImGui::Checkbox("Auto update graph layout", &copy_gau))
            channel->graph_auto_update.store(copy_gau);

        if(ImGui::TreeNode("Layout settings"))
        {
            bool copy_gadsl = channel->graph_auto_determine_segment_length.load();
            if (ImGui::Checkbox("Auto calculate segment fragment size", &copy_gadsl)){
                channel->graph_auto_determine_segment_length.store(copy_gadsl);
                if (channel->graph_loaded.load()) channel->graph_param_change.store(true);
            }

            ImGui::TextWrapped("Segment fragment size");
            ImGui::BeginDisabled(copy_gadsl);
            ImGui::SetNextItemWidth(m_width * 0.7f);
            long long step = 1, step_fast = std::max<long long int>(channel->graph_segment_length.load() / 100, 10);
            long long int copy_gsl = channel->graph_segment_length.load();
            if (ImGui::InputScalar("##segment_fragment_size", ImGuiDataType_S64, &copy_gsl, &step, &step_fast) && copy_gsl > 0){
                channel->graph_segment_length.store(copy_gsl);
                if (channel->graph_loaded.load()) channel->graph_param_change.store(true);
            }

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

            ImGui::SameLine();
            HelpMarker("This number controls how big the segments will end up being\n---\n"
                "Example: if a segment is 5000 base pairs long, and this number is set to 1000, "
                "the segment size will be 5\n[minimal size is 1]\n---\n"
                "Pro tip: you can hold CTRL while holding the - + buttons to quickly tune the values");
            ImGui::TreePop();
            ImGui::EndDisabled();
        }
        if (ImGui::TreeNode("Appearance"))
        {
            // TODO: add segment and link widths

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

            ImGui::TreePop();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
}