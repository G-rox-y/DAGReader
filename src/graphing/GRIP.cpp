#include "GRIP.hpp"

using namespace std;

float GRIP::find_dist(int id1, int id2) const {
    static map<pair<int, int>, int> alr_found;
    
    auto refpair = make_pair(min(id1, id2), max(id1, id2));
    if (alr_found.find(refpair) != alr_found.end()) // check if we alr calculated this
        return alr_found.at(refpair);

    queue<int> q;
    unordered_set<int> visited;
    q.push(id1); q.push(0);
    while(!q.empty()){
        int id = q.front(); q.pop();
        int d = q.front(); q.pop();
        if (visited.find(id) != visited.end()) continue;
        visited.insert(id);

        auto newpair = make_pair(min(id, id1), max(id, id1));
        alr_found[newpair] = d;

        if (id == id2){
            alr_found[refpair] = d;
            return static_cast<float>(d);
        }

        for(auto& v:m_adjListG.at(id)){
            if (visited.find(v) == visited.end()){
                q.push(v); q.push(d+1);
            }
        }
    }
    // if we exit the loop it means we have a disjoint union which should not happen
    grip_error("Error in base filter placement: Disjoint union detected");
    return 0.f; // return to make compiler shut up
}

void GRIP::grip_error(const std::string& description) const {
    spdlog::error("GRIP error: {}", description);
    throw std::runtime_error("GRIP error (check logs)");
}

void GRIP::create_filtrations(vector<vector<int>>& f) const {
    f.emplace_back(); // create first filter (identical to mr_graph.vertices)
    for(size_t i = 0; i < mr_graph->vertices.size(); i++) // fill first filter (all vertices)
        f[0].emplace_back(mr_graph->vertices[i].id);

    int maxDepth = 1; 
    while (f.back().size() > (size_t)m_dimensions+1){
        vector<int> f_prev(f.back().begin(), f.back().end()); // make a copy where we will pull random vertices from
        unordered_map<int, size_t> f_prev_indmap; // map with (vertex id, f_prev index) pairs
        unordered_set<int> visited;
        for(size_t i = 0; i < f_prev.size(); i++) // and fill it
            f_prev_indmap[f_prev[i]] = i;

        f.emplace_back(); // create new level
        while(!f_prev.empty()){
            queue<int> q; // BFS queue that contains int value pairs (id, depth)
            uniform_int_distribution<size_t> distr(0, f_prev.size()-1);
            size_t odabir = distr(m_rng); // choose random available vertex
            f.back().push_back(f_prev.at(odabir)); // add it to the filter
            q.push(f_prev.at(odabir)); q.push(0); // and use a BFS to remove other vertices that are too close
            while(!q.empty()){
                int id = q.front(); q.pop(); // extract id 
                int d = q.front(); q.pop(); // extract depth vertex is on

                if (visited.find(id) == visited.end()){ // do this only if unvisited
                    visited.insert(id); // and mark as visited

                    if (f_prev_indmap.find(id) != f_prev_indmap.end()){ // only if present in the filter layer
                        size_t index = f_prev_indmap.at(id); // get its index
                        f_prev[index] = f_prev.back(); // swap with last
                        f_prev.pop_back(); // and pop so removal is O(1)
                        f_prev_indmap[f_prev[index]] = index; // update the index value of the swapped element
                    }
                }

                if (d < maxDepth){
                    for(auto& v:m_adjListG.at(id)){
                        if(visited.find(v) == visited.end()){
                            q.push(v); q.push(d+1); // add next elements if good depth and unvisited
                        }
                    }
                }
            }
        }
        maxDepth *= 2;
    }

    // if we dont have dimensions+1 points in the last layer we will have to promote some points from the previous layer
    if (f.size() < 2 || f.front().size() < (size_t)m_dimensions+1) return; // can only do this if there is >=2 filters and enough vertices
    while(f.back().size() < (size_t)m_dimensions+1){ // fill with the elements in the previous filter which are farthest away
        float maxdist = 0.f; int saved_v;
        set<int> f_back_has(f.back().begin(), f.back().end());
        for(auto v:f[f.size()-2]){
            if (f_back_has.find(v) != f_back_has.end()) continue; // dont duplicate nodes

            float thisdist = 0.f; // calc the avg graph distance from all base nodes
            for(auto u:f.back())
                thisdist += find_dist(u, v);
            thisdist /= (float)f.back().size();

            if (thisdist > maxdist){ // if bigger save as farthest element yet
                maxdist = thisdist;
                saved_v = v;
            }
        }
        f.back().push_back(saved_v); // and add the farthest element
    }
}

void GRIP::compute_vertex_neighbourhoods(
    const Vertex* v, vector<vector<pair<int, int>>>& n, const vector<int> nbrs, 
    const vector<unordered_set<int>>& f_c, const int K, const unordered_set<int>& placed
) const {
    queue<int> q; // BFS queue that contains int value pairs (id, depth)
    unordered_set<int> visited{v->id};
    for(auto& el:m_adjListG.at(v->id)){ // init with vertices next to v
        q.push(el); q.push(1);
    }

    n.resize(K+1);
    for(int i = 0; i <= K; i++){
        while(n[i].size() < (size_t)nbrs[i] && !q.empty()){
            int id = q.front(); q.pop(); // extract index of vertex in question
            int d = q.front(); q.pop(); // extract depth vertex is on

            if (visited.find(id) != visited.end()) continue;
            visited.insert(id);

            if (
                ( i < K && f_c[i].find(id) != f_c[i].end())  // if present in the filter they are neighbours
                || ( i == K && K+1 != (int)nbrs.size() && placed.find(id) != placed.end())
                // ^ for the last neighbourhood we can add only already placed, except if the base layer
            ){
                n[i].emplace_back(make_pair(id, d));
            }
            
            for(auto& el:m_adjListG.at(id)){
                if (visited.find(el) == visited.end()){
                    q.push(el); q.push(d+1); // keep traversing the graph
                }
            }
        }
        if (i < K) // if the next filter exists
            for(auto& kv:n[i]) // copy all of the current neighbours
                if (f_c[i+1].find(kv.first) != f_c[i+1].end()) // if they are present in the next filter
                    n[i+1].push_back(kv);
    }
}

void GRIP::base_filter_placement(const vector<int>& base) const {
    if (base.size() < 1) return;
    mr_graph->vertices[base[0]].pos = glm::vec3(0.f);

    if (base.size() < 2) return;
    float dist01 = find_dist(base[0], base[1]);
    mr_graph->vertices[base[1]].pos = glm::vec3(dist01, 0.f, 0.f);

    if (base.size() < 3 || m_dimensions+1 < 3) return;

    float dist02 = find_dist(base[0], base[2]);
    float dist12 = find_dist(base[1], base[2]);
    
    float x3 = (glm::pow(dist01, 2) + glm::pow(dist02, 2) - glm::pow(dist12, 2)) / dist01 / 2.f;
    float y3 = glm::sqrt(glm::pow(dist02, 2) - glm::pow(x3, 2));

    mr_graph->vertices[base[2]].pos = glm::vec3(x3, y3, 0.f);

    if (base.size() < 4 || m_dimensions+1 < 4) return;

    float r1 = find_dist(base[0], base[3]);
    float r2 = find_dist(base[1], base[3]);
    float r3 = find_dist(base[2], base[3]);

    float X = (dist01*dist01 + r1*r1 - r2*r2) / (2.0f * dist01);
    float Y = (x3*x3 + y3*y3 + r1*r1 - r3*r3 - 2.0f*x3*X) / (2.0f * y3);
    float zz = r1*r1 - X*X - Y*Y;
    // if zz < 0 its tehnically not correct to abs it, but eh
    float Z = std::sqrt(std::max(0.f, std::abs(zz)));

    mr_graph->vertices[base[3]].pos = glm::vec3(X, Y, Z);
}

void GRIP::vertex_initial_placement(Vertex* v, const vector<pair<int, int>>& n, const unordered_set<int>& placed) const {
    // since neighbourhoods are built with a BFS, its vector is already sorted by graph distance from v
    int found = 0;
    array<int, 3> ids{};
    for(size_t i = 0; i < n.size() && found < 3; i++){ // first find closest 3 placed vertices
        if (placed.find(n[i].first) == placed.end()) continue;
        ids[found++] = i; // increment the found variable after use
    }

    if (found == 0)
        grip_error("vertex initial placement found no neighbouring vertices?");
    
    // we do a placement using a simple barycenter method, this is because in the original paper two methods are outlined,
    // simple bc is one and the other is three closest neighbours which is explained in 2D and i dont understand how to apply in 3D
    // this is because in 3D the equations for that method have infinitely many solutions due to the added dimension
    // this could be changed with adding a 4th point so we get a tetrahedron, but nothing is said about that in the paper
    // in any case, testing that will be a TODO, and we will stick to the barycenter calculation of 3 neighbours

    glm::vec3 bc(0.f);
    for(int i = 0; i < found; i++) bc += mr_graph->vertices.at(n[ids[i]].first).pos;
    bc /= static_cast<float>(found);

    const float mag = 1e-3f * m_edgeLength;
    static uniform_real_distribution<float> dist(-mag, mag); // dont forget to change this if you make edgeLength changeable
    glm::vec3 jitter(dist(m_rng), dist(m_rng), dist(m_rng));

    v->pos = bc + jitter;
}

void GRIP::calc_temp(float& oldTemp, float& oldCos, const glm::vec3& oldDisp, const glm::vec3& force) const {
    if (glm::length(oldDisp) == 0.f) return;
    
    float heat, cos = glm::dot(force, oldDisp) / (glm::length(force) * glm::length(oldDisp));
    if (oldCos * cos > 0)
        heat = oldTemp * cos * m_temperatureGain * m_temperatureNarrowGainIncrease;
    else
        heat = oldTemp * cos * m_temperatureGain;
    
    oldCos = cos;
    oldTemp = heat;
}

glm::vec3 GRIP::compute_KKforce(const Vertex* v, const vector<pair<int, int>>& n) const {
    glm::vec3 force = glm::vec3(0.f);
    for(size_t i = 0; i < n.size(); i++){
        Vertex* u = &mr_graph->vertices.at(n[i].first);
        float distR = glm::distance(u->pos, v->pos);
        float distG = static_cast<float>(n[i].second) * static_cast<float>(glm::pow(m_edgeLength, 2));
        glm::vec3 inc = (u->pos - v->pos) * (distR / distG - 1.f);
        force += inc;
    }
    return force;
}

glm::vec3 GRIP::compute_FRforce(const Vertex* v, const vector<pair<int, int>>& n) const {
    glm::vec3 force;
    for(size_t i = 0; i < n.size(); i++){
        Vertex* u = &mr_graph->vertices.at(n[i].first);
        float factor = (float)pow(m_edgeLength, 2) / max(glm::distance2(u->pos, v->pos), 1e-4f);
        force += (v->pos - u->pos) * factor * m_scalingFactor;
    }
    for(size_t i = 0; i < m_adjListG.at(v->id).size(); i++){
        Vertex* u = &mr_graph->vertices.at(m_adjListG.at(v->id)[i]);
        float factor = glm::distance2(u->pos, v->pos) / pow(m_scalingFactor, 2);
        force += (u->pos - v->pos) * factor;
    }
    return force;
}


GRIP::GRIP(Graph& g) : mr_graph(&g) {
    for(size_t i = 0; i < mr_graph->vertices.size(); i++) // touch all of the indices so they at least exist
        m_adjListG[i] = vector<int>();
    for(size_t i = 0; i < mr_graph->edges.size(); i++){ // fill in the connections
        auto& e = mr_graph->edges.at(i);
        m_adjListG[e.start].emplace_back(e.end);
        m_adjListG[e.end].emplace_back(e.start);
    }
}

void GRIP::run() {
    // for logic behind how the algorithm works read up on GRIP: Graph Drawing with Intelligent Placement
    // i recommend the original papers by Pawel Gajer, Michael T. Goodrich, and Stephen G. Kobourov
    // at the time of writing, all info can be found here: https://www2.cs.arizona.edu/~kobourov/GRIP/

    size_t N = mr_graph->vertices.size(); // to simplify code
    if (N < 1) return;

    // calculate average degree of the graph
    for(auto& [_,val]:m_adjListG)
        m_avgDegG += static_cast<float>(val.size());
    m_avgDegG /= static_cast<float>(m_adjListG.size());

    // filters V_0, V_1, V_2 ... (V_i is a subset of V_i-1), filters[0] is V_0
    vector<vector<int>> filters;
    create_filtrations(filters);
    size_t K = filters.size(); // to simplify code
    
    // a structure for checking if element is a member of a filter
    vector<unordered_set<int>> filter_finder(K);
    for(size_t i = 0; i < K; i++) // copy the elements over
        for(size_t j = 0; j < filters[i].size(); j++)
            filter_finder[i].insert(filters[i][j]);

    vector<int> nbrs(K);
    for(size_t i = 0; i < K; i++)
        nbrs[i] = max(3, static_cast<int>(ceil(
                m_avgDegG * static_cast<float>(N) / static_cast<float>(filters[i].size())
            )));

    // neighbourhoods for every vertex indexed by their ID
    // neighbourhoods[6][1] means neighbourhood of vertex with ID 6 for V_1 (hence N_1)
    // neighbourhood contains a vector of pairs, first is a vertex, and second is its distance
    vector<vector<vector<pair<int, int>>>> neighbourhoods(N); 

    // helper vectors for heat and displacement calculations, all are indexed by vertex id
    vector<glm::vec3> displacements(N, glm::vec3(0.f));
    vector<float> oldCos(N, 0.f); // a cosine angle between a previous displacement and a new one
    vector<float> heat(N, m_edgeLength/6.f); // default heat is a sixth of edge length

    // a set to help track which vertices have already been placed and which havent
    unordered_set<int> placed_id;
    for(int i = (int)K-1; i >= 0; i--){ // start with smaller filters and progress to larger filters (to i = 0)
        vector<int>& v = filters[i]; // just an alias to simplify code

        if (i < (int)K-1){
            for(size_t j = 0; j < v.size(); j++){ // setup new vertices
                if (placed_id.find(v[j]) != placed_id.end()) continue;
                Vertex* vp = &mr_graph->vertices.at(v[j]);
                compute_vertex_neighbourhoods(vp, neighbourhoods[v[j]], nbrs, filter_finder, i, placed_id);
                vertex_initial_placement(vp, neighbourhoods[v[j]][i], placed_id);
                placed_id.insert(v[j]);
            }
        }
        else{  // base filter needs special treatment
            for(size_t j = 0; j < v.size(); j++){
                Vertex* vp = &mr_graph->vertices.at(v[j]);
                compute_vertex_neighbourhoods(vp, neighbourhoods[v[j]], nbrs, filter_finder, i, placed_id);
                placed_id.insert(v[j]);
            }
            base_filter_placement(v);
        }
        for(int r = 0; r < m_rounds_number; r++){
            for(size_t j = 0; j < v.size(); j++){
                Vertex* vp = &mr_graph->vertices.at(v[j]);

                glm::vec3 force;
                if (i == 0) // for the last iteration compute with FRforce
                    force = compute_FRforce(vp, neighbourhoods[v[j]][i]);
                else
                    force = compute_KKforce(vp, neighbourhoods[v[j]][i]);
                
                if (glm::length(force) < 1e-4) continue;

                calc_temp(heat[v[j]], oldCos[v[j]], displacements[v[j]], force);
                displacements[v[j]] = heat[v[j]] * glm::normalize(force);
            }
            for(size_t j = 0; j < v.size(); j++)
                mr_graph->vertices.at(v[j]).pos += displacements[j];
        }
    }
}