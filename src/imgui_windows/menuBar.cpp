#include "menuBar.hpp"
#include "infoExchange.hpp"
#include <imgui.h>

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

        if (ImGui::BeginMenu("Info"))
        {
            bool aws = channel->about_window_shown.load();
            if (ImGui::MenuItem("About DAGReader", NULL, aws))
                channel->about_window_shown.store(!aws);
            
            if (ImGui::MenuItem("How to use")){
                
            }
            if (ImGui::MenuItem("Dependencies")){

            }
            if (ImGui::MenuItem("License")){
                
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
