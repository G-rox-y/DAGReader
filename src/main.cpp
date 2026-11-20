#include "pch.hpp"
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "cmakevars.hpp"

#include "graphics/window.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

// Roadmaps (subject to change):
//
// V 1.1 "The Graphics update" Roadmap:
// [x] - Adding curve segmentation
// [x] - Replace Geometry shaders with Compute shaders
// [] - Switch graphics backend to Vulkan
// [] - Mac support
// [] - Add compute shaders for calculating which boxes need not be drawn at all
// [] - Adding compute shaders for RMF calculations
// [] - Add Maxent as default layout
// [] - 2D control scheme
// [] - Replace realtime gpu side hover highlighting with cpu side highlighting
// [] - Rethink the selection mechanism to be always active
// [] - Draw faraway segments as lines
// [] - Adjust CMake to try to search for modules instead of just fetching by default
// [] - add CMake install
// [] - Automatic scale selection when moving and focusing
//
// V 1.2 "The GFA1 update" Roadmap:
// [] - Adding Fuzzy search
// [] - Containment visualization
// [] - Path visualization
// [] - GFA 1.1 / 1.2 support
// [] - Walk visualization
// [] - Jump visualization
// [] - GC content coloring
// [] - Determining contiguity and coloring based on it
// [] - Logarithm segment length scaling
// [] - Coverage coloring
// [] - More graph info (like in Bandage), also should shown individually for subgraph info
// [] - Modify GFA data stored in tags
// [] - Tag based coloring
// [] - Sequence reading from external files
// [] - File descriptions (like in bandageng)
// [] - Fetching files from servers
//
// Other planned features
// [] - Export image of view
// [] - Modify (non-color) CSV data inside DAGReader
// [] - Import FASTA
// [] - Better explanations of side panel options
// [] - Moving segments and links (like in Bandage)
// [] - Memory optimizations
// [] - Changeable debug timestamp precision (save in .ini)
// [] - Layout time estimates
// [] - Kofi
// [] - cli
// [] - When focusing, instead of telerporting give a trajectory to the camera so it animates

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
    // we gotta know our versions
    spdlog::info("DAGReader Version: {}", DAGR_VERSION_STRING);

    spdlog::debug("Starting the program!");

    // initialize the communication channel for threads
    infoExchange channel;

    // initialize the controller, this class mostly handles files
    Controller controller(&channel);

    // TODO: Add some .ini file to read starting dimensions from, and save them to
    int window_w = 1080, window_h = 920;

    // TODO: if the main function keeps not doing anything later on in the project, the window can run here rather than in its own thread
    std::thread t_window([window_w, window_h, &channel](){
        spdlog::debug("Window thread starting");
        Window window(&channel, window_w, window_h);
        window.run();
        spdlog::debug("Window thread exiting");
    }); // run the window in a thread

    // run the controller
    spdlog::debug("Controller starting");
    controller.run();

    // TODO: add sigint and sigterm handling

    t_window.join(); // wait for the window to close before ending the program
    spdlog::debug("Program exiting...");
    spdlog::shutdown();
}