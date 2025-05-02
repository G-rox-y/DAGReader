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
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
    if (ImGui::Begin("Side Panel", NULL, flags))
    {
        ImGui::Text("Hi :D");
        if (ImGui::Button("Layout the graph!")){
            if(channel->updated_drawables){
                channel->drawables->clear(); // has to be cleared here cause this thread has the opengl context
                channel->addControllerTask(tasks::LAYOUT_GRAPH);
            }
            else spdlog::info("No changes to draw!");
        }
        if(channel->updated_drawables){
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
    ImGui::End();
    ImGui::PopStyleColor();
}