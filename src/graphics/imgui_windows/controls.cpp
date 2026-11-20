#include "controls.hpp"

void controls::draw()
{
    if (!channel->controls_window_shown.load()) return;

    static std::string lines[] = {
        "#Movement",
        "Up, Down Left, Right - look around",
        "W, A, S, D, - move",
        "E, Q - rotate",
        "Space, C - up / down",
        "I, O - Increase / decrease speed",
        "LShift(hold) - Speed up movement",
        "R - Reset position",
        "#Selection",
        "X - Toggle selection mode",
        "F - Focus on selection",
        "U - Unselect all selections",
        "#",
        "H - Toggle controls"
    };

    for(auto& l:lines)
        m_width = std::max(m_width, ImGui::CalcTextSize(l.c_str()).x);

    const ImGuiViewport* viewport = ImGui::GetMainViewport(); // get the viewport

    // force the panel to be in the lower left corner
    float margin = 6.f;
    ImVec2 ll(viewport->WorkPos.x + margin, viewport->WorkPos.y + viewport->WorkSize.y - margin);
    ImGui::SetNextWindowPos(ll, ImGuiCond_Always, ImVec2(0.0f, 1.0f));

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    
    if (ImGui::Begin("Controls", NULL, flags)){
        if (channel->controls_window_toggled.load())
            for(auto& l:lines){
                if (!l.empty() && l[0] == '#') ImGui::SeparatorText(l.substr(1).c_str());
                else ImGui::Text("%s", l.c_str());
            }
        else{
            ImGui::SeparatorText("Controls:");
            ImGui::Text("press H to show");
        }
        ImGui::End();
    }
    ImGui::PopStyleVar(2);
}