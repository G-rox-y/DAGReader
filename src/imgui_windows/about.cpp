#include "about.hpp"

#include "cmakevars.hpp"

void about::setOffset(const std::string& text, float scale = 1.f){
    float offset = (ImGui::GetContentRegionAvail().x - scale * ImGui::CalcTextSize(text.c_str()).x) * 0.5f;
    if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
}

void about::draw()
{
    static std::string lines1[] = {
        "Developed by Petar Dušević.",
        "Special thanks to Krešimir Križanović for mentorship and support and to Josipa Lipovac for testing and advice",
        "DAGReader is licensed under the zlib License, see LICENSE for more information.",
    };

    static std::string lines2[] = {
        "DAGReader is a 3D graph visualization tool for genome assembly data, "
        "inspired by Bandage and developed as part of a bachelor's thesis.",
        "Built from scratch with OpenGL and ImGui to explore interactive rendering "
        "of large-scale directed acyclic graphs."
    };

    static std::string lines3[] = {
        "If you have any ideas for contributions i am always open for discussion and pull requests on GitHub",
        "If you want to support the project in any way, please reach out to me via GitHub :)"
    };

    bool shown = channel->about_window_shown.load();    
    if (!shown) return;

    if (ImGui::Begin("About DAGReader", &shown, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)){
        std::string title = "DAGReader " DAGR_VERSION_STRING;
        float scale = 1.6f;
        setOffset(title, scale);
        ImGui::SetWindowFontScale(scale);
        ImGui::TextUnformatted(title.c_str());
        ImGui::SetWindowFontScale(1.0f);

        std::string linkText = "Github repository";
        setOffset(linkText);
        ImGui::TextLinkOpenURL(linkText.c_str(), "https://github.com/G-rox-y/DAGReader");

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        for(auto& l:lines1)
            ImGui::TextUnformatted(l.c_str());
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        for(auto& l:lines2)
            ImGui::TextUnformatted(l.c_str());
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        for(auto& l:lines3)
            ImGui::TextUnformatted(l.c_str());

        if (!shown){ // if ordered to close...
            channel->about_window_shown.store(false);
        }

        ImGui::End();
    }
}