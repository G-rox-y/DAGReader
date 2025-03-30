#include <thread>
#include "window.hpp"
#include "controller.hpp"

int main()
{
    // TODO: Add some .ini file to read starting dimensions from, and save them to
    int window_w = 1080, window_h = 920;

    std::thread t_window([window_w, window_h](){
        Window window(window_w, window_h);
        window.run();
    }); // run the window in a thread

    std::thread t_controller([](){
        Controller controller;
        controller.run();
    }); // run the controller in a thread

    t_window.join(); // wait for the window to close before ending the program
    tasks::addFileControllerTask(tasks::CT_EXIT); // signal the controller to exit
    t_controller.join(); // wait for the controller to exit
}