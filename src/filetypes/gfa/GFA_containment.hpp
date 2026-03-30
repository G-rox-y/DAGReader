#pragma once

#include "pch.hpp"

#include "gfa/GFA_virtuals.hpp"

class GFA_containment : public readCount, public numOfMismatchsGaps, public edgeIdentifier, public overlap {
private:
    size_t containerID, containedID;
    bool container_orientation, contained_orientation;
    long long int position; // 0-based start of contained segment

public:
    GFA_containment() = default;
    GFA_containment(size_t crid, bool cro, size_t cdid, bool cdo, long long int p)
    : containerID(crid), containedID(cdid), container_orientation(cro),contained_orientation(cdo), position(p) {};

    size_t getContainerID() const { return containerID; }
    size_t getContainedID() const { return containedID; }
    char getContainerOrientation() const { return (container_orientation) ? '+' : '-'; }
    char getContainedOrientation() const { return (contained_orientation) ? '+' : '-'; }
    long long int getPositon() const { return position; }
};