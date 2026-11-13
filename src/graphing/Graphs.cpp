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

Graph simplify_graph(Graph& G, Graph& S, float prune_tolerance, float bezier_tolerance){
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

        double error = ml * prune_tolerance;

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

    
    // calculate bezier curves from the simplified thing
    std::unordered_map<size_t, size_t> sid_to_rid; // map S-vertex ID to R-vertex ID
    for(size_t plid = 0; plid < pruned.size(); plid++){
        const auto& relpts = pruned.at(plid);
        const size_t N = relpts.size();
        if(relpts.size() < 2){
            spdlog::warn("Degenerate Edge detected in graph simplification");
            continue;
        }

        // First: only segments need what follows, links can be skipped
        if (!G.edges.at(plid).segPart) {
            if (N != 2){
                spdlog::warn("Degenerate non-segment found in simplification, skipping");
                continue;
            }
            auto mapv = [&](size_t sid)->size_t{
                auto it = sid_to_rid.find(sid);
                if(it != sid_to_rid.end()) return it->second;
                size_t rid = R.vertices.size();
                R.vertices.emplace_back(rid, S.vertices.at(sid).pos);
                sid_to_rid.emplace(sid, rid);
                return rid;
            };
            size_t rid0 = mapv(relpts.front()), rid1 = mapv(relpts.back());
            size_t lid = R.edges.size();
            Edge ne(G.edges.at(plid).gid, rid0, rid1, G.edges.at(plid).originalLength);
            ne.lid = lid; ne.segPart = false; ne.length = G.edges.at(plid).length; ne.parent_lid = plid;
            R.edges.push_back(std::move(ne));
            continue;
        }

        // copy 3D positions of pruned points (simpler code)
        std::vector<glm::dvec3> pos;
        pos.reserve(N);
        for(size_t sid : relpts) pos.push_back(S.vertices.at(sid).pos);

        // centripetal parameterisation of the whole anchor chain [0..1]
        std::vector<double> t(N, 0.0);
        double total = 0.0;
        for(size_t i = 1; i < N; i++) total += std::sqrt(glm::length(pos[i]-pos[i-1]) + 1e-12);
        double acc = 0.0;
        for(size_t i = 1; i < N; i++){
            acc += std::sqrt(glm::length(pos[i]-pos[i-1]) + 1e-12);
            t[i] = (total > 1e-12) ? (acc/total) : (double(i)/(N-1));
        }

        // scale invariant tolerance (same idea as DP)
        double tol = 0.0;
        for(size_t i=1;i<N;i++) tol += glm::length(pos[i]-pos[i-1]);
        tol = (N>1) ? (tol/(N-1) * bezier_tolerance) : 0.0;

        struct Chunk { int s, e; glm::dvec3 c1, c2; };
        std::vector<Chunk> chunks;

        // recursive least squares cubic fit
        std::function<void(int,int)> split = [&](int s, int e){
            // two points = straight cubic
            if(e == s+1){
                chunks.push_back({s,e,
                    pos[s] + (pos[e]-pos[s])/3.0,
                    pos[e] - (pos[e]-pos[s])/3.0
                });
                return;
            }

            double t0 = t[s], t1 = t[e];
            double range = t1 - t0;
            if(range < 1e-12){
                chunks.push_back({s,e,
                    pos[s] + (pos[e]-pos[s])/3.0,
                    pos[e] - (pos[e]-pos[s])/3.0});
                return;
            }

            // Normal equations for LS fit of inner control points
            double a00=0,a01=0,a11=0;
            double bx0=0,bx1=0, by0=0,by1=0, bz0=0,bz1=0;
            for(int k=s;k<=e;k++){
                double tt = (t[k]-t0)/range;
                double A0 = 3.0*(1.0-tt)*(1.0-tt)*tt;
                double A1 = 3.0*(1.0-tt)*tt*tt;
                a00 += A0*A0; a01 += A0*A1; a11 += A1*A1;
                glm::dvec3 target = pos[k]
                    - std::pow(1.0-tt,3.0)*pos[s]
                    - std::pow(tt,3.0)*pos[e];
                bx0 += A0*target.x; bx1 += A1*target.x;
                by0 += A0*target.y; by1 += A1*target.y;
                bz0 += A0*target.z; bz1 += A1*target.z;
            }

            double det = a00*a11 - a01*a01;
            glm::dvec3 C1,C2;
            bool ok = false;
            if(std::abs(det) > 1e-14){
                C1.x=(bx0*a11-bx1*a01)/det; C2.x=(a00*bx1-a01*bx0)/det;
                C1.y=(by0*a11-by1*a01)/det; C2.y=(a00*by1-a01*by0)/det;
                C1.z=(bz0*a11-bz1*a01)/det; C2.z=(a00*bz1-a01*bz0)/det;

                double maxErr = 0.0;
                for(int k=s;k<=e;k++){
                    double tt = (t[k]-t0)/range;
                    double b0=std::pow(1.0-tt,3.0);
                    double b1=3.0*(1.0-tt)*(1.0-tt)*tt;
                    double b2=3.0*(1.0-tt)*tt*tt;
                    double b3=std::pow(tt,3.0);
                    glm::dvec3 B = b0*pos[s] + b1*C1 + b2*C2 + b3*pos[e];
                    maxErr = std::max(maxErr, (double)glm::length(B-pos[k]));
                }
                if(maxErr <= tol) ok = true;
            }

            if(ok){
                chunks.push_back({s,e,C1,C2});
            } else {
                int mid = s + (e-s)/2;
                split(s,mid);
                split(mid,e);
            }
        };

        split(0, static_cast<int>(N)-1);

        // Emit one edge per chunk (interior anchors are consumed by the cubic)
        for(const auto& ck : chunks){
            size_t sid0 = relpts[ck.s];
            size_t sid1 = relpts[ck.e];

            size_t rid0, rid1;
            auto it = sid_to_rid.find(sid0);
            if(it==sid_to_rid.end()){
                rid0 = R.vertices.size();
                R.vertices.emplace_back(rid0, S.vertices.at(sid0).pos);
                sid_to_rid.emplace(sid0, rid0);
            } else rid0 = it->second;

            it = sid_to_rid.find(sid1);
            if(it==sid_to_rid.end()){
                rid1 = R.vertices.size();
                R.vertices.emplace_back(rid1, S.vertices.at(sid1).pos);
                sid_to_rid.emplace(sid1, rid1);
            } else rid1 = it->second;

            size_t segLid = R.edges.size();
            Edge ne(G.edges.at(plid).gid, rid0, rid1);
            ne.segPart = true;
            ne.length  = std::max(1, G.edges.at(plid).length / static_cast<int>(chunks.size()));
            ne.parent_lid = plid;
            ne.lid = segLid;
            R.edges.push_back(std::move(ne));

            if(R.ctpts.size() <= segLid) R.ctpts.resize(segLid+1);
            R.ctpts[segLid] = std::make_pair(ck.c1, ck.c2);
        }
    }
    
    R.fill_adjacency_list();

    return R;
}