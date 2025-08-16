#pragma once

#include "pch.hpp"

struct Vertex{
    int id;
    glm::vec3 pos;

    Vertex(int _id) : id(_id), pos(glm::vec3(0.f)) {}
};

struct Edge{
    int start;
    int end;
    long long int length;
    bool segPart = false;

    Edge(int v1, int v2, long long int v3 = 1) : start(v1), end(v2), length(v3) {}
    void addLength(int v3) { length = v3; }
    void isSegmentPart(bool is) { segPart = is; }
};

struct Graph{
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;

    void clear() {
        vertices.clear();
        edges.clear();
    }
    bool empty() {
        return vertices.empty() | edges.empty();
    }
};
