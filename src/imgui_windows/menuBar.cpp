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
            if (ImGui::MenuItem("Open recent")){

            }
            if (ImGui::MenuItem("Save", "Ctrl+S")){ // TODO: make this shortcut work

            }
            if (ImGui::MenuItem("Save as")){

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
