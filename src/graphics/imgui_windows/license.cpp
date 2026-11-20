#include "license.hpp"

void license::draw()
{

    bool shown = channel->license_window_shown.load();
    if (!shown) return;


    ImGui::SetNextWindowSize(ImVec2(489, 320), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("License", &shown, ImGuiWindowFlags_NoCollapse))
    {
        auto setOffset = [](const char* text, float scale = 1.0f) {
            float offset = (ImGui::GetContentRegionAvail().x - scale * ImGui::CalcTextSize(text).x) * 0.5f;
            if (offset > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
        };

        // Centered title (same style as DAGReader version string)
        const char* title = "License";
        float titleScale = 1.6f;
        setOffset(title, titleScale);
        ImGui::SetWindowFontScale(titleScale);
        ImGui::TextUnformatted(title);
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing(); 
        ImGui::Separator(); 
        ImGui::Spacing();

        // Scrollable wrapped text area
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (ImGui::BeginChild("LicenseText", avail, false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(licenseText ? licenseText : "");
            ImGui::PopTextWrapPos();
        }
        ImGui::EndChild();

        if (!shown) channel->license_window_shown.store(shown);
    }
    ImGui::End();
}