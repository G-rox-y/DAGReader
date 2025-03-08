#pragma once

#include <fstream>
#include <string>
#include <vector>

class GFA_path {
private:
    std::string name;
    std::vector<std::string> segments; // list of segment names
    std::vector<bool> orientations; //  their orientations
    std::vector<std::string> overlaps; // Optional comma-separated list of CIGAR strings
public:
    GFA_path(const std::string& n);

    void setSegments(std::ifstream& input);
    void setOverlaps(std::ifstream& input);
};