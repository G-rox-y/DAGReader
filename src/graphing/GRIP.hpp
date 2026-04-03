// the implementation of the GRIP algorithm (Graph dRawing with Intelligent Placement)

#pragma once

#include "pch.hpp"
#include "dataTypes.hpp"
#include "Graphs.hpp"

class GRIP{
private:
    Graph* mr_g; // mr_ = member reference
    
    double m_defaultEdgeLength = 1.0; // what is the worldspace length equivalent of a graph length of 1

    // override for edge lengths of pairs
    // if this map contiains a pair, than this value is ued, if not, the default value is used
    std::unordered_map<stdpp::sorted_pair<int>, int> m_edgeLengths;

    double m_scalingFactor = 0.05; // the scaling factor for the Fruchterman-Reingold computation

    double m_temperatureGain = 0.45; // keep within <0,1>
    double m_temperatureNarrowGainIncrease = 1.3; // keep >1

    int m_rounds_number = 16; // authors of the algorithm suggest within [5,30]

    int m_dimensions = 3;

    // max num of elements for the graph to still be considered small
    int m_smallGraphLimit = 10000;

    mutable std::mt19937 m_rng{std::random_device{}()};

    // finds out if default value should be used for edge_length or some other value
    double find_edge_length(int id1, int id2) const;

    void compute_filters(std::vector<std::vector<int>>& filters) const;

    // computes neighbourhoods and fills in their vector (n) for a given vertex
    void compute_vertex_neighbourhoods(
        int ID, std::vector<std::vector<std::pair<int, int>>>& n, const std::vector<size_t>& nbrs, 
        const std::vector<int>& f_c, const int K, const std::vector<bool>& placed
    ) const;

    // sets the initial position of vertices in the base filter
    void base_filter_placement(const std::vector<int>& base) const;

    // sets the vertex initial position given its neighbourhood
    void vertex_initial_placement(int ID, const std::vector<std::pair<int, int>>& n, const std::vector<bool>& placed) const;

    // updates the temperature with given parameters, updates through updating a reference (cos and temp)
    void calc_temp(double& oldTemp, double& oldCos, const glm::dvec3& oldDisp, const glm::dvec3& force) const;

    // computes the Kamada-Kawai force vector on a vertex by its neighbourhood O(n)
    glm::dvec3 compute_KKforce(int ID, const std::vector<std::pair<int, int>>& n) const;

    // computes the Fruchterman-Reingold force vector on a vertex by its neighbourhood O(n+adj)
    glm::dvec3 compute_FRforce(int ID, const std::vector<std::pair<int, int>>& n) const;

    // just a function for reporting errors
    [[noreturn]] void grip_error(const std::string& description) const;

public:
    GRIP(Graph& G);

    void setTempGain(const double newTemp) { m_temperatureGain = newTemp; }
    void setTempNarrowGainInc(const double newNgain) { m_temperatureNarrowGainIncrease = newNgain; }
    void setRoundsNumber(const int newNum) { m_rounds_number = newNum; }
    void setFRscaling(const double newScale) { m_scalingFactor = newScale; }
    void setDimensions(const int dims) { m_dimensions = dims; }

    void run();
};
