#include "backgroundWindow.hpp"

void backgroundWindow::drawMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
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

void backgroundWindow::draw(){
    this->drawMenu();

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings 
        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;

    const ImGuiViewport* viewport = ImGui::GetMainViewport(); // get the viewport
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    // Main Area = entire viewport, Work Area = entire viewport minus sections used by the main menu bars, task bars etc.
    // therefore if it said viewport->Pos and viewport->Size, this window would be displayed over the menu bar
    // this way they are perserved

    if (ImGui::Begin("Example: Fullscreen window", NULL, flags))
    {
        ImGui::Text("Hi!");
    }
    ImGui::End();
}