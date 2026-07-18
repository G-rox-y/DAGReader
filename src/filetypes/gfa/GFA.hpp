#pragma once

#include "pch.hpp"

#include "gfa/GFA_link.hpp"
#include "gfa/GFA_segment.hpp"
#include "gfa/GFA_containment.hpp"
#include "gfa/GFA_path.hpp"
#include "Graphs.hpp"
#include "parser.hpp"
#include "datatype.hpp"
#include "csv/CSV.hpp"

struct GFA_field{
    std::string tag, type, value;
};

class GFA : public parser, public datatype {
public:
    enum mapType {SEGMENT=1, LINK=2, CONTAINMENT=4, PATH=8};
private:
    using parser::parser_warning;
    using parser::parser_error;

    std::string version_string = "1.0"; // if not specified, gfa spec mandates this to be 1.0
    
    // the index of an element contained in a vector like this is called the "OBJECT ID"
    std::vector<GFA_segment> segments;
    std::vector<GFA_link> links;
    std::vector<GFA_containment> containments;
    std::vector<GFA_path> paths;

    // csv data to attach to the gfa file
    std::shared_ptr<CSV> m_attachedCSV;

    // name -> OBJECT ID lookup
    std::unordered_map<std::string_view, size_t> segment_lookup, path_lookup;

    // segment object ID -> object ids of links/containments it appears in
    std::vector<std::vector<std::pair<size_t, char>>> segment_associations;

    // table to be able to retrieve segment/link data from edge ids
    // "EDGE ID" is an ID of a visible edge that will be drawn later, these are stored in a Graph class from Graphs.hpp
    // one OBJECT ID may have multiple EDGE IDs
    // Bonus: an edge id is called "External ID" within GFA_virtuals and may be stored in some class vectors above
    mutable std::unordered_map<size_t, std::pair<size_t, char>> edgeMap;

    void parser_warning(const std::string& description, const int line_n) const override;
    [[noreturn]] void parser_error(const std::string& description, const int line_n) const override;

public:
    // the constructor of this class parses a GFA file from the path provided
    GFA(const std::string& path, bool minimalMemory);

    // populate the two provided vectors with graph vertices and edges to be drawn
    void fillData(std::vector<Vertex>& v, std::vector<Edge>& e) override;

    // return a map with edge properties and their values using the object id and object type
    std::map<std::string, dataProperties> retrieveObjectData(size_t oid, char type, bool verbose = false) const;
    
    // same as above, but use edge id
    std::map<std::string, dataProperties> retrieveEdgeData(size_t eid, bool verbose = false) const override;

    // return a map with general graph properties and their values
    std::map<std::string, dataProperties> retrieveGeneralData() const override;

    // retrieve sequence location, if available for a specific edge using the edge id
    std::optional<std::tuple<std::filesystem::path, std::streampos>> retrieveSequence(size_t eid) const;

    // retrieve CIGAR location, if available for a specific edge using the edge id
    std::optional<std::tuple<std::filesystem::path, std::streampos>> retrieveCIGAR(size_t eid) const;

    // retrieve {OBJECT ID, OBJECT TYPE, EDGE ID (if exists)} of all objects with name "nameStr"
    std::vector<std::tuple<size_t, GFA::mapType, std::optional<size_t>>> searchStrictForName(const std::string& nameStr, char filters) const;

    // retrieve {OBJECT ID, OBJECT TYPE, EDGE ID (if exists)} of all objects with name similar to "nameStr" 
    std::vector<std::tuple<size_t, GFA::mapType, std::optional<size_t>>> searchFuzzyForName(const std::string& nameStr, char filters) const;

    // attach additional csv data to the file
    void attachCSV(std::shared_ptr<CSV> csv) { m_attachedCSV = csv; }

    // check if there is a csv attachement
    bool hasAttachedCSV() const { return m_attachedCSV != nullptr; }

    // reach the csv attachement if present
    const CSV* getAttachedCSV() const { return m_attachedCSV.get(); }
};