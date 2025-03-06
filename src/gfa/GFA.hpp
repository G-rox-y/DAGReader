#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tuple>

#include "GFA_link.hpp"
#include "GFA_segment.hpp"
#include "GFA_containment.hpp"
#include "GFA_path.hpp"

struct GFA_field{
    std::string tag, type, value;
};

class GFA {
private:
    std::string version_string;

    // this function handles errors coming from GFA class
    void parser_error(const std::string& description, const int line_n) const;
    void parser_error(const std::string& description) const;

public:
    // the constructor of this class parses a GFA file from the path provided
    GFA(const std::string& path);
};