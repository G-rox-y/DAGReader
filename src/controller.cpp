#include "controller.hpp"

#include <iostream>
namespace fs = std::filesystem;

std::mutex tasks::ct_mutex;
std::queue<tasks::controllerTask> tasks::ct;
std::queue<std::filesystem::path> tasks::ct_paths;
std::condition_variable tasks::ct_cv;

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
            getPathNFD();
        }
        
        else if (t == tasks::CT_EXIT){
            spdlog::info("Task: EXIT");
            shouldExit = true;
        }
    }
}

std::string Controller::getPathNFD()
{
    NFD_Init();

    nfdu8char_t *outPath;
    nfdu8filteritem_t filters[1] = { { "GFA", "gfa" } };
    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = 1;
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY){
        spdlog::info("NFD path fetched: {}", outPath);
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
        spdlog::info("NFD cancelled");
    else 
        spdlog::error("NFD Error: {}", NFD_GetError());

    NFD_Quit();
    return "";
}