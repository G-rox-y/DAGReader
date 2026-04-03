#pragma once

#include "pch.hpp"

struct Vertex{
    size_t id;
    glm::dvec3 pos;

    Vertex(size_t _id) : id(_id), pos(glm::dvec3(0.0)) {}
};

struct Edge{
    size_t gid, lid;

    size_t start;
    size_t end;
    
    // these values are to be used only if segpart is falsefilters
    // an orientation bool is true if the segment is connected with +, and false if with -
    bool startOri, endOri, segPart = false;

    long long int length;
    long long int originalLength;

    Edge(size_t ID, size_t v1, size_t v2, long long int v3 = 1) : gid(ID), start(v1), end(v2), originalLength(v3) {}
    void setOrientations(bool s, bool e) { startOri = s; endOri = e; }
};

struct Graph{
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;

    // adjacency list, indexed by vertex ID, contains vectors of pairs
    // each pair has neighbor (pair.first) and the edge that connects them (pair.second)
    std::vector<std::vector<std::pair<size_t, size_t>>> adjList;

    void clear() {
        vertices.clear();
        edges.clear();
        adjList.clear();
        radius = 0.0;
    }
    bool empty() {
        return vertices.empty() || edges.empty();
    }

    double radius = 0.0;
    const glm::dvec3 calculateBarycenter();
    void translate(const glm::dvec3& T);

    // can find the distance between two vertices using BFS, dont use too much due to complexity
    int findDist(const size_t id1, const size_t id2);
};

struct graphCollection {
    std::vector<Graph> graphs;

    void setGraphs(std::vector<Vertex>& v, std::vector<Edge>& e);
    void clear() { graphs.clear(); }
    const bool empty() const { return graphs.empty(); }
};