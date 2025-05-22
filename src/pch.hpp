#pragma once

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <inipp/inipp.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/fileformats/GraphIO.h>

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <memory>
#include <fstream>
#include <filesystem>

#include <exception>

#include <map>
#include <array>
#include <queue>
#include <tuple>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>

#include <limits>
#include <algorithm>

// this is a header for precompiling