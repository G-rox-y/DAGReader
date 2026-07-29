#include "menuBar.hpp"
#include "infoExchange.hpp"

menuBar::menuBar(infoExchange* c) : channel(c) {}

void menuBar::draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open file", "Ctrl+O")){ // TODO: make this shortcut work
                channel->addControllerTask(tasks::OPEN_NFD);
            }
            if (ImGui::BeginMenu("Open recent"))
            {
                std::unique_lock lk(channel->recent_paths_mut, std::try_to_lock);
                if (lk.owns_lock()){
                    if (channel->recent_paths.empty())
                        ImGui::TextDisabled("No recently opened files");
                    else{
                        // show 5 paths from the back of the vector
                        for(int i = (int)channel->recent_paths.size() - 1; i >= 0 && (int)channel->recent_paths.size() - i <= 5; i--){
                            std::string title = channel->recent_paths[i].filename().string() + "##" + std::to_string(i);
                            if (ImGui::MenuItem(title.c_str())){
                                channel->addControllerTaskPath(channel->recent_paths[i]);
                                channel->addControllerTask(tasks::OPEN_PATH);
                            }
                        }
                    }
                    lk.unlock();
                }
                else ImGui::TextDisabled("Loading...");
                ImGui::EndMenu();
            }
            ImGui::Separator();

            bool csvAttached = false;
            if (auto* gfa = dynamic_cast<GFA*>(channel->file_data.get()))
                csvAttached = gfa->hasAttachedCSV();

            if (channel->graph_loaded.load()) {
                if (ImGui::MenuItem("Load CSV labels"))
                    channel->addControllerTask(tasks::OPEN_CSV_NFD);
                if (csvAttached) {
                    if (ImGui::MenuItem("Unload CSV labels")){
                        channel->addControllerTask(tasks::UNLOAD_CSV);
                        channel->addControllerTask(tasks::REFRESH_GRAPH);
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem("Unload CSV labels");
                    ImGui::EndDisabled();
                }
            }
            else {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Load CSV labels");
                ImGui::MenuItem("Unload CSV labels");
                ImGui::EndDisabled();
            }

            // --- Export custom colors ---
            bool hasCustomColors = false;
            {
                std::lock_guard lk(channel->color_storage_mut);
                hasCustomColors = !channel->color_storage.empty();
            }

            if (channel->graph_loaded.load() && hasCustomColors) {
                if (ImGui::MenuItem("Export custom colors to CSV...")) {
                    if (csvAttached) m_pendingExportModal = true;
                    else channel->addControllerTask(tasks::EXPORT_CSV_NEW);
                }
            }
            else {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Export custom colors to CSV...");
                ImGui::EndDisabled();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Clear layout")){
                channel->renderer->clearAll();
                channel->graph_loaded.store(false);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            bool sws = channel->sidebar_window_shown.load();
            if (ImGui::MenuItem("Sidebar", NULL, sws))
                channel->sidebar_window_shown.store(!sws);

            bool cws = channel->controls_window_shown.load();
            if (ImGui::MenuItem("Controls", NULL, cws))
                channel->controls_window_shown.store(!cws);

            bool iws = channel->info_window_shown.load();
            if (ImGui::MenuItem("Position info", NULL, iws))
                channel->info_window_shown.store(!iws);

            bool swa = channel->selection_window_allowed.load();
            if (ImGui::MenuItem("Selection window", NULL, swa))
                channel->selection_window_allowed.store(!swa);

            ImGui::Separator();

            bool lm = channel->light_mode.load();
            if (ImGui::MenuItem("Light mode", NULL, lm) && !lm){
                channel->light_mode.store(!lm);
                channel->update_window_vars.store(true);
                ImGui::StyleColorsLight();
            }
            if (ImGui::MenuItem("Dark mode", NULL, !lm) && lm){
                channel->light_mode.store(!lm);
                channel->update_window_vars.store(true);
                ImGui::StyleColorsDark();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Orbit selection")){
                if (auto res = channel->renderer->getGroupOrbitData(groups::SELECTION)){
                    auto [centerPos, maxDist] = res.value();
                    channel->cam->engageOrbit(centerPos, maxDist * 2.5f);
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Performance"))
        {
            int m_fps = channel->max_fps.load();
            if (ImGui::SliderInt("MAX FPS", &m_fps, 3, 300))
                channel->max_fps.store(m_fps);

            bool minimalLoad = channel->minimal_memory_load.load();
            if (ImGui::Checkbox("Minimal Memory Load", &minimalLoad))
                channel->minimal_memory_load.store(minimalLoad);
            ImGui::SameLine();
            HelpMarker("Skips loading of some potentially memorywise large graph information, "
                    "this may help if the graph is too large to fit in RAM");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Info"))
        {
            bool aws = channel->about_window_shown.load();
            if (ImGui::MenuItem("About DAGReader", NULL, aws))
                channel->about_window_shown.store(!aws);
            
            if (ImGui::MenuItem("How to use")){
                
            }
            if (ImGui::MenuItem("Dependencies")){

            }

            bool lws = channel->license_window_shown.load();
            if (ImGui::MenuItem("License", NULL, lws))
                channel->license_window_shown.store(!lws);
            
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

    }

    if (m_pendingExportModal){
        ImGui::OpenPopup("Export Colors CSV");
        m_pendingExportModal = false;
    }

    // ---- Modal: overwrite existing CSV or create new? ----
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Export Colors CSV", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("A CSV file is already loaded.");
        ImGui::Text("Would you like to export to the existing file or create a new one?");
        ImGui::TextColored(ImVec4(0.9f, 0.25f, 0.2f, 1.0f), "Warning: Choosing \"Existing file\" will modify the original CSV.");
        ImGui::Separator();

        if (ImGui::Button("Existing file", ImVec2(120, 0))) {
            channel->addControllerTask(tasks::EXPORT_CSV_OVERWRITE);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("New file", ImVec2(120, 0))) {
            channel->addControllerTask(tasks::EXPORT_CSV_NEW);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
