#include "menuBar.hpp"

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

            ImGui::Separator();

            bool lm = channel->light_mode.load();
            if (ImGui::MenuItem("Light mode", NULL, lm)){
                channel->light_mode.store(!lm);
                channel->update_window_vars.store(true);
                if (!lm) ImGui::StyleColorsLight();
                else ImGui::StyleColorsDark();
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
