#pragma once

#include <nfd.hpp>
#include "pch.hpp"
#include "gfa/GFA.hpp"
#include "Graphs.hpp"
#include "GRIP.hpp"
#include "infoExchange.hpp"

// this class contols file input output and data manipulation
class Controller {
private:
    infoExchange* channel; // shared variables between threads

    std::filesystem::path m_binaryPath; // the where the DAGReader binary is located

    inipp::Ini<char> ini; // inipp handler
	std::filesystem::path iniPath; // ini file stream

    graphCollection gc;

    void handleFile(tasks::controllerTask t);
    void getPathNFD(std::filesystem::path& path) const;
    void layoutGraph();
    void resetGraph();
    void refreshGraph();
public:
    Controller(infoExchange* c);
    ~Controller() = default;

    // this function is waiting for a signal from other threads to do something
    // when it doesnt need to do anything it is automatically blocked (waiting but not running)
    void run();
};