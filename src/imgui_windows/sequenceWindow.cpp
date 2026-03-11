#include "sequenceWindow.hpp"

void sequenceWindow::loadSequence() {
    m_sequence.clear();
    m_error.clear();
    m_sequenceLoaded = false;

    if (seq_file.empty()) {
        m_error = "No file provided";
        return;
    }

    std::ifstream file(seq_file, std::ios::binary);
    if (!file.is_open()) {
        m_error = "Failed to open file";
        return;
    }

    file.seekg(seq_loc_gfa);
    if (!file.good()) {
        m_error = "Failed to seek to position";
        return;
    }

    // Read until whitespace/terminator
    char c;
    while (file.get(c)) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0') {
            break;
        }
        m_sequence += c;
    }

    m_sequenceLoaded = true;
}

void sequenceWindow::setSequence(std::filesystem::path file, std::streampos loc) {
    if (file == seq_file && loc == seq_loc_gfa)
        m_sequenceLoaded = true;
    else {
        seq_file = file;
        seq_loc_gfa = loc;
        m_sequenceLoaded = false;
    }
    m_showWindow = true;
}

void sequenceWindow::draw() {
    if (!m_showWindow) return;

    // Load sequence on first draw after provideSequence()
    if (!m_sequenceLoaded && m_error.empty()) {
        loadSequence();
    }

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Sequence Viewer", &m_showWindow, ImGuiWindowFlags_NoSavedSettings)) {
        if (!m_error.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", m_error.c_str());
        }
        else if (m_sequence.empty()) {
            ImGui::Text("No sequence data");
        }
        else {
            // Info bar
            ImGui::Text("Length: %zu bp", m_sequence.size());
            ImGui::SameLine();
            if (ImGui::Button("Copy to Clipboard")) {
                ImGui::SetClipboardText(m_sequence.c_str());
            }

            ImGui::Separator();

            // Sequence display with wrapping
            static int charsPerLine = 60;
            ImGui::SetNextItemWidth(150);
            ImGui::SliderInt("Chars per line", &charsPerLine, 20, 120);

            ImGui::Separator();

            // Scrollable region for sequence
            ImGui::BeginChild("SequenceScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            
            // Display sequence with line numbers
            for (size_t i = 0; i < m_sequence.size(); i += charsPerLine) {
                size_t lineLen = std::min(static_cast<size_t>(charsPerLine), m_sequence.size() - i);
                
                // Line number
                ImGui::TextDisabled("%7zu  ", i + 1);
                ImGui::SameLine(0, 0);
                
                // Sequence chunk
                ImGui::TextUnformatted(m_sequence.data() + i, m_sequence.data() + i + lineLen);
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();

    // If window was closed via X button
    if (!m_showWindow) {
        m_sequence.clear();
        m_sequenceLoaded = false;
        m_error.clear();
    }
}