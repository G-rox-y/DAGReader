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