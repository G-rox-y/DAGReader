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
    std::string m_sourcePath;
    std::vector<std::string> headers;
    std::unordered_map<std::string, NodeData> data;

public:
    CSV(const std::string& path);
    ~CSV() = default;

    bool hasNode(const std::string& name) const;
    const NodeData* getNodeData(const std::string& name) const;
    const std::vector<std::string>& getHeaders() const { return headers; }
    bool isValid() const { return m_valid; }

    // path this CSV was loaded from
    const std::string& getPath() const { return m_sourcePath; }

    // Case-insensitive lookup for "Colour" or "Color" column
    std::optional<std::string> getNodeColor(const std::string& name) const;

    // Parse the raw string into RGBA (hex #[AA]RRGGBB or named colors)
    static std::optional<glm::u8vec4> parseColorString(const std::string& s);

    // Convenience: look up + parse in one call
    std::optional<glm::u8vec4> getNodeColorParsed(const std::string& name) const;

    // Returns a vector/string_view span of all node names the CSV knows about.
    std::vector<std::string> getNodeNames() const;
};