#include "controller.hpp"

#include <iostream>
namespace fs = std::filesystem;

std::mutex tasks::ct_mutex;
std::queue<tasks::controllerTask> tasks::ct;
std::queue<std::filesystem::path> tasks::ct_paths;
std::condition_variable tasks::ct_cv;

void tasks::addFileControllerTask(controllerTask task){
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
        std::unique_lock lk(tasks::ct_mutex);
        tasks::ct_cv.wait(lk, [&](){ return !tasks::ct.empty(); });
        auto t = tasks::ct.front();
        tasks::ct.pop();
        lk.unlock();

        if (t == tasks::CT_OPEN_NFD) getPathNFD();
        
        else if (t == tasks::CT_EXIT) shouldExit = true;
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
    if (result == NFD_OKAY)
    {
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
    {
    }
    else 
    {
        printf("Error: %s\n", NFD_GetError());
    }

    NFD_Quit();
    return "";
}