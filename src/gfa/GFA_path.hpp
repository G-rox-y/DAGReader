#pragma once

#include <string>
#include <vector>

class GFA_path {
private:
    std::string name;
    std::vector<std::string> segments; // list of segment names and orientations
    std::vector<std::string> overlaps; // Optional comma-separated list of CIGAR strings
public:
    GFA_path() = default;
};