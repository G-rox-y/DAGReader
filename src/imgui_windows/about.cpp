#include "about.hpp"

#include <string>
#include "cmakevars.hpp"

void about::setOffset(const std::string& text, float scale = 1.f){
    float offset = (ImGui::GetContentRegionAvail().x - scale * ImGui::CalcTextSize(text.c_str()).x) * 0.5f;
    if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
}

void about::draw()
{
    bool shown = channel->about_window_shown.load();    
    if (!shown) return;

    if (ImGui::Begin("About DAGReader", &shown, ImGuiWindowFlags_AlwaysAutoResize)){
        std::string version = DAGR_VERSION_STRING;
        std::string title = "DAGReader " + version;
        float scale = 1.6f;
        setOffset(title, scale);
        ImGui::SetWindowFontScale(scale);
        ImGui::TextUnformatted(title.c_str());
        ImGui::SetWindowFontScale(1.0f);

        std::string linkText = "Github repository";
        setOffset(linkText);
        ImGui::TextLinkOpenURL(linkText.c_str(), "https://github.com/ocornut/imgui");

        
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImGui::TextUnformatted(u8"Developed by Petar Dušević.");
        ImGui::TextUnformatted(u8"Special thanks to my mentor Krešimir Križanović, and my friends and family <3");
        ImGui::Text("DAGReader is licensed under the zlib License, see LICENSE for more information.");

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImGui::Text("Hi, my name is Petar and I created this program");
        ImGui::Text("The reason for creation was simple, i really like making graphical programs in c++");
        ImGui::TextUnformatted(u8"And so, when my mentor Krešimir showed me a program called Bandage while doing some bioinformatics projects");
        ImGui::Text("We figured that making a 3D spinoff would be quite cool and i could use it for my bachelors thesis, so I got to work");
        ImGui::Text("I gave myself a challenge to make as much of it as reasonably possible from scratch");
        ImGui::Text("This took quite a lot of time, effort and thinking, but it was WORTH IT, i love what i created");
        ImGui::Text("Anyways, these are my words of encouragement to you, if you have some project in mind, just go for it");
        ImGui::Text("Dont overthink it, go with the flow, and be persistent, ly");

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImGui::Text("If you have any ideas for contributions i am always open for discussion and pull requests on GitHub");
        ImGui::Text("If you are interested in some collaboration dont hesitate to contact me (Github, linkedin, whatever)");
        ImGui::Text("If you want to support the project in any way, please reach out to me :)");

        if (!shown){ // if ordered to close...
            channel->about_window_shown.store(false);
        }

        ImGui::End();
    }
}