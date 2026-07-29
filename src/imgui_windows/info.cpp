#include "info.hpp"
#include "cmakevars.hpp"

void info::draw()
{
    if (!channel->info_window_shown.load()) return;
    channel->cam->getPos();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = 6.0f;

    bool csvAttached = false;
    if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get()))
        csvAttached = gfa->hasAttachedCSV();

    std::string csvText = csvAttached ? "CSV ATTACHED" : "";
    std::string selText = channel->selection_mode.load() ? "SELECTION MODE ACTIVE" : "";
    std::string version = "DAGReader v" DAGR_VERSION_STRING;

    // FPS measurement
    static std::queue<std::chrono::steady_clock::time_point> q;
    q.push(std::chrono::steady_clock::now());
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - q.front()).count() > 1000) q.pop();
    std::string fps = "FPS: " + std::to_string(q.size());

    std::string scale = "Model scale: " + fmt::format("{}", channel->cam->getScale());
    auto nums = channel->cam->getPos();
    std::string posn = fmt::format("X: {}; Y: {}; Z: {}", nums.x, nums.y, nums.z);

    float maxWidth = ImGui::CalcTextSize(version.c_str()).x;
    if (!selText.empty())
        maxWidth = std::max(maxWidth, ImGui::CalcTextSize(selText.c_str()).x);
    if (!csvText.empty())
        maxWidth = std::max(maxWidth, ImGui::CalcTextSize(csvText.c_str()).x);
    maxWidth = std::max(maxWidth, ImGui::CalcTextSize(fps.c_str()).x);
    maxWidth = std::max(maxWidth, ImGui::CalcTextSize(scale.c_str()).x);
    maxWidth = std::max(maxWidth, ImGui::CalcTextSize(posn.c_str()).x);

    float winW = maxWidth + ImGui::GetStyle().WindowPadding.x * 2.f;

    // place and size the window 
    ImVec2 br(vp->WorkPos.x + vp->WorkSize.x - margin,
              vp->WorkPos.y + vp->WorkSize.y - margin);
    ImGui::SetNextWindowPos(br, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(winW, 0), ImGuiCond_Always); // fixed width, auto height

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs;

    if (ImGui::Begin("Lower-Right HUD", nullptr, flags)) {
        float avail = ImGui::GetContentRegionAvail().x;

        if (!selText.empty()) {
            float w = ImGui::CalcTextSize(selText.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
            ImGui::Text("%s", selText.c_str());
        }

        if (!csvText.empty()) {
            float w = ImGui::CalcTextSize(csvText.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
            ImGui::Text("%s", csvText.c_str());
        }

        float w = ImGui::CalcTextSize(version.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
        ImGui::Text("%s", version.c_str());

        w = ImGui::CalcTextSize(fps.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
        ImGui::Text("%s", fps.c_str());

        w = ImGui::CalcTextSize(scale.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
        ImGui::Text("%s", scale.c_str());

        w = ImGui::CalcTextSize(posn.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
        ImGui::Text("%s", posn.c_str());

        ImGui::End();
    }
}