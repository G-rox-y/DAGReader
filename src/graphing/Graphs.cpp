#include "Graphs.hpp"

const glm::dvec3 Graph::calculateBarycenter() {
    // calculate the barycenter
    glm::dvec3 bc(0.0);
    for(auto& v:vertices) bc += v.pos;
    bc /= static_cast<double>(vertices.size());
    
    // and update the radius
    radius = 0.0;
    for(auto& v:vertices)
        radius = std::max<double>(radius, glm::distance(bc, v.pos));
    radius = std::max<double>(radius + 0.5, radius * 1.05); // add some clearance

    return bc;
}

void Graph::translate(const glm::dvec3& T){
    for(auto& v:vertices)
        v.pos = v.pos + T;
}

int Graph::findDist(const size_t id1, const size_t id2){
    std::queue<std::pair<size_t, int>> q;
    std::vector<int> visited(vertices.size(), false);
    q.push(std::make_pair(id1, 0));
    while(!q.empty()){
        auto [id, d] = q.front(); q.pop();
        if (visited.at(id)) continue;
        visited.at(id) = true;

        if (d < 0) [[unlikely]] throw std::runtime_error("Graph: findDist error: Negative distance found");

        if (id == id2) return d;

        for(auto [v, _]:adjList.at(id))
            if (!visited.at(v)) q.push(std::make_pair(v, d+1));
    }

    throw std::runtime_error("Graph: findDist error: Negative distance found");
}

void graphCollection::setGraphs(std::vector<Vertex>& v, std::vector<Edge>& e){
    graphs.clear();
    // create and adjacency list, indexed by Vertex ID
    std::vector<std::vector<std::pair<size_t, size_t>>> adjList(v.size(), std::vector<std::pair<size_t, size_t>>());
    for(size_t i = 0; i < e.size(); i++){
        adjList[e.at(i).start].push_back(std::make_pair(e.at(i).end, i));
        adjList[e.at(i).end].push_back(std::make_pair(e.at(i).start, i));
    }

    // use the adjacencly list to separate the vertex vector into disjoint graphs
    long long int lastElement = adjList.size() - 1; // the largest ID that wasnt added to some graph
    std::vector<bool> visited(adjList.size(), false);
    while(lastElement >= 0){
        // Create graph
        graphs.emplace_back();

        // since vertices and edges will be moved, their indices will change, the change needs to be remembered
        // key = old ID, value = new ID
        std::unordered_map<size_t, size_t> vertMap, edgeMap;

        // "Flood Fill" ; Go through all vertices connected to the LastElement
        std::queue<long long int> q; 
        q.push(lastElement);
        while(!q.empty()){
            auto VertexID = q.front(); q.pop();
            if (visited[VertexID]) continue;
            visited[VertexID] = true;

            // Add to graph
            graphs.back().vertices.emplace_back(std::move(v[VertexID]));

            // Change the ID, and remember the change
            graphs.back().vertices.back().id = vertMap[VertexID] = graphs.back().vertices.size()-1;

            for(auto [VID, EID]:adjList[VertexID]){
                q.push(VID);
                if (edgeMap.find(EID) == edgeMap.end()){ // if new edge
                    graphs.back().edges.emplace_back(std::move(e[EID])); // add to graph
                    edgeMap[EID] = graphs.back().edges.size()-1; // and remember the ID change
                }
            }
        }

        // Reassign graph vertex IDs inside edges
        for(auto& edge:graphs.back().edges){
            edge.start = vertMap.at(edge.start);
            edge.end = vertMap.at(edge.end);
            edge.lid = edgeMap.at(edge.gid);
        }

        // copy the adjacency list and modify the IDs inside
        graphs.back().adjList.resize(vertMap.size(), std::vector<std::pair<size_t, size_t>>());
        for(const auto& [oldID, newID]:vertMap){
            graphs.back().adjList[newID] = adjList[oldID];
            for(auto& el:graphs.back().adjList.at(newID))
                el = std::make_pair(vertMap[el.first], edgeMap[el.second]);
        }
        
        while(lastElement >= 0 && visited[lastElement]) lastElement--;
    }
}
