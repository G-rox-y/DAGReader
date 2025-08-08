#include "about.hpp"

#include <string>
#include "cmakevars.hpp"

void about::setOffset(const std::string& text, float scale = 1.f){
    float offset = (ImGui::GetContentRegionAvail().x - scale * ImGui::CalcTextSize(text.c_str()).x) * 0.5f;
    if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
}

void about::draw()
{
    static std::string lines1[] = {
        u8"Developed by Petar Dušević.",
        u8"Special thanks to my mentor Krešimir Križanović, and my friends and family <3",
        "DAGReader is licensed under the zlib License, see LICENSE for more information.",
    };

    static std::string lines2[] = {
        "Hi, my name is Petar and I created this program",
        "The reason for creation was simple, i really like making graphical programs in c++",
        u8"And so, when my mentor Krešimir showed me a program called Bandage while doing some bioinformatics projects",
        "We figured that making a 3D spinoff would be quite cool and i could use it for my bachelors thesis, so I got to work",
        "I gave myself a challenge to make as much of it as reasonably possible from scratch",
        "This took quite a lot of time, effort and thinking, but it was WORTH IT, i love what i created",
        "Anyways, these are my words of encouragement to you, if you have some project in mind, just go for it",
        "Dont overthink it, go with the flow, and be persistent, ly"
    };

    static std::string lines3[] = {
        "If you have any ideas for contributions i am always open for discussion and pull requests on GitHub",
        "If you are interested in some collaboration dont hesitate to contact me (Github, linkedin, whatever)",
        "If you want to support the project in any way, please reach out to me :)"
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