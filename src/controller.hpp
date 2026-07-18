#pragma once

#include <nfd.hpp>
#include "pch.hpp"
#include "gfa/GFA.hpp"
#include "Graphs.hpp"
#include "GRIP.hpp"
#include "infoExchange.hpp"
#include "datatype.hpp"
#include "csv/CSV.hpp"
#include "rapidcsv.h"

// this class contols file input output and data manipulation
class Controller {
private:
    infoExchange* channel; // shared variables between threads

    std::filesystem::path m_binaryPath; // the where the DAGReader binary is located

    inipp::Ini<char> ini; // inipp handler
	std::filesystem::path iniPath; // ini file stream

    std::shared_ptr<datatype> data;
    graphCollection gc;

    void handleFile(tasks::controllerTask t);
    void handleCSV(tasks::controllerTask t);
    void getPathNFD(std::filesystem::path& path, const char* filterName, const char* filterExt) const;
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