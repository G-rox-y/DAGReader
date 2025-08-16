#pragma once

#include "pch.hpp"
#include "Graphs.hpp"

class parser {
protected:
    std::string m_filepath;

    virtual void parser_warning(const std::string& description, const int line_n) const = 0;
    void parser_warning(const std::string& description) const { parser_warning(description, -1); };
    virtual void parser_error(const std::string& description, const int line_n) const = 0;
    void parser_error(const std::string& description) const { parser_error(description, -1); };

public:
    // the constructor of this class parses a GFA file from the path provided
    parser(const std::string& path) : m_filepath(path) {}

    virtual void fillGraph(Graph& g) const = 0;
};