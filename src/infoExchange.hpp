#pragma once

#include "pch.hpp"
#include "camera.hpp"
#include "renderer.hpp"

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


    // the renderer -> thread safe!
    // created by: window (on head)
    // modified by: controller (adds elements through member functions)
    // used by: window (for rendering)
    std::shared_ptr<Renderer> renderer;

    // and the camera (only internal scale variable is thread safe)
    // created by: window (on head)
    // modified by: controller (sets initial zoom) and window
    // used by: window (for rendering)
    std::shared_ptr<Camera> cam;

    // a vector of recent paths
    // used by: window (menu > open recent)
    // modified by: controller
    std::vector<std::filesystem::path> recent_paths;
    std::mutex recent_paths_mut;

    // configuration variables for graph drawing
    std::atomic<bool> graph_loaded{false};
    std::atomic<bool> graph_auto_update{false};
    std::atomic<bool> graph_param_change{false};
    std::atomic<long long int> graph_segment_length{100000};
    std::atomic<bool> graph_auto_determine_segment_length{true};
};

