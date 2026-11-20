#pragma once

#include "pch.hpp"

struct Vertex{
    size_t id;
    glm::dvec3 pos;

    Vertex(size_t _id) : id(_id), pos(glm::dvec3(0.0)) {}
    Vertex(size_t _id, glm::dvec3 _pos) : id(_id), pos(_pos) {}
};

struct Edge{
    size_t gid, lid; // global id is unique to every edge, and local id is unique to every edge in a subgraph

    size_t start;
    size_t end;

    long long int originalLength;
    int length;

    int sid = -1; // subdivision id (if required)
    size_t parent_lid = std::numeric_limits<size_t>::max(); // subdivision origin (lid stored for easier retrieval)

    // these values are to be used only if segpart is falsefilters
    // an orientation bool is true if the segment is connected with +, and false if with -
    bool startOri, endOri, segPart = false;

    Edge(size_t GID, size_t v1, size_t v2, long long int v3 = 1) : gid(GID), start(v1), end(v2), originalLength(v3) {}
    void setOrientations(bool s, bool e) { startOri = s; endOri = e; }
};

struct Graph{
    std::vector<Vertex> vertices; // id of vertices set such that ID = index of place in the vector
    std::vector<Edge> edges; // lid of edges set such that LID = index of place in vector

    // adjacency list, indexed by vertex ID, contains vectors of pairs
    // each pair has neighbor (pair.first) and the edge that connects them (pair.second)
    std::vector<std::vector<std::pair<size_t, size_t>>> adjList;

    // edge control point storage, indexed by LID, first is p1 and second is p2 (p0 and p3 are at start and end pos)
    std::vector<std::pair<glm::dvec3, glm::dvec3>> ctpts;

    void clear() {
        vertices.clear();
        edges.clear();
        adjList.clear();
        radius = 0.0;
        ctpts.clear();
    }
    bool empty() {
        return vertices.empty() || edges.empty();
    }

    double radius = 0.0;
    const glm::dvec3 calculateBarycenter();
    void translate(const glm::dvec3& T);

    // can find the distance between two vertices using BFS, dont use too much due to complexity
    int findDist(const size_t id1, const size_t id2);

    void fill_adjacency_list();
};

struct graphCollection {
    std::vector<Graph> graphs;

    void setGraphs(std::vector<Vertex>& v, std::vector<Edge>& e);
    void clear() { graphs.clear(); }
    const bool empty() const { return graphs.empty(); }
};

Graph subdivide_graph(Graph& G, int minSubDiv, int maxSubDiv);

// this simplification relies on the coordinate positions in the graph, a layout is required before using it
Graph simplify_graph(Graph& G, Graph& S, float prune_tolerance, float bezier_tolerance);