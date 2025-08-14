#include "GFA_path.hpp"

GFA_path::GFA_path(const std::string& n) : name(n) {}

void GFA_path::setSegments(std::ifstream& input)
{
    std::string builtstr;
    for(char c = input.get(); input.good() && c != '\t' && c != '\n'; c = input.get()){
        if (c == ',' || input.peek() == '\t' || input.peek() == '\n'){
            if (input.peek() == '\t' || input.peek() == '\n') builtstr += c;
            segments.emplace_back(builtstr.substr(0, builtstr.size()-1)); // TODO: check if the name is valid against regex
            if (builtstr.back() == '+') orientations.emplace_back(true);
            else if (builtstr.back() == '-') orientations.emplace_back(false);
            else throw std::runtime_error("Invalid orientation \n\tcomma separated name in question: " + builtstr);
            builtstr.clear();
        }
        else builtstr += c;
    }
}

void GFA_path::setOverlaps(std::ifstream& input, const std::vector<GFA_link>& links, const std::map<std::pair<std::string_view, std::string_view>, int> link_lookup)
{
    std::string builtstr;
    for(char c = input.get(); input.good() && c != '\t' && c != '\n'; c = input.get()){
        if (c == ',' || input.peek() == '\t' || input.peek() == '\n'){
            if (input.peek() == '\t' || input.peek() == '\n') builtstr += c;
            if (builtstr != "*") overlaps.push_back(builtstr); // TODO: check against regex
            builtstr.clear();
        }
        else builtstr += c;
    }

    // TODO: check if builtstr is = "*" and if it is, pull overlaps from links using link lookup and segments
}