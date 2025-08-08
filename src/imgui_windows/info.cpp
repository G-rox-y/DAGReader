#include "info.hpp"
#include "cmakevars.hpp"

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

        std::string version = "DAGReader v" DAGR_VERSION_STRING;
        float avail = ImGui::GetContentRegionAvail().x;
        float width = ImGui::CalcTextSize(version.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - width));
        ImGui::SetNextItemWidth(width);
        ImGui::Text(version.c_str());
        
        ImGui::Text("X = %f; Y = %f; Z = %f", nums.x, nums.y, nums.z);
        ImGui::End();
    }
}