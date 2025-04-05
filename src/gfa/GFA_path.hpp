#pragma once

#include "pch.hpp"

#include "GFA_link.hpp"

class GFA_path {
private:
    std::string name;
    std::vector<std::string> segments; // list of segment names
    std::vector<bool> orientations; //  their orientations
    std::vector<std::string> overlaps; // Optional comma-separated list of CIGAR strings
public:
    GFA_path(const std::string& n);

    void setSegments(std::ifstream& input);

    // setOverlaps must not be run before setSegments had been ran
    void setOverlaps(std::ifstream& input, const std::vector<GFA_link>& links, const std::map<std::pair<std::string_view, std::string_view>, int> link_lookup);
};