#pragma once

#include "pch.hpp"
#include "camera.hpp"
#include "renderer.hpp"

namespace tasks{
    // tasks that the fileController could have
    enum controllerTask {
        OPEN_NFD, OPEN_PATH,
        LAYOUT_GRAPH, RESET_GRAPH, REFRESH_GRAPH,
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
        spdlog::debug("Sending a task to the controller");
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

    // variable for controlling imgui windows
    std::atomic<bool> about_window_shown{false};
    std::atomic<bool> controls_window_shown{true};
    std::atomic<bool> sidebar_window_shown{true};
    std::atomic<bool> info_window_shown{true};

    // graph data
private:
    std::string graph_name_string;
    std::mutex graph_name_string_mut;
public:
    void graph_name_set(const std::string& name) {
        std::lock_guard lk(graph_name_string_mut);
        graph_name_string = name;
    }
    const std::string_view graph_name_get() const {
        return std::string_view(graph_name_string);
    };
    std::atomic<int> graph_data_seg_num{0};
    std::atomic<int> graph_data_link_num{0};

    // window variables
    std::atomic<bool> update_window_vars{true};
    std::atomic<bool> light_mode{false};

    // configuration variables for graph drawing
    std::atomic<bool> graph_loaded{false};
    std::atomic<bool> graph_param_change{false};
    // graph appearance variables
    std::atomic<glm::u8vec4> segment_color_packed{glm::u8vec4(255, 50, 180, 175)};
    std::atomic<glm::u8vec4> link_color_packed{glm::u8vec4(230, 190, 80, 255)};
    std::atomic<bool> randomize_segment_colors = {false};
    std::atomic<bool> randomize_link_colors = {false};
    std::atomic<float> link_widths{0.015f};
    std::atomic<float> segment_widths{0.06f};

    // what to hide
private:
    std::unordered_set<int> groupBlacklist;
    std::mutex groupBlacklist_mut;
public:
    void hideGroup(const int id){
        std::lock_guard lk(groupBlacklist_mut);
        groupBlacklist.insert(id);
    }
    void unhideGroup(const int id){
        std::lock_guard lk(groupBlacklist_mut);
        if(groupBlacklist.find(id) != groupBlacklist.end()) [[likely]]
            groupBlacklist.erase(id);
    }
    bool isGroupHidden(const int id){
        std::lock_guard lk(groupBlacklist_mut);
        return (groupBlacklist.find(id) != groupBlacklist.end());
    }
    const size_t numOfHiddenGroups(){
        std::lock_guard lk(groupBlacklist_mut);
        return groupBlacklist.size();
    }

    struct SubgraphData{size_t segment_num, edge_num; };
private:
    std::vector<SubgraphData> subgraphData;
    std::mutex subgraphData_mut;
public:
    const SubgraphData getSubgraphData(const size_t id){
        std::lock_guard lk(subgraphData_mut);
        if (subgraphData.size() <= id)
            throw std::runtime_error("Error: Wrong subgraph data ID");
        return subgraphData.at(id);
    }
    const size_t subgraphAmount(){
        std::lock_guard lk(subgraphData_mut);
        return subgraphData.size();
    }
    void addSubgraphData(const SubgraphData data){
        std::lock_guard lk(subgraphData_mut);
        subgraphData.emplace_back(data);
    }
    void clearSubGraphData(){
        std::lock_guard lk(subgraphData_mut);
        subgraphData.clear();
    }

    // grip variables
    std::atomic<float> grip_scalingFactor{0.05f};
    std::atomic<float> grip_tempGain{0.45f};
    std::atomic<float> grip_tempNarrowGain{1.3f};
    std::atomic<int> grip_roundsNum{16};

    // progress variables
    std::atomic<bool> loading_file_in_progress{false};
    std::atomic<bool> layout_in_progress{false};
};

