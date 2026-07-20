#include "csv/CSV.hpp"

static char detectDelimiter(const std::string& line)
{
    size_t commas     = std::count(line.begin(), line.end(), ',');
    size_t tabs       = std::count(line.begin(), line.end(), '\t');
    size_t semicolons = std::count(line.begin(), line.end(), ';');

    if (tabs > commas && tabs > semicolons) return '\t';
    if (semicolons > commas && semicolons > tabs) return ';';
    return ',';
}

CSV::CSV(const std::string& path)
{
    try {
        // delimiter auto detection from the first line
        char separator = ',';
        {
            std::ifstream sniff(path);
            if (sniff.is_open()) {
                std::string firstLine;
                if (std::getline(sniff, firstLine)) {
                    separator = detectDelimiter(firstLine);
                    spdlog::info("CSV delimiter detected: '{}'",
                        separator == '\t' ? "TAB" :
                        separator == ';' ? "SEMICOLON" : "COMMA");
                }
            }
        }

        rapidcsv::Document doc(
            path,
            rapidcsv::LabelParams(0, -1), // row 0 = headers, no column labels
            rapidcsv::SeparatorParams(separator, true, true) // separator, trim whitespace, handle CR
        );

        headers = doc.GetColumnNames();

        if (headers.empty()) {
            spdlog::warn("CSV file has no headers: {}", path);
            return;
        }

        for (size_t r = 0; r < doc.GetRowCount(); ++r) {
            std::string nodeName = doc.GetCell<std::string>(0, r);

            NodeData nd;
            for (size_t c = 1; c < headers.size(); ++c) {
                std::string val = doc.GetCell<std::string>(c, r);
                nd.columns[headers[c]] = std::move(val);
            }
            data[nodeName] = std::move(nd);
        }

        m_valid = true;
        spdlog::info("CSV loaded: {} nodes, {} data columns", data.size(), headers.size() - 1);
    }
    catch (const std::exception& e) {
        spdlog::warn("Failed to parse CSV '{}': {}", path, e.what());
    }
}

bool CSV::hasNode(const std::string& name) const
{
    return data.find(name) != data.end();
}

const CSV::NodeData* CSV::getNodeData(const std::string& name) const
{
    auto it = data.find(name);
    if (it != data.end()) return &it->second;
    return nullptr;
}

std::optional<std::string> CSV::getNodeColor(const std::string& name) const {
    auto iequals = [](const std::string& a, const std::string& b) -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](char a, char b){ return std::tolower(a) == std::tolower(b); });
    };

    auto it = data.find(name);
    if (it == data.end()) return std::nullopt;
    for (const auto& [col, val] : it->second.columns) {
        if (iequals(col, "Colour") || iequals(col, "Color")) return val;
    }
    return std::nullopt;
}

std::optional<glm::u8vec4> CSV::parseColorString(const std::string& s) {
    std::string trimmed = s;
    size_t start = 0, end = trimmed.size();
    while (start < end && std::isspace(trimmed[start])) ++start;
    while (end > start && std::isspace(trimmed[end - 1])) --end;
    trimmed = trimmed.substr(start, end - start);

    if (trimmed.empty()) return std::nullopt;

    // Hex: #RRGGBB or #RRGGBBAA
    if (trimmed[0] == '#') {
        std::string hex = trimmed.substr(1);
        if (hex.size() == 6 || hex.size() == 8) {
            try {
                auto pair = [](const std::string& str, size_t pos) -> unsigned int {
                    return std::stoul(str.substr(pos, 2), nullptr, 16);
                };
                unsigned int r = pair(hex, 0);
                unsigned int g = pair(hex, 2);
                unsigned int b = pair(hex, 4);
                unsigned int a = 255;
                if (hex.size() == 8) a = pair(hex, 6);
                return glm::u8vec4(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                                   static_cast<uint8_t>(b), static_cast<uint8_t>(a));
            } catch (...) { /* fall through */ }
        }
        return std::nullopt;
    }

    // named colours
    static const std::unordered_map<std::string, glm::u8vec4> namedColors = {
        {"red", {255,0,0,255}}, {"green", {0,128,0,255}}, {"blue", {0,0,255,255}},
        {"yellow", {255,255,0,255}}, {"cyan", {0,255,255,255}}, {"magenta", {255,0,255,255}},
        {"black", {0,0,0,255}}, {"white", {255,255,255,255}}, {"gray", {128,128,128,255}},
        {"grey", {128,128,128,255}}, {"orange", {255,165,0,255}}, {"purple", {128,0,128,255}},
        {"pink", {255,192,203,255}}, {"brown", {165,42,42,255}}, {"lime", {0,255,0,255}},
        {"navy", {0,0,128,255}}, {"teal", {0,128,128,255}}, {"olive", {128,128,0,255}},
        {"maroon", {128,0,0,255}}, {"silver", {192,192,192,255}}, {"gold", {255,215,0,255}},
        {"indigo", {75,0,130,255}}, {"violet", {238,130,238,255}}, {"coral", {255,127,80,255}},
        {"salmon", {250,128,114,255}}, {"khaki", {240,230,140,255}}, {"plum", {221,160,221,255}},
        {"orchid", {218,112,214,255}}, {"tan", {210,180,140,255}}, {"beige", {245,245,220,255}},
        {"mint", {189,252,201,255}}, {"lavender", {230,230,250,255}}, {"crimson", {220,20,60,255}}
    };

    std::string lower;
    lower.reserve(trimmed.size());
    for (char c : trimmed) lower += static_cast<char>(std::tolower(c));

    auto it = namedColors.find(lower);
    if (it != namedColors.end()) return it->second;
    return std::nullopt;
}

std::optional<glm::u8vec4> CSV::getNodeColorParsed(const std::string& name) const {
    auto raw = getNodeColor(name);
    if (!raw) return std::nullopt;
    return parseColorString(*raw);
}