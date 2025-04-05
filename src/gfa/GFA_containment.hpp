#pragma once

#include "pch.hpp"

#include "GFA_virtuals.hpp"

class GFA_containment : public readCount, public numOfMismatchsGaps, public edgeIdentifier, public overlap {
private:
    std::string container, contained;
    bool container_orientation, contained_orientation;
    long long int position; // 0-based start of contained segment

public:
    GFA_containment(const std::string& cr, bool cro, const std::string& cd, bool cdo, long long int p)
    : container(cr), contained(cd), container_orientation(cro),contained_orientation(cdo), position(p) {};

};