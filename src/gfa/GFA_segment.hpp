#pragma once

#include <vector>
#include <string>

#include "GFA_virtuals.hpp"

class GFA_segment : public segmentLength, public readCount, public fragmentCount, public kmerCount, public hash{
private:
    std::string name;
    std::vector<unsigned long long> sequence;

public:
    GFA_segment(const std::string& n);

    void loadSequence(const std::string& uri);
    
};
