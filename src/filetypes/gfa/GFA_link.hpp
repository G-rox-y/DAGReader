#pragma once

#include "pch.hpp"

#include "gfa/GFA_virtuals.hpp"

class GFA_link : public mappingQuality, public numOfMismatchsGaps, public readCount, public fragmentCount, 
    public kmerCount, public edgeIdentifier, public overlap, public externalID
{
private:
    size_t fromID, toID; // segment names
    bool from_orientation, to_orientation; // segment orientations

public:
    GFA_link() = default;
    GFA_link(size_t fid, bool fo, size_t tid, bool to)
    : fromID(fid), toID(tid), from_orientation(fo), to_orientation(to) {};

    size_t getFromID() const { return fromID; }
    size_t getToID() const { return toID; }
    char getFromOrientation() const { return (from_orientation) ? '+' : '-'; }
    char getToOrientation() const { return (to_orientation) ? '+' : '-'; }
};