#pragma once

#include "pch.hpp"
#include "camera.hpp"
#include "renderer.hpp"
#include "datatype.hpp"

namespace tasks{
    // tasks that the fileController could have
    enum controllerTask {
        OPEN_NFD, OPEN_PATH,
        OPEN_CSV_NFD, OPEN_CSV_PATH, UNLOAD_CSV,
        EXPORT_CSV_NEW, EXPORT_CSV_OVERWRITE,
        LAYOUT_GRAPH, RESET_GRAPH, REFRESH_GRAPH,
        EXIT
    };
}

namespace groups{
    enum rendererGroup {
        SELECTION = -3,
        LINK = -2,
        SEGMENT = -1
    };
}

// this struct will be used to exchange information between window and controller threads
// it contains data which can be used by either thread, all data will be either atomic (a_) or just have a mutex (s_)
// this struct will be created on main and its pointer passed to window and controller threads
struct infoExchange
{
    // controller task handling
    // remember that the functions below will be run from different threads
    // for example the controller thread will be using "waitForTask" while the window thread will be using functions to add tasks
private:
    // a queue containing controller tasks, these tasks are waited on with a condition variable
    std::queue<tasks::controllerTask> controller_tasks;
    std::queue<std::filesystem::path> controller_tasks_paths;
    std::mutex controller_tasks_mut;
public:
    std::condition_variable controller_tasks_cv;
    // a queue containing paths for the controller tasks (some tasks get additional path arguments)
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
    std::string getControllerTaskPath(){
        std::scoped_lock lk(controller_tasks_mut);
        std::string ret = controller_tasks_paths.front().string();
        controller_tasks_paths.pop();
        return ret;
    }
    tasks::controllerTask waitForTask(){
        std::unique_lock lk(controller_tasks_mut);
        controller_tasks_cv.wait(lk, [&](){ return !controller_tasks.empty(); });
        auto t = controller_tasks.front();
        controller_tasks.pop();
        return t;
    }

    // maps color to element ids with that color (color compressed into uint32_t cause unordered_map cant hash glm::u8vec4)
    // also has a mutex below that has to be used with it
    // keep the vector sorted so its easier to work with
    std::unordered_map<uint32_t, std::vector<size_t>> color_storage;
    std::mutex color_storage_mut;
    // this maps the segment id to the color it currently has assigned (if any)
    // also has to be protected by the color storage mutex
    // we need this to help enforcing each segment to have only one color active
    std::unordered_map<size_t, uint32_t> segment_color_map;
    // should the custom colors be shown
    std::atomic<bool> show_custom_colors{true};

    // maps CSV-derived color to element ids with that color
    // again, should be sorted, and has a mutex that has to be used with it
    std::unordered_map<uint32_t, std::vector<size_t>> csv_color_storage;
    // again, maps the segment id to csv color, should be used with a mutex
    std::unordered_map<size_t, uint32_t> csv_segment_color_map;
    std::mutex csv_color_storage_mut;
    // should the csv colors be shown
    std::atomic<bool> show_csv_colors{true};

    // the renderer -> thread safe!
    // created by: window (on head)
    // modified by: controller (adds elements through member functions)
    // used by: window (for rendering)
    std::unique_ptr<Renderer> renderer;

    // and the camera (only internal scale variable is thread safe)
    // created by: window (on head)
    // modified by: controller (sets initial zoom) and window
    // used by: window (for rendering)
    std::unique_ptr<Camera> cam;

    // a vector of recent paths
    // used by: window (menu > open recent)
    // modified by: controller
    std::vector<std::filesystem::path> recent_paths;
    std::mutex recent_paths_mut;

    // variable for controlling imgui windows
    std::atomic<bool> about_window_shown{false};
    std::atomic<bool> controls_window_shown{true}; // if the window is visible
    std::atomic<bool> controls_window_toggled{false}; // if help is printed within the window
    std::atomic<bool> sidebar_window_shown{true};
    std::atomic<bool> info_window_shown{true};
    std::atomic<bool> selection_window_allowed{true};
    std::atomic<bool> license_window_shown{false};
    
    // variable for controllng loading parameters
    std::atomic<bool> minimal_memory_load{false};

    // MSAA control
    std::atomic<bool> MSAA_enabled{true};

    // fps performance
    std::atomic<int> max_fps{90};

    // graph data
private:
    std::string graph_name_string;
    mutable std::mutex graph_name_string_mut;
public:
    void graph_name_set(const std::string& name) {
        std::lock_guard lk(graph_name_string_mut);
        graph_name_string = name;
    }
    std::string_view graph_name_get() const {
        std::lock_guard lk(graph_name_string_mut);
        return std::string_view(graph_name_string);
    };

    // data of the opened file 
    std::shared_ptr<datatype> file_data;

    // window variables
    std::atomic<bool> update_window_vars{true};
    std::atomic<bool> light_mode{false};

    // configuration variables for graph drawing
    std::atomic<bool> graph_loaded{false};
    std::atomic<bool> graph_param_change{false};

    // graph appearance variables
    std::atomic<glm::u8vec4> segment_color_packed{glm::u8vec4(255, 50, 180, 175)};
    std::atomic<glm::u8vec4> link_color_packed{glm::u8vec4(230, 190, 80, 255)};
    std::atomic<glm::u8vec4> selected_color_packed{glm::u8vec4(140, 180, 220, 230)};
    std::atomic<glm::u8vec4> err_color{glm::u8vec4(255, 0, 0, 255)};

    enum colScheme { NONE, RANDOM, DEPTH, LENGTH };
    enum colRule {NORMAL, SQRT, CBRT, PROGRESSIVE};
    std::atomic<colScheme> segment_color_scheme{NONE};
    std::atomic<colRule> segment_color_rule{NORMAL};
    std::atomic<colScheme> link_color_scheme{NONE};
    
    std::atomic<float> link_widths{0.015f};
    std::atomic<float> segment_widths{0.06f};

    std::atomic<bool> selection_mode{false};

    // what to hide
private:
    std::unordered_set<int> groupBlacklist;
    mutable std::mutex groupBlacklist_mut;
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
    bool isGroupHidden(const int id) const {
        std::lock_guard lk(groupBlacklist_mut);
        return (groupBlacklist.find(id) != groupBlacklist.end());
    }
    size_t numOfHiddenGroups() const {
        std::lock_guard lk(groupBlacklist_mut);
        return groupBlacklist.size();
    }

    // subgraph data, and data control
    struct SubgraphData{
        size_t segment_num, edge_num;
        glm::vec3 pos;
        float radius;
        bool selected = false;
    };
private:
    std::vector<SubgraphData> subgraphData;
    mutable std::mutex subgraphData_mut;
public:
    const SubgraphData& getSubgraphData(const size_t id) const {
        std::lock_guard lk(subgraphData_mut);
        if (subgraphData.size() <= id)
            throw std::runtime_error("Error: Wrong subgraph data ID");
        return subgraphData.at(id);
    }
    size_t subgraphAmount() const {
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
    void selectSubGraph(const size_t id){
        std::lock_guard lk(subgraphData_mut);
        if (subgraphData.size() <= id)
            throw std::runtime_error("Error: Wrong subgraph data ID");
        subgraphData.at(id).selected = true;
        renderer->addGroupXToY(id, groups::SELECTION);
    }
    void unselectSubGraph(const size_t id){
        std::lock_guard lk(subgraphData_mut);
        if (subgraphData.size() <= id)
            throw std::runtime_error("Error: Wrong subgraph data ID");
        subgraphData.at(id).selected = false;
        renderer->removeGroupXFromY(id, groups::SELECTION);
    }
    void setSubGraphPosition(const size_t id, const glm::vec3& pos){
        std::lock_guard lk(subgraphData_mut);
        if (subgraphData.size() <= id)
            throw std::runtime_error("Error: Wrong subgraph data ID");
        subgraphData.at(id).pos = pos;
    }
    void setSubGraphRadius(const size_t id, const float rad){
        std::lock_guard lk(subgraphData_mut);
        if (subgraphData.size() <= id)
            throw std::runtime_error("Error: Wrong subgraph data ID");
        subgraphData.at(id).radius = rad;
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
