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
        if (ImGui::BeginMenu("Info"))
        {
            if (ImGui::MenuItem("About DAGReader")){

            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
