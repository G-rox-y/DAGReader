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

    std::string version_string = "1.0"; // if not specified, gfa spec mandates this to be 1.0

    std::vector<GFA_segment> segments;
    std::vector<GFA_link> links;
    std::vector<GFA_containment> containments;
    std::vector<GFA_path> paths;

    std::unordered_map<std::string_view, size_t> segment_lookup;
    std::map<std::pair<size_t, size_t>, size_t> link_lookup;
    // TODO: implement a custom hashing function to be able to relpace map with unordered_map

    // table to be able to retrieve segment/link data from edge ids
    mutable std::unordered_map<size_t, std::pair<size_t, int>> edgeMap;

    void parser_warning(const std::string& description, const int line_n) const override;
    [[noreturn]] void parser_error(const std::string& description, const int line_n) const override;

    // warning marker (class remembers it so it doesnt repeat itself)
    mutable bool hifiasm_warned = false; // if hifiasm is detected the parser will warn

public:
    // the constructor of this class parses a GFA file from the path provided
    GFA(const std::string& path);

    void fillData(std::vector<Vertex>& v, std::vector<Edge>& e) const override;
    std::map<std::string, dataProperties> retrieveEdgeData(size_t id, bool verbose = false) const override;
    std::map<std::string, dataProperties> retrieveGeneralData() const override;
    std::optional<std::tuple<std::filesystem::path, std::streampos>> retrieveSequence(size_t id) const;
    std::optional<std::tuple<std::filesystem::path, std::streampos>> retrieveCIGAR(size_t id) const;
    std::vector<std::string> searchForName(bool seg, bool link, bool cont, bool path);
};