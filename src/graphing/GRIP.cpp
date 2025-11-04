#include "GRIP.hpp"

using namespace std;
using namespace stdpp;

double GRIP::find_edge_length(int id1, int id2) const {
    sorted_pair p(id1, id2);
    if (m_edgeLengths.find(p) == m_edgeLengths.end()) return m_defaultEdgeLength;
    else return m_defaultEdgeLength * m_edgeLengths.at(p);
}

void GRIP::grip_error(const string& description) const {
    spdlog::error("GRIP error: {}", description);
    throw runtime_error("GRIP error (check logs)");
}

void GRIP::compute_filters(std::vector<std::vector<int>>& filters) const {
    filters.emplace_back(); // create first filter layer (identical to the whole graph)
    for(auto v: mr_g->vertices)
        filters.back().emplace_back(v.id);
    
    int maxDepth = 1;
    while (filters.back().size() > static_cast<size_t>(m_dimensions+1)){
        vector<int> layer_copy(filters.back().begin(), filters.back().end()); // make a copy where we will pull random vertices from
        unordered_map<int, size_t> layer_copy_indmap; // map with (vertex id, layer_copy index) pairs
        for(size_t i = 0; i < layer_copy.size(); i++) // and fill it
            layer_copy_indmap[layer_copy[i]] = i;
        
        unordered_set<int> visited;
        filters.emplace_back(); // create new level
        while(!layer_copy.empty()){
            queue<int> q; // BFS queue that contains int value pairs (id, depth)
            uniform_int_distribution<size_t> distr(0, layer_copy.size()-1);
            size_t odabir = distr(m_rng); // choose random available vertex
            filters.back().push_back(layer_copy.at(odabir)); // add it to the filter
            q.push(layer_copy.at(odabir)); q.push(0); // and use a BFS to remove other vertices that are too close
            while(!q.empty()){
                int id = q.front(); q.pop(); // extract id 
                int d = q.front(); q.pop(); // extract depth vertex is on

                if (visited.find(id) != visited.end()) continue; // do this only if unvisited
                visited.insert(id); // and mark as visited

                // if this id is present in the filter layer you may remove it
                if (layer_copy_indmap.find(id) != layer_copy_indmap.end()){
                    size_t index = layer_copy_indmap.at(id); // get its index
                    layer_copy[index] = layer_copy.back(); // swap with last
                    layer_copy_indmap[layer_copy[index]] = index; // update the index value of the swapped element
                    layer_copy.pop_back(); // and pop so removal is O(1)
                }

                if (d < maxDepth){
                    for(auto& [v, _]:mr_g->adjList.at(id)){
                        q.push(v); q.push(d+1); // add next elements if good depth
                    }
                }
            }
        }
        maxDepth *= 2;
    }
    size_t K = filters.size();
    // if we dont have dimensions+1 points in the last layer we will have to promote some points from the previous layer
    // but we can only do this if there is >=2 filters and enough vertices
    if (K >= 2 && mr_g->vertices.size() >= static_cast<size_t>(m_dimensions+1))
    {
        // fill with the elements in the previous filter which are farthest away
        vector<int>& last_filter = filters.back();
        vector<int>& almost_last_filter = filters.at(K-2);
        while(last_filter.size() < static_cast<size_t>(m_dimensions+1))
        {
            double maxdist = 0.0; int saved_v;
            set<int> f_back_has(last_filter.begin(), last_filter.end());
            for(auto v:almost_last_filter){
                if (f_back_has.find(v) != f_back_has.end()) continue; // dont duplicate nodes

                // calc the avg graph distance from all base nodes
                double thisdist = 0.0;
                for(auto u:last_filter) thisdist += mr_g->findDist(u, v);
                thisdist /= static_cast<double>(last_filter.size());

                // if bigger save as farthest element yet
                if (thisdist > maxdist){
                    maxdist = thisdist;
                    saved_v = v;
                }
            }
            last_filter.push_back(saved_v); // and add the farthest element
        }
    }
}

void GRIP::compute_vertex_neighbourhoods(
    int ID, vector<vector<pair<int, int>>>& n, const vector<size_t> nbrs, 
    const vector<unordered_set<int>>& f_c, const int K, const unordered_set<int>& placed
) const {
    queue<pair<int, int>> q; // BFS queue that contains int value pairs (id, depth)
    unordered_set<int> visited{ID};
    for(auto& [el, _]:mr_g->adjList.at(ID)) // init with vertices next to v
        q.push(make_pair(el, 1));

    n.resize(K+1);
    for(int i = 0; i <= K; i++){
        while(n[i].size() < nbrs[i] && !q.empty()){
            auto [id, d] = q.front(); q.pop();

            if (visited.find(id) != visited.end()) continue;
            visited.emplace(id);

            if (
                ( i < K && f_c[i].find(id) != f_c[i].end())  // if present in the filter they are neighbours
                || ( i == K && K+1 != (int)nbrs.size() && placed.find(id) != placed.end())
                // ^ for the last neighbourhood we can add only already placed, except if the base layer
            ) n[i].emplace_back(make_pair(id, d));
            
            for(auto& [el, _]:mr_g->adjList.at(id))
                if (visited.find(el) == visited.end())
                    q.push(make_pair(el, d+1)); // keep traversing the graph
        }
        if (i < K) // if the next filter exists
            for(auto& kv:n[i]) // copy all of the current neighbours
                if (f_c[i+1].find(kv.first) != f_c[i+1].end()) // if they are present in the next filter
                    n[i+1].push_back(kv);
    }
}

void GRIP::base_filter_placement(const vector<int>& base) const {
    if (base.size() < 1) return;
    mr_g->vertices[base[0]].pos = glm::dvec3(0.0);

    if (base.size() < 2) return;
    double dist01 = static_cast<double>(mr_g->findDist(base[0], base[1]));
    mr_g->vertices[base[1]].pos = glm::dvec3(dist01, 0.0, 0.0);

    if (base.size() < 3 || m_dimensions+1 < 3) return;
    double dist02 = static_cast<double>(mr_g->findDist(base[0], base[2]));
    double dist12 = static_cast<double>(mr_g->findDist(base[1], base[2]));
    double x2 = (dist01*dist01 + dist02*dist02 - dist12*dist12) / dist01 / 2.0;
    double y2 = sqrt(dist02*dist02 - x2*x2);
    mr_g->vertices[base[2]].pos = glm::dvec3(x2, y2, 0.0);
    
    if (base.size() == 4 && m_dimensions+1 == 4) return;
    double dist03 = static_cast<double>(mr_g->findDist(base[0], base[3]));
    double dist13 = static_cast<double>(mr_g->findDist(base[1], base[3]));
    double dist23 = static_cast<double>(mr_g->findDist(base[2], base[3]));

    if (abs(y2) < 1e-5){ // if first three were colinear
        double x3 = (dist01*dist01 + dist02*dist02 - dist12*dist12) / dist01 / 2.0;
        double y3 = sqrt(dist02*dist02 - x3*x3);
        mr_g->vertices[base[2]].pos = glm::dvec3(x3, y3, 0.0);
    }
    else{
        double x3 = (dist01*dist01 + dist03*dist03 - dist13*dist13) / (2.0 * dist01);
        double y3 = (x2*x2 + y2*y2 + dist03*dist03 - dist23*dist23 - 2.0*x2*x3) / (2.0 * y2);
        double z3 = sqrt(dist03*dist03 - x3*x3 - y3*y3);
        mr_g->vertices[base[3]].pos = glm::dvec3(x3, y3, z3);
    }

}

void GRIP::vertex_initial_placement(int ID, const vector<pair<int, int>>& n, const unordered_set<int>& placed) const {
    // since neighbourhoods are built with a BFS, its vector is already sorted by graph distance from v
    int found = 0;
    array<int, 4> ids{};
    for(size_t i = 0; i < n.size() && found < m_dimensions+1; i++){ // first find closest placed vertices
        if (placed.find(n[i].first) == placed.end()) continue;
        ids[found++] = i; // increment the found variable after use
    }

    if (found == 0)
        grip_error("vertex initial placement found no neighbouring vertices?");
    
    // do a placement using a simple barycenter method
    glm::dvec3 bc(0.0);
    for(int i = 0; i < found; i++) bc += mr_g->vertices.at(n[ids[i]].first).pos;
    bc /= static_cast<double>(found);

    // add a small amount of noise to the simulation
    const double mag = 1e-3 * m_defaultEdgeLength;
    static uniform_real_distribution<double> dist(-mag, mag); // dont forget to change this if you make edgeLength changeable
    glm::dvec3 jitter(dist(m_rng), dist(m_rng), dist(m_rng));
    mr_g->vertices.at(ID).pos = bc + jitter;
}

void GRIP::calc_temp(double& oldTemp, double& oldCos, const glm::dvec3& oldDisp, const glm::dvec3& force) const {
    if (glm::length(oldDisp) == 0.0) return;
    
    double heat = oldTemp, cos = glm::dot(force, oldDisp) / (glm::length(force) * glm::length(oldDisp));
    if (oldCos * cos > 0.0)
        heat += oldTemp * cos * m_temperatureGain * m_temperatureNarrowGainIncrease;
    else
        heat += oldTemp * cos * m_temperatureGain;
    
    heat = max<double>(0.0, heat);

    oldCos = cos;
    oldTemp = heat;
}

glm::dvec3 GRIP::compute_KKforce(int ID, const vector<pair<int, int>>& n) const {
    glm::dvec3 force(0.0);
    glm::dvec3& POS = mr_g->vertices.at(ID).pos;
    for(auto& [OTHER_ID, d]:n){
        glm::dvec3 delta = mr_g->vertices.at(OTHER_ID).pos - POS;
        double distR = glm::length2(delta);
        double edgeL = find_edge_length(ID, OTHER_ID);
        double distG = edgeL*edgeL * static_cast<double>(d*d);
        force += delta * (distR / distG - 1.0);
    }
    return force;
}

glm::dvec3 GRIP::compute_FRforce(int ID, const vector<pair<int, int>>& n) const {
    glm::dvec3 force(0.0);
    glm::dvec3& POS = mr_g->vertices.at(ID).pos;
    for(auto& [OTHER_ID, _]:mr_g->adjList.at(ID)){
        glm::dvec3 delta = mr_g->vertices.at(OTHER_ID).pos - POS;
        double edgeL = find_edge_length(ID, OTHER_ID);
        double factor = glm::length2(delta) / (edgeL*edgeL);
        force += delta * factor;
    }
    for(auto& [OTHER_ID, d]:n){
        glm::dvec3 delta = POS - mr_g->vertices.at(OTHER_ID).pos;
        double edgeL = find_edge_length(ID, OTHER_ID);
        double factor = edgeL*edgeL / max<double>(glm::length2(delta), 1e-4);
        force += delta * factor * m_scalingFactor;
    }
    return force;
}


GRIP::GRIP(Graph& G) : mr_g(&G) {
    // fill edge lengths
    for(auto& e:mr_g->edges)
        if (e.length != 1)
            m_edgeLengths[sorted_pair<int>(e.start, e.end)] = e.length;
}

void GRIP::run()
{
    // for logic behind how the algorithm works read up on GRIP: Graph Drawing with Intelligent Placement
    // i recommend the original papers by Pawel Gajer, Michael T. Goodrich, and Stephen G. Kobourov
    // at the time of writing, all info can be found here: https://www2.cs.arizona.edu/~kobourov/GRIP/

    size_t N = mr_g->vertices.size(); // to simplify code

    // --- AVERAGE DEGREE ---

    double avgDegG = 0.0;
    for(auto& list:mr_g->adjList)
        avgDegG += static_cast<double>(list.size());
    avgDegG /= static_cast<double>(mr_g->adjList.size());

    // --- FILTERS ---
    // filters V_0, V_1, V_2 ... (V_i is a subset of V_i-1), filters[0] is V_0
    vector<vector<int>> filters;
    compute_filters(filters);
    size_t K = filters.size();

    // a structure for checking if element is a member of a filter
    vector<unordered_set<int>> filter_finder;
    for(auto& layer:filters)
        filter_finder.emplace_back(unordered_set<int>(layer.begin(), layer.end()));

    // --- NBRS array and neighbourhoods ---

    // TODO: check if nbrs is really meant to be implemented like this
    vector<size_t> nbrs(K);
    size_t maxComplexity = max(
        static_cast<size_t>(llround(static_cast<double>(N) * avgDegG)), 
        static_cast<size_t>(m_smallGraphLimit)
    );
    // find first level under the m_smallGraphLimit
    size_t smallLevel = K;
    for (size_t i = 0; i < K; ++i) {
        size_t L = filters[i].size();
        if (pow(L,2) <= static_cast<size_t>(m_smallGraphLimit)) {
            smallLevel = i;
            break; 
        }
    }
    for (size_t i = 0; i < K; ++i) {
        size_t L = filters[i].size();
        if (i >= smallLevel)
            nbrs[i] = L-1;
        else{
            double schedule = clamp<double>(static_cast<double>(i)/static_cast<double>(m_smallGraphLimit), 1.0, 2.0);
            nbrs[i] = min<size_t>(static_cast<size_t>(schedule * static_cast<double>(maxComplexity) / static_cast<double>(L)), L-1);
        }
        nbrs[i] = max<size_t>(nbrs[i], 1);
    }
    if (K != 0) nbrs[0] = min(nbrs[0] * 2, N - 1); // double first layer

    
    // neighbourhoods for every vertex indexed by their ID
    // neighbourhoods[6][1] means neighbourhood of vertex with ID 6 for V_1 (hence N_1)
    // neighbourhood contains a vector of pairs, first is a vertex, and second is its distance
    vector<vector<vector<pair<int, int>>>> neighbourhoods(N); 
    
    // ----------------------
    // --- GRIP ALGORIHTM ---
    // ----------------------
    
    // helper vectors for heat and displacement calculations, all are indexed by vertex id
    vector<glm::dvec3> displacements(N, glm::dvec3(0.0));
    vector<double> oldCos(N, 0.0); // a cosine angle between a previous displacement and a new one
    vector<double> heat(N, m_defaultEdgeLength/6.0); // default heat is a sixth of edge length

    // a set to help track which vertices have already been placed and which havent
    unordered_set<int> placed_id;

    for(int i = static_cast<int>(K-1); i >= 0; i--){ // start with smaller filters and progress to larger filters (to i = 0)
        vector<int>& current_filter = filters.at(i);

        for(int ID:current_filter){ // setup new vertices
            if (placed_id.find(ID) != placed_id.end()) continue; // do not place twice
            compute_vertex_neighbourhoods(ID, neighbourhoods.at(ID), nbrs, filter_finder, i, placed_id);
            if (i != static_cast<int>(K-1))
                vertex_initial_placement(ID, neighbourhoods.at(ID).at(i), placed_id);
            placed_id.insert(ID);
        }
        if (i == static_cast<int>(K-1))
            base_filter_placement(current_filter);

        for(int r = 0; r < m_rounds_number; r++){
            for(int ID:current_filter){
                glm::dvec3 force;
                if (i == 0) force = compute_FRforce(ID, neighbourhoods[ID][i]); // last filter special treatment
                else force = compute_KKforce(ID, neighbourhoods[ID][i]);

                if (glm::length(force) < 1e-4) continue;

                calc_temp(heat[ID], oldCos[ID], displacements[ID], force);
                displacements[ID] = heat[ID] * glm::normalize(force);
            }
            for(int ID:current_filter)
                mr_g->vertices.at(ID).pos += displacements[ID];
        }

        for(int ID:current_filter){
            auto& POS = mr_g->vertices.at(ID).pos;
            double l = glm::length(POS);
            if (l != l) spdlog::warn("Grip: NaN Detected for ID {} at filter {} of {}", ID, i, K-1);
        }
    }

    // --- GRAPH POST PROCESSING ---
    glm::dvec3 bc = mr_g->calculateBarycenter();
    mr_g->translate(bc);
}