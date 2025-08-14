// the implementation of the GRIP algorithm (Graph dRawing with Intelligent Placement)

#pragma once

#include "pch.hpp"
#include "dataTypes.hpp"
#include "Graphs.hpp"

class GRIP{
private:
    Graph* mr_graph; // mr_ = member reference
    std::vector<std::vector<int>> mr_noDsu_Graphs;

    // graph adjacency list, a map vertex id is paired with a vector filled with vertex ids of its neighbours
    std::unordered_map<int, std::vector<int>> m_adjListG;
    
    float m_defaultEdgeLength = 1.f; // what is the worldspace length equivalent of a graph length of 1

    // override for edge lengths of pairs
    // if this map contiains a pair, than this value is ued, if not, the default value is used
    std::unordered_map<stdpp::sorted_pair<int>, int> m_edgeLengths;

    float m_scalingFactor = 0.05f; // the scaling factor for the Fruchterman-Reingold computation

    float m_temperatureGain = 0.45f; // keep within <0,1>
    float m_temperatureNarrowGainIncrease = 1.3f; // keep >1

    int m_rounds_number = 16; // authors of the algorithm suggest within [5,30]

    int m_dimensions = 3;

    // max num of elements for the graph to still be considered small
    int m_smallGraphLimit = 10000;

    mutable std::mt19937 m_rng{std::random_device{}()};

    // can find the distance between two vertices using BFS
    // worst complexity O(nlogn) best O(logn)
    // guaranteed to be most O(logn) when used from vertex_initial_placement
    // this function should only be ran in the base layer
    float find_dist(int id1, int id2) const;

    // finds out if default value should be used for edge_length or some other value
    float find_edge_length(int id1, int id2) const;

    // computes neighbourhoods and fills in their vector (n) for a given vertex
    void compute_vertex_neighbourhoods(
        int ID, std::vector<std::vector<std::pair<int, int>>>& n, const std::vector<size_t> nbrs, 
        const std::vector<std::unordered_set<int>>& f_c, const int K, const std::unordered_set<int>& placed
    ) const;

    // sets the initial position of vertices in the base filter
    void base_filter_placement(const std::vector<int>& base) const;

    // sets the vertex initial position given its neighbourhood
    void vertex_initial_placement(int ID, const std::vector<std::pair<int, int>>& n, const std::unordered_set<int>& placed) const;

    // updates the temperature with given parameters, updates through updating a reference (cos and temp)
    void calc_temp(float& oldTemp, float& oldCos, const glm::vec3& oldDisp, const glm::vec3& force) const;

    // computes the Kamada-Kawai force vector on a vertex by its neighbourhood O(n)
    glm::vec3 compute_KKforce(int ID, const std::vector<std::pair<int, int>>& n) const;

    // computes the Fruchterman-Reingold force vector on a vertex by its neighbourhood O(n+adj)
    glm::vec3 compute_FRforce(int ID, const std::vector<std::pair<int, int>>& n) const;

    // just a function for reporting errors
    void grip_error(const std::string& description) const;

    // function for running on a non-dsu graph
    void runGraph(int graphId);

public:
    GRIP(Graph& g);

    void setTempGain(const float newTemp) { m_temperatureGain = newTemp; }
    void setTempNarrowGainInc(const float newNgain) { m_temperatureNarrowGainIncrease = newNgain; }
    void setRoundsNumber(const int newNum) { m_rounds_number = newNum; }
    void setFRscaling(const float newScale) { m_scalingFactor = newScale; }
    void setDimensions(const int dims) { m_dimensions = dims; }

    void run();
};
