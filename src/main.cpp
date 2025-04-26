#include "pch.hpp"
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "window.hpp"
#include "controller.hpp"

// TODO: at this stage, the code is poorly commented, that should be fixed
// TODO: at some later stages code structure should also be modified to be more readable to an outsider (especially the gfa parser)

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

    // initialize the controller, this class mostly handles files
    Controller controller;

    // TODO: Add some .ini file to read starting dimensions from, and save them to
    int window_w = 1080, window_h = 920;

    // TODO: if the main function keeps not doing anything later on in the project, the window can run here rather than in its own thread
    std::thread t_window([window_w, window_h](){
        spdlog::info("Window thread starting");
        Window window(window_w, window_h);
        window.run();
        spdlog::info("Window thread exiting");
    }); // run the window in a thread

    // run the controller
    spdlog::info("Controller starting");
    controller.run();

    // TODO: add sigint and sigterm handling

    t_window.join(); // wait for the window to close before ending the program
    spdlog::info("Progeam exiting...");
    spdlog::shutdown();
}