#include "controls.hpp"

void controls::draw()
{
    if (!channel->controls_window_shown.load()) return;

    static std::string lines[] = {
        "Up, Down Left, Right - Look around",
        "W, A, S, D, - Move",
        "E, Q - Rotate",
        "Space - Move up",
        "C - Move Down",
        "LShift(hold) - Speed up movement",
        "R - Reset view",
        "H - Hide/Show this window"
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

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.4f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (ImGui::Begin("Controls", NULL, flags)){
        for(auto& l:lines) ImGui::Text(l.c_str());
        ImGui::End();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}