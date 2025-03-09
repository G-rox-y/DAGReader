#pragma once

#include <string>

#include "GFA_virtuals.hpp"

class GFA_link : public mappingQuality, public numOfMismatchsGaps, public readCount, public fragmentCount, public kmerCount, public edgeIdentifier, public overlap{
private:
    std::string from_name, to_name; // segment names
    bool from_orientation, to_orientation; // segment orientations

public:
    GFA_link(const std::string& fn, bool fo, const std::string& tn, bool to)
    : from_name(fn), to_name(tn), from_orientation(fo), to_orientation(to) {};

    const std::string& getFromName() const { return from_name; }
    const std::string& getToName() const { return to_name; }
};