#pragma once

#include <string>

#include "GFA_virtuals.hpp"

class GFA_link : public mappingQuality, public numOfMismatchsGaps, public readCount, public fragmentCount, public kmerCount, public edgeIdentifier{
private:
    std::string from_name, to_name;
    bool from_orientation, to_orientation;
    std::string overlap;

public:
    GFA_link() = default;
};