#pragma once

#include "pch.hpp"

#include "gfa/GFA_link.hpp"
#include "gfa/GFA_segment.hpp"
#include "gfa/GFA_containment.hpp"
#include "gfa/GFA_path.hpp"
#include "Graphs.hpp"
#include "parser.hpp"
#include "datatype.hpp"

struct GFA_field{
    std::string tag, type, value;
};

class GFA : public parser, public datatype {
private:
    using parser::parser_warning;
    using parser::parser_error;

    std::string version_string;

    std::vector<GFA_segment> segments;
    std::vector<GFA_link> links;
    std::vector<GFA_containment> containments;
    std::vector<GFA_path> paths;

    std::map<std::pair<std::string_view, std::string_view>, int> link_lookup;
    // TODO: implement a custom hashing function to be able to relpace map with unordered_map

    // table to be able to retrieve segment/link data from edge ids
    mutable std::unordered_map<size_t, std::pair<size_t, int>> edgeMap;

    void parser_warning(const std::string& description, const int line_n) const override;
    [[noreturn]] void parser_error(const std::string& description, const int line_n) const override;

public:
    // the constructor of this class parses a GFA file from the path provided
    GFA(const std::string& path);

    void fillData(std::vector<Vertex>& v, std::vector<Edge>& e) const override;
    std::map<std::string, dataProperties> retrieveEdgeData(size_t id, bool verbose = false) const override;
    std::map<std::string, dataProperties> retrieveGeneralData() const override;
};