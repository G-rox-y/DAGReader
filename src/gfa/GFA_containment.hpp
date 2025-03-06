#pragma once

#include <string>

#include "GFA_virtuals.hpp"

class GFA_containment : public readCount, public numOfMismatchsGaps, public edgeIdentifier {
private:
    std::string container, contained;
    bool container_orientation, contained_orientation;
    long long int position; // 0-based start of contained segment
    std::string overlap;

public:
    GFA_containment() = default;
};