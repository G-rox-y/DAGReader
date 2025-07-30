#include "sidePanel.hpp"

sidePanel::sidePanel(infoExchange* c, const float w) : channel(c), m_width(w), m_collapsed(false) {}

void sidePanel::draw()
{
    ImGuiIO& io = ImGui::GetIO();

    const ImGuiViewport* viewport = ImGui::GetMainViewport(); // get the viewport

    // force the panel to be under the menu and one the right side
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - m_width, viewport->WorkPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(m_width, 0), ImVec2(m_width, viewport->WorkSize.y));

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
    if (ImGui::Begin("Side Panel", NULL, flags))
    {
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
                "Example: if a segment is 5000 base pairs long, and this number is set to 1000, the segment size will be 5\n[minimal size is 1]\n---\n"
                "Pro tip: you can hold CTRL while holding the - + buttons to quickly tune the values");
            ImGui::TreePop();
            ImGui::EndDisabled();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
}