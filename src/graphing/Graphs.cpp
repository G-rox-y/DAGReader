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
    for(auto& p:ctpts){
        p.first = p.first + T;
        p.second = p.second + T;
    }
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

void Graph::fill_adjacency_list(){
    this->adjList.clear();
    this->adjList.resize(this->vertices.size());
    for(size_t i = 0; i < this->edges.size(); i++){
        auto& e = this->edges.at(i);
        this->adjList[e.start].push_back(std::make_pair(e.end, i));
        this->adjList[e.end].push_back(std::make_pair(e.start, i));
    }
    this->adjList.shrink_to_fit();
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

Graph subdivide_graph(Graph& G, int minSubDiv, int maxSubDiv){
    Graph S;
    S.vertices = G.vertices;
    for (auto eG : G.edges){
        int segdeg = std::clamp<int>(eG.length, minSubDiv, maxSubDiv);
        if (segdeg == 0 || !eG.segPart){
            auto newLid = S.edges.size();
            S.edges.push_back(eG);
            S.edges.back().parent_lid = eG.lid;
            S.edges.back().lid = newLid;
        }
        else{
            for (int i = 0; i < segdeg; i++){
                auto newid = S.vertices.size();
                S.vertices.emplace_back(newid);
            }
            for(int i = 0; i <= segdeg; i++){
                auto newLid = S.edges.size();
                S.edges.push_back(eG);
                S.edges.back().lid = newLid;
                S.edges.back().length = std::max(S.edges.back().length/segdeg, 1); // TODO: make length able to be float
                S.edges.back().originalLength /= segdeg;
                S.edges.back().parent_lid = eG.lid;
                S.edges.back().sid = i;
                if (i == 0) S.edges.back().start = eG.start;
                else S.edges.back().start = S.vertices.size() - segdeg + i - 1;
                if (i == segdeg) S.edges.back().end = eG.end;
                else S.edges.back().end = S.vertices.size() - segdeg + i;
            }
        }
    }

    S.fill_adjacency_list();

    return S;
}

Graph simplify_graph(Graph& G, Graph& S, float tolerance, float tension){
    Graph R;
    
    // we will be locating groups and storing their position pairs (start, end) in a vector indexed by LID of graph G
    std::vector<std::pair<size_t, size_t>> groups(G.edges.size(), std::make_pair(std::numeric_limits<size_t>::max(), 0));
    // subdivide_graph inserts the new edges created by subdivision sequentially
    // this lets us be sure that we can always find the subdivision edges next to each other
    // so a single left to right pass of the edge vector is sufficient for locating it all
    for(auto eS:S.edges){
        auto& vel = groups.at(eS.parent_lid);
        if (vel.first > eS.lid) vel.first = eS.lid;
        if (vel.second < eS.lid) vel.second = eS.lid;
    }

    // only important points for each parent LID of graph G
    std::vector<std::vector<size_t>> pruned(G.edges.size());
    // run douglas pecker to prune irrelevant (colinear) points
    for(auto g:groups){
        // extract the points first (we rely on the points being stored in order)
        std::vector<size_t> pts;
        pts.push_back(S.edges.at(g.first).start);
        for(size_t i = g.first; i <= g.second; i++)
            pts.push_back(S.edges.at(i).end);

        // calculate mean length
        double ml = 0.0;
        for(size_t i = 1; i < pts.size(); i++)
            ml += glm::length(S.vertices.at(pts.at(i)).pos - S.vertices.at(pts.at(i-1)).pos);
        ml /= static_cast<double>(pts.size()-1);

        double error = ml * tolerance;

        std::vector<bool> select_pts(pts.size(), false);
        select_pts.at(0) = true;
        select_pts.back() = true;

        // now prune with douglas pecker
        std::queue<std::pair<int, int>> q; // this queue stores the start,end indices of the pts vector, not the IDs
        q.push(std::make_pair(0, pts.size()-1));
        while(!q.empty()){ // doing the queue cause its tidier than making a recursive function
            auto [s, e] = q.front(); q.pop();
            if (e == s+1) continue; // no points in between means there is nothing to look for anymore

            const auto& ep = S.vertices.at(pts.at(e)).pos;
            const auto& sp = S.vertices.at(pts.at(s)).pos;

            // find the most distanced point from the |se| line
            int maxP; double maxDist = 0.0; 
            for(int i = s+1; i < e; i++){
                const auto pp = S.vertices.at(pts.at(i)).pos;
                const auto dp = glm::length(glm::cross(ep-sp, pp-sp)) / glm::length(ep-sp);
                if (dp > maxDist){
                    maxDist = dp;
                    maxP = i;
                }
            }

            // if maxDist > error, keep it and work on the subsegments
            if (maxDist > error){
                select_pts.at(maxP) = true;
                q.push(std::make_pair(s, maxP));
                q.push(std::make_pair(maxP, e));
            }
        }

        size_t parent_lid = S.edges.at(g.first).parent_lid;
        for(size_t i = 0; i < pts.size(); i++)
            if (select_pts.at(i)) pruned.at(parent_lid).emplace_back(pts.at(i));
    }

    // map S-vertex ID to R-vertex ID
    std::unordered_map<size_t, size_t> sid_to_rid;
    // catmull rom
    for(size_t plid = 0; plid < pruned.size(); plid++){
        const auto& relpts = pruned.at(plid);
        const size_t N = relpts.size(); 
        if(relpts.size() < 2){
            spdlog::warn("Degenerate Edge detected in graph simplification");
            continue;    
        } 

        // copy relevant points vertices to R
        std::vector<size_t> rids; // and remember their assigned R-vertex IDs
        rids.reserve(N);
        for(size_t sid : relpts){
            auto it = sid_to_rid.find(sid);
            if(it == sid_to_rid.end()){
                size_t rid = R.vertices.size();
                R.vertices.emplace_back(rid, S.vertices.at(sid).pos);
                sid_to_rid.emplace(sid, rid);
                rids.push_back(rid);
            }
            else rids.push_back(it->second);
        }

        // First: only segments need catmull rom, links can be skipped
        if (!G.edges.at(plid).segPart) {
            for(size_t i = 0; i+1 < N; i++){
                size_t newLid = R.edges.size();
                Edge ne(G.edges.at(plid).gid, rids[i], rids[i+1], G.edges.at(plid).originalLength);
                ne.lid = newLid;
                ne.segPart = false;
                ne.length = G.edges.at(plid).length;
                ne.parent_lid = plid;
                R.edges.push_back(std::move(ne));
            }
            continue;
        }

        size_t baseLid = R.edges.size(); // remember how many edges before adding new ones
        // add edges with dummy data
        int len = std::max(1, G.edges.at(plid).length / static_cast<int>(N));
        for(size_t i = 0; i+1 < N; i++){
            Edge ne(G.edges.at(plid).gid, rids[i], rids[i+1]);
            ne.segPart = true;
            ne.length = len;
            ne.lid = R.edges.size();
            ne.parent_lid = plid;
            R.edges.push_back(std::move(ne));
        }

        // catmull rom -> cubic Bezier for this point chain.
        // we duplicate the endpoints as phantom neighbours so the open curve starts and ends exactly at them
        const float s = tension / 6.0f;
        for(size_t i = 0; i+1 < N; i++){
            glm::dvec3 p_curr = R.vertices.at(rids.at(i)).pos;
            glm::dvec3 p_next = R.vertices.at(rids.at(i+1)).pos;
            
            glm::dvec3 p_prev = (i == 0) ? R.vertices.at(rids.at(0)).pos : R.vertices.at(rids.at(i-1)).pos;
            glm::dvec3 p_next2 = (i+2 < N) ? R.vertices.at(rids.at(i+2)).pos : p_next;

            glm::dvec3 c1 = p_curr + (p_next-p_prev) * static_cast<double>(s);
            glm::dvec3 c2 = p_next - (p_next2-p_curr) * static_cast<double>(s);

            size_t segLid = baseLid + i;
            if (R.ctpts.size() <= segLid) R.ctpts.resize(segLid + 1);
            R.ctpts[segLid] = std::make_pair(c1, c2);
        }
    }
    
    R.fill_adjacency_list();

    return R;
}