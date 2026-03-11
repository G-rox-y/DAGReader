#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"

class sequenceWindow : public imguiWindow{
private:
    infoExchange* channel;

    std::filesystem::path seq_file; // sequence file
    std::streampos seq_loc_gfa; // stream position inside the gfa where the sequence can be found

    // Cached sequence data
    std::string m_sequence;
    bool m_sequenceLoaded = false;
    bool m_showWindow = false;
    std::string m_error;

    void loadSequence();

public:
    sequenceWindow(infoExchange* c) : channel(c) {}

    void setSequence(std::filesystem::path file, std::streampos loc);

    void draw() override;
};