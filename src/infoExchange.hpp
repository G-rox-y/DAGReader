#pragma once

#include "pch.hpp"

namespace tasks{
    // tasks that the fileController could have
    enum controllerTask {
        OPEN_NFD, OPEN_PATH,
        LAYOUT_GRAPH,
        EXIT
    };
}

// this struct will be used to exchange information between window and controller threads
// it contains data which can be used by either thread, all data will be either atomic (a_) or just have a mutex (s_)
// this struct will be created on main and its pointer passed to window and controller threads
struct infoExchange
{
    // a queue containing controller tasks, these tasks are waited on with a condition variable
    std::queue<tasks::controllerTask> controller_tasks;
    std::mutex controller_tasks_mut;
    std::condition_variable controller_tasks_cv;
    // a queue containing paths for the controller tasks (some tasks get additional path arguments)
    std::queue<std::filesystem::path> controller_tasks_paths;
    // helper functions for those variables
    void addControllerTask(tasks::controllerTask task){
        spdlog::info("Sending a task to the controller");
        {   std::scoped_lock lk(controller_tasks_mut);
            controller_tasks.push(task);
        }
        controller_tasks_cv.notify_one();
    }
    void addControllerTaskPath(const std::filesystem::path& path){
        std::scoped_lock lk(controller_tasks_mut);
        controller_tasks_paths.push(path);
    }
    void addControllerTaskPath(std::string& path){
        addControllerTaskPath(std::filesystem::path(path));
    }


    // a vector of drawables
    // used by: window (to draw elements to the screen)
    // modified by: controller (changes the elements)
    // owned by: window (only window owns the opengl context so only it can call destructors and other things)
    std::shared_ptr<std::vector<std::unique_ptr<drawable>>> drawables;
    std::mutex drawables_mutex;
    // this bool checks if the drawables had any updates and if they should be redrawn
    std::atomic_bool updated_drawables = false;

    // a vector of recent paths
    // used by: window (menu > open recent)
    // modified by: controller
    std::vector<std::filesystem::path> recent_paths;
    std::mutex recent_paths_mut;
};

