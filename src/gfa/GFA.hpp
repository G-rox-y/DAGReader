#pragma once

#include "pch.hpp"

#include "GFA_link.hpp"
#include "GFA_segment.hpp"
#include "GFA_containment.hpp"
#include "GFA_path.hpp"
#include "quad.hpp"
#include "line.hpp"

struct GFA_field{
    std::string tag, type, value;
};

class GFA {
private:
    std::string version_string;

    std::vector<GFA_segment> segments;
    std::vector<GFA_link> links;
    std::vector<GFA_containment> containments;
    std::vector<GFA_path> paths;

    std::map<std::string_view, int> segment_lookup;
    std::map<std::pair<std::string_view, std::string_view>, int> link_lookup;
    // TODO: implement a custom hashing function to be able to relpace map with unordered_map

    // this function handles errors coming from GFA class
    void parser_error(const std::string& description, const int line_n) const;
    void parser_error(const std::string& description) const;

public:
    // the constructor of this class parses a GFA file from the path provided
    GFA(const std::string& path);

    void insertGraph(ogdf::Graph& graph, ogdf::GraphAttributes& ga, std::atomic<long long int>& segmentSize, bool calsSize) const;

    const int segmentNum() const { return segments.size(); }
    const int containmentNum() const { return containments.size(); }
    const int linkNum() const { return links.size(); }
    const int pathNum() const { return paths.size(); }
};