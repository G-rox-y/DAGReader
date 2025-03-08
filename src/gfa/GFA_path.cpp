#include "GFA_path.hpp"

GFA_path::GFA_path(const std::string& n) : name(n) {}

void GFA_path::setSegments(std::ifstream& input)
{
    std::string builtstr;
    for(char c = input.get(); c != '\t'; c = input.get()){
        if (c == ',' || input.peek() == '\t'){
            if (input.peek() == '\t') builtstr += c;
            segments.emplace_back(builtstr.substr(0, builtstr.size()-1)); // TODO: check if the name is valid against regex
            if (builtstr.back() == '+') orientations.emplace_back(true);
            else if (builtstr.back() == '-') orientations.emplace_back(false);
            else throw std::runtime_error("Invalid orientation \n\tcomma separated name in question: " + builtstr);
            builtstr.clear();
        }
        else builtstr += c;
    }
}

void GFA_path::setOverlaps(std::ifstream& input)
{
    std::string builtstr;
    for(char c = input.get(); c != '\t'; c = input.get()){
        if (c == ',' || input.peek() == '\t'){
            if (input.peek() == '\t') builtstr += c;
            if (builtstr == "*") overlaps.emplace_back("");
            else overlaps.push_back(builtstr); // TODO: check against regex
            builtstr.clear();
        }
        else builtstr += c;
    }
}