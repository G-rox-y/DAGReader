#pragma once

#include "pch.hpp"
#include "Graphs.hpp"

using dataProperties = std::variant<bool, char, int, float, double, long long int, size_t, std::string_view>;

class datatype {
public:
    enum mapType {SEGMENT, LINK};

    virtual void fillData(std::vector<Vertex>& v, std::vector<Edge>& e) const = 0;
    virtual std::map<std::string, dataProperties> retrieveEdgeData(size_t id, bool verbose) const = 0;
    virtual std::map<std::string, dataProperties> retrieveGeneralData() const = 0;
};