#pragma once

#include "pch.hpp"
#include "rapidcsv.h"

class CSV {
public:
    struct NodeData {
        std::unordered_map<std::string, std::string> columns;
    };

private:
    bool m_valid = false;
    std::vector<std::string> headers;
    std::unordered_map<std::string, NodeData> data;

public:
    CSV(const std::string& path);
    ~CSV() = default;

    bool hasNode(const std::string& name) const;
    const NodeData* getNodeData(const std::string& name) const;
    const std::vector<std::string>& getHeaders() const { return headers; }
    bool isValid() const { return m_valid; }
};