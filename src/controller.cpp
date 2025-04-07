#include "controller.hpp"

#include <iostream>
namespace fs = std::filesystem;

std::mutex tasks::ct_mutex;
std::queue<tasks::controllerTask> tasks::ct;
std::queue<std::filesystem::path> tasks::ct_paths;
std::condition_variable tasks::ct_cv;
std::shared_ptr<std::vector<std::unique_ptr<drawable>>> tasks::ct_ptr = nullptr;
std::shared_ptr<std::mutex> tasks::ct_ptr_mut;

void tasks::addFileControllerTask(controllerTask task){
    spdlog::info("Sending a task to the controller");
    {
        std::scoped_lock lk(ct_mutex);
        ct.push(task);
    }
    ct_cv.notify_one();
}
void tasks::addFileControllerTaskPath(std::filesystem::path& path){
    std::scoped_lock lk(ct_mutex);
    ct_paths.push(path);
}
void tasks::addFileControllerTaskPath(std::string& path){
    std::scoped_lock lk(ct_mutex);
    ct_paths.emplace(std::filesystem::path(path));
}
void tasks::addFileControllerDrawables(const std::shared_ptr<std::vector<std::unique_ptr<drawable>>>& ptr, const std::shared_ptr<std::mutex>& mut){
    std::scoped_lock lk(ct_mutex);
    ct_ptr = ptr;
    ct_ptr_mut = mut;
}

void Controller::run()
{
    bool shouldExit = false;
    while(!shouldExit){
        spdlog::info("Controller waiting for a task...");
        std::unique_lock lk(tasks::ct_mutex);
        tasks::ct_cv.wait(lk, [&](){ return !tasks::ct.empty(); });
        auto t = tasks::ct.front();
        tasks::ct.pop();
        lk.unlock();

        spdlog::info("Controller recieved a task");

        if (t == tasks::CT_OPEN_NFD){
            spdlog::info("Task: OPEN_NFD");
            fs::path path;
            getPathNFD(path);
            if (path.empty()) spdlog::info("NFD returned an empty path");
            if (!fs::exists(path)) spdlog::warn("NFD returned a path that doesnt exist!");
            else{
                spdlog::info("Running the parser on the file");
                graphPtr = std::make_unique<GFA>(path.string());
            }
        }
        else if (t == tasks::CT_LAYOUT_GRAPH){
            spdlog::info("Task: LAYOUT_GRAPH");
            if (graphPtr){
                spdlog::info("Laying out a graph");
                graphPtr.get()->computeGraph();
                if (s_drawables){
                    spdlog::info("Inserting the data into the shared datastructure");
                    std::unique_lock<std::mutex> lk(*s_drawables_mutex);
                    s_drawables->clear();
                    graphPtr.get()->insertGraph(s_drawables);
                }
                else spdlog::warn("Shared datastrure pointer is not defined");
            }
            else spdlog::warn("No graph found!");
        }
        else if (t == tasks::CT_SET_DRAWABLES){
            spdlog::info("Task: Set Drawables (shared datastructure)");
            lk.lock();
            s_drawables = tasks::ct_ptr;
            s_drawables_mutex = tasks::ct_ptr_mut;
            lk.unlock();
            if (s_drawables) spdlog::info("Shared dastructure pointer set");
            else spdlog::warn("Shared datastructure pointer not specified!");
        }
        else if (t == tasks::CT_EXIT){
            spdlog::info("Task: EXIT");
            shouldExit = true;
        }
    }
}

void Controller::getPathNFD(std::filesystem::path& path) const
{
    // TODO: nfd init can throw an error, you should catch it
    NFD_Init();

    nfdu8char_t *outPath;
    nfdu8filteritem_t filters[1] = { { "GFA", "gfa" } };
    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = 1;
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY){
        spdlog::info("NFD path fetched: {}", outPath);
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
        spdlog::info("NFD cancelled");
    else 
        spdlog::error("NFD Error: {}", NFD_GetError());

    NFD_Quit();
}