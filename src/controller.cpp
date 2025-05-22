#include "controller.hpp"

// the includes below will be used for determining what is the path of the binary
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;
using namespace ogdf;

Controller::Controller(infoExchange* c) : channel(c)
{   // Get the path of the DAGReader executable
#if defined(_WIN32)
    std::string buffer(MAX_PATH, '\0');
    DWORD size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0){
        spdlog::error("Controller error: GerModuleFileName failed");
        throw std::system_error(GetLastError(), std::system_category(), "GetModuleFileName failed");
    }
    buffer.resize(size);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size); // Get required size
    std::string buffer(size, '\0');
    _NSGetExecutablePath(buffer.data(), &size);
#else // Linux
    std::string buffer(PATH_MAX, '\0');
    ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (size == -1){
        spdlog::error("Controller error: readlink failed");
        throw std::system_error(errno, std::system_category(), "readlink failed");
    }
    buffer.resize(size);
#endif
    m_binaryPath = buffer;
    spdlog::info("Executable ran from: {}", m_binaryPath.string());

    iniPath = m_binaryPath.parent_path() / "DAGReader.ini";
    
    // check if .ini file exists, if not, create it
    if (!fs::exists(iniPath)){
        std::ofstream create(iniPath, std::ios::out);
        create.close();
    }
    
    // Now we load the .ini file
    spdlog::info("Loading the .ini file from {}", iniPath.string());
	std::ifstream iniFile(iniPath);
    if (!iniFile.is_open()){
        spdlog::error("Controller error: failed to open the .ini file");
        throw std::runtime_error("failed to open the .ini file");
    }

	ini.parse(iniFile);
    iniFile.close();

    // if the recentPaths section doesnt exist, create it
    if (ini.sections.find("recentPaths") == ini.sections.end())
        ini.sections["recentPaths"];

    auto& section = ini.sections.at("recentPaths");
    {   // now lets load those paths in
        std::scoped_lock lk(channel->recent_paths_mut);
        for(int i = 4; i >= 0; i--){ // there should be max 5 entries here
            if (section.find(std::to_string(i)) == section.end()) // if there is less, skip
                continue;
            channel->recent_paths.emplace_back(section.at(std::to_string(i)));
            // the entries are loaded backwards, this is so that upon adding a path (with push_back)
            // the paths get a new order, and only the last 5 paths get saved so new ones get saved as first (most recent)
        }
    }

    // update the ini file and close
    std::ofstream iniFileOut(iniPath, std::ios::trunc);
    if (iniFileOut.is_open()) ini.generate(iniFileOut);
    else spdlog::warn("Failed to open the ini file (for updating) at: {}", iniPath.string());
    iniFileOut.close();
}

void Controller::run()
{
    bool shouldExit = false;
    while(!shouldExit){
        spdlog::info("Controller waiting for a task...");
        std::unique_lock lk(channel->controller_tasks_mut);
        channel->controller_tasks_cv.wait(lk, [&](){ return !channel->controller_tasks.empty(); });
        auto t = channel->controller_tasks.front();
        channel->controller_tasks.pop();
        lk.unlock();

        spdlog::info("Controller recieved a task");

        if (t == tasks::OPEN_NFD || t == tasks::OPEN_PATH)
        {
            fs::path path;
            if (t == tasks::OPEN_NFD){
                spdlog::info("Task: OPEN_NFD");
                getPathNFD(path);
            }
            else{
                spdlog::info("Task: OPEN_PATH");
                lk.lock();
                path = channel->controller_tasks_paths.front();
                channel->controller_tasks_paths.pop();
                lk.unlock();
            }
            if (path.empty()) spdlog::info("Recieved an empty path");
            if (!fs::exists(path)) spdlog::warn("Recieved a path that doesnt exist: {}", path.string());
            else{
                // save the path to the recently used paths
                spdlog::info("Updating the .ini ...");
                {   // now we need to use recent_paths data
                    std::scoped_lock lk2(channel->recent_paths_mut);
                    
                    // remove previous occurences
                    channel->recent_paths.erase(
                        std::remove(channel->recent_paths.begin(), channel->recent_paths.end(), path),
                        channel->recent_paths.end()
                    );

                    // then add our path
                    channel->recent_paths.push_back(path);
                    
                    // update the ini file
                    auto& section = ini.sections.at("recentPaths");
                    
                    // save 5 (or less) elements from the back
                    for(int i = (int)channel->recent_paths.size() - 1; i >= 0 && (int)channel->recent_paths.size() - i <= 5; i--)
                        section[std::to_string((int)channel->recent_paths.size() - i - 1)] = channel->recent_paths[i].string();
                }
                std::ofstream iniFileOut(iniPath, std::ios::trunc);
                if (iniFileOut.is_open()) ini.generate(iniFileOut);
                else spdlog::warn("Failed to open the ini file (for updating) at: {}", iniPath.string());
                iniFileOut.close();

                spdlog::info("Running the parser on the file");
                graphPtr = std::make_unique<GFA>(path.string());
                channel->graph_loaded.store(true);
                channel->graph_param_change.store(true); // new things to draw now available
            }
        }
        else if (t == tasks::LAYOUT_GRAPH)
        {
            spdlog::info("Task: LAYOUT_GRAPH");
            if (graphPtr){
                spdlog::info("Laying out the graph");
                // prepare the graph datastructures
                m_graph.clear();
                m_graphAttr = GraphAttributes(m_graph, GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics | GraphAttributes::nodeLabel 
                    | GraphAttributes::nodeLabelPosition | GraphAttributes::edgeArrow);

                // first pull the graph data from the graphPtr
                graphPtr.get()->insertGraph(m_graph, m_graphAttr, channel->graph_segment_length, channel->graph_auto_determine_segment_length.load());

                // run graph layout algorithms
                PlanarizationLayout l;
                l.call(m_graphAttr);

                {   // set the zoom of the camera and center the graph
                    double minX, maxX, minY, maxY;
                    minX = minY = std::numeric_limits<double>::max();
                    maxX = maxY = std::numeric_limits<double>::min();
                    for (auto n:m_graph.nodes){ // first calculate the bounding box and the scale factor
                        if (m_graphAttr.x(n) < minX) minX = m_graphAttr.x(n);
                        if (m_graphAttr.x(n) > maxX) maxX = m_graphAttr.x(n);
                        if (m_graphAttr.y(n) < minY) minY = m_graphAttr.y(n);
                        if (m_graphAttr.y(n) > maxY) maxY = m_graphAttr.y(n);
                    }
                    double scale = 1.0 / std::max(maxX-minX, maxY-minY) / 2.0; // take care of the zoom
                    channel->cam->setZoom(scale);
                    double centerX = (minX+maxX) / 2.0, centerY = (minY+maxY) / 2.0; // take care of the translation
                    m_graphAttr.translate(-centerX, -centerY); // and apply
                }

                if (channel->renderer){
                    for(auto n:m_graph.nodes) // quads
                        channel->renderer->addQuad(
                            glm::vec2(m_graphAttr.x(n), m_graphAttr.y(n)), 
                            m_graphAttr.width(n), m_graphAttr.height(n), 
                            0.f, glm::u8vec4(255, 255, 255, 255)
                        );

                    for(auto e:m_graph.edges){ // lines
                        std::vector<float> pts;
                        for(auto& b:m_graphAttr.bends(e)) pts.insert(pts.begin(), {(float)b.m_x, (float)b.m_y});
                        channel->renderer->addLine(pts, glm::u8vec4(255, 255, 255, 255));
                    }

                    channel->graph_param_change.store(false); // update has been drawn, bool false now
                    channel->renderer->setShouldUpdate();
                }
                else spdlog::warn("Shared datastructure pointer is not defined");
            }
            else spdlog::warn("No graph found!");
        }
        else if (t == tasks::EXIT)
        {
            spdlog::info("Task: EXIT");
            shouldExit = true;
        } // exit shouldnt be checked like this but by some atomic bool in infochannel that shuts down the while
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