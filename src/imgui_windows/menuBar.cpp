#include "menuBar.hpp"

void menuBar::draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open file", "Ctrl+O")){ // TODO: make this shortcut work
                tasks::addFileControllerTask(tasks::CT_OPEN_NFD);
            }
            if (ImGui::BeginMenu("Open recent"))
            {
                std::unique_lock lk(tasks::ct_recent_paths_mut, std::try_to_lock);
                if (lk.owns_lock()){
                    if (tasks::ct_recent_paths.empty())
                        ImGui::TextDisabled("No recently opened files");
                    else{
                        // show 5 paths from the back of the vector
                        for(int i = (int)tasks::ct_recent_paths.size() - 1; i >= 0 && (int)tasks::ct_recent_paths.size() - i <= 5; i--){
                            std::string title = tasks::ct_recent_paths[i].filename().string() + "##" + std::to_string(i);
                            if (ImGui::MenuItem(title.c_str())){
                                tasks::addFileControllerTaskPath(tasks::ct_recent_paths[i]);
                                tasks::addFileControllerTask(tasks::CT_OPEN_PATH);
                            }
                        }
                    }
                    lk.unlock();
                }
                else ImGui::TextDisabled("Loading...");
                ImGui::EndMenu();
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
