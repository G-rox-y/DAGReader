#include "info.hpp"

void info::draw()
{
    if (!channel->info_window_shown.load()) return;
    channel->cam->getPos();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = 6.0f;

    ImVec2 br(vp->WorkPos.x + vp->WorkSize.x - margin, vp->WorkPos.y + vp->WorkSize.y - margin);
    ImGui::SetNextWindowPos(br, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("Lower-Right HUD", nullptr, flags)){
        auto& nums = channel->cam->getPos();
        std::string coords = "X = " + std::to_string(nums.x) + "; Y = " + std::to_string(nums.y) + "; Z = " + std::to_string(nums.z);
        ImGui::Text(coords.c_str());
        ImGui::End();
    }
}