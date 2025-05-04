#pragma once

#include <nfd.hpp>
#include "pch.hpp"
#include "GFA.hpp"
#include "infoExchange.hpp"

// this class contols file input output and data manipulation
class Controller {
private:
    infoExchange* channel; // shared variables between threads

    std::unique_ptr<GFA> graphPtr;

    std::filesystem::path m_binaryPath; // the where the DAGReader binary is located

    inipp::Ini<char> ini; // inipp handler
	std::filesystem::path iniPath; // ini file stream

    // ogdf graph structures
    ogdf::Graph m_graph;
    ogdf::GraphAttributes m_graphAttr;
public:
    Controller(infoExchange* c);
    ~Controller() = default;

    // this function is waiting for a signal from other threads to do something
    // when it doesnt need to do anything it is automatically blocked (waiting but not running)
    void run();

    void getPathNFD(std::filesystem::path& path) const;
};