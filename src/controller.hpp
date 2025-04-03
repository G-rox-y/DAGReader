#pragma once

#include <nfd.hpp>
#include "pch.hpp"
#include "GFA.hpp"

namespace tasks{
    // tasks that the fileController could have
    enum controllerTask {
        CT_OPEN_NFD, CT_OPEN_PATH,
        CT_EXIT
    };

    extern std::mutex ct_mutex; // mutex used to ensure control over fct variable reads and writes
    extern std::queue<controllerTask> ct; // a variable for the file controller to check if it hasd tasks
    extern std::queue<std::filesystem::path> ct_paths; // saves the paths that the file controller needs to open
    extern std::condition_variable ct_cv; // condition variable that will be used to notify the file controller for work

    void addFileControllerTask(controllerTask task);
    void addFileControllerTaskPath(std::filesystem::path& path);
    void addFileControllerTaskPath(std::string& path);
}

// this class contols file input output and data manipilation
class Controller {
private:
    std::unique_ptr<GFA> graphPtr;
public:
    Controller() = default;
    ~Controller() = default;

    // this function is waiting for a signal from other threads to do something
    // when it doesnt need to do anything it is automatically blocked (waiting but not running)
    void run();

    void getPathNFD(std::filesystem::path& path) const;
};