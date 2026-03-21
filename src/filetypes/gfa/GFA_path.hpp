#pragma once

#include "pch.hpp"

#include "gfa/GFA_virtuals.hpp"

class GFA_path : public overlap{
private:
    std::string name;
    std::vector<size_t> segments; // list of segment names
    std::vector<bool> orientations; //  their orientations

public:
    GFA_path() = default;
    GFA_path(const std::string& n, std::vector<size_t> segIDs, std::vector<bool> oris)
        : name(n), segments(std::move(segIDs)), orientations(std::move(oris)) {}
};