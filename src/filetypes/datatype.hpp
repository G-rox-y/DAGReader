#pragma once

#include "pch.hpp"
#include "Graphs.hpp"
#include <string_view>

using dataProperties = std::variant<bool, char, int, float, double, long long int, size_t, std::string_view>;

class datatype {
public:
    virtual void fillData(std::vector<Vertex>& v, std::vector<Edge>& e) = 0;
    virtual std::map<std::string, dataProperties> retrieveEdgeData(size_t eid, bool verbose) const = 0;
    virtual std::map<std::string, dataProperties> retrieveGeneralData() const = 0;
};