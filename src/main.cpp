#include "pch.hpp"
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "window.hpp"
#include "controller.hpp"

int main()
{
    // file logger init, this can, in theory, create an exception
    spdlog::set_pattern("[%Y-%b-%d %T.%e] [%t] [%^%l%$] %v");
#ifdef DEBUG_LOGGING
    spdlog::info("DEBUG_LOGGING detected, logging set to synchronous mode");
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_default_logger(spdlog::stdout_color_mt<spdlog::async_factory>("async_console"));
    spdlog::set_level(spdlog::level::info);
    spdlog::info("logging set to async mode");
#endif
    spdlog::info("Starting the program!");

    // TODO: Add some .ini file to read starting dimensions from, and save them to
    int window_w = 1080, window_h = 920;

    std::thread t_window([window_w, window_h](){
        spdlog::info("Window thread started");
        Window window(window_w, window_h);
        window.run();
        spdlog::info("Window thread exiting");
    }); // run the window in a thread

    std::thread t_controller([](){
        spdlog::info("Controller thread started");
        Controller controller;
        controller.run();
        spdlog::info("Controller thread exiting");
    }); // run the controller in a thread

    // TODO: add sigint and sigterm handling

    t_window.join(); // wait for the window to close before ending the program
    spdlog::info("Window closure signal detected, sending exit commands...");
    tasks::addFileControllerTask(tasks::CT_EXIT); // signal the controller to exit
    t_controller.join(); // wait for the controller to exit

    spdlog::info("All threads joined, exiting the program!");
    spdlog::shutdown();
}