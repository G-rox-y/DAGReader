#pragma once

#include "pch.hpp"
#include "dataTypes.hpp"

struct Vertex{
    int id;
    glm::dvec3 pos;

    Vertex(int _id) : id(_id), pos(glm::dvec3(0.0)) {}
};

struct Edge{
    int start;
    int end;
    
    bool startOri = false;
    bool endOri = true;

    long long int length;
    long long int originalLength;
    bool segPart = false;

    Edge(int v1, int v2, long long int v3 = 1) : start(v1), end(v2), originalLength(v3) {}
    void addLength(int v3) { length = v3; }
    void isSegmentPart(bool is) { segPart = is; }
    void setOrientations(bool s, bool e) { startOri = s; endOri = e; }
};

struct Graph{
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;

    // adjacency list, indexed by vertex ID, contains vectors of pairs
    // each pair has neighbor (pair.first) and the edge that connects them (pair.second)
    std::vector<std::vector<std::pair<int, int>>> adjList;

    // graph distances between vertices
    std::unordered_map<stdpp::sorted_pair<int>, int> dists;

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
    int findDist(const int id1, const int id2);
};

struct graphCollection {
    std::vector<Graph> graphs;

    void setGraphs(std::vector<Vertex>& v, std::vector<Edge>& e);
    void clear() { graphs.clear(); }
    const bool empty() const { return graphs.empty(); }
};