#include "GRIP.hpp"

using namespace std;
using namespace stdpp;

float GRIP::find_dist(int id1, int id2) const {
    static unordered_map<sorted_pair<int>, int> alr_found;
    
    sorted_pair refpair(id1, id2);
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

        sorted_pair newpair(id, id1);
        alr_found[newpair] = d;

        if (id == id2){
            alr_found[refpair] = d;
            if (d <= 0.f) grip_error("Error: Impossible distance between 2 vertices");
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

float GRIP::find_edge_length(int id1, int id2) const {
    sorted_pair p(id1, id2);
    if (m_edgeLengths.find(p) == m_edgeLengths.end()) return m_defaultEdgeLength;
    else return m_defaultEdgeLength * m_edgeLengths.at(p);
}

void GRIP::grip_error(const string& description) const {
    spdlog::error("GRIP error: {}", description);
    throw runtime_error("GRIP error (check logs)");
}

void GRIP::compute_vertex_neighbourhoods(
    int ID, vector<vector<pair<int, int>>>& n, const vector<size_t> nbrs, 
    const vector<unordered_set<int>>& f_c, const int K, const unordered_set<int>& placed
) const {
    queue<int> q; // BFS queue that contains int value pairs (id, depth)
    unordered_set<int> visited{ID};
    for(auto& el:m_adjListG.at(ID)){ // init with vertices next to v
        q.push(el); q.push(1);
    }

    n.resize(K+1);
    for(int i = 0; i <= K; i++){
        while(n[i].size() < nbrs[i] && !q.empty()){
            int id = q.front(); q.pop(); // extract index of vertex in question
            int d = q.front(); q.pop(); // extract depth vertex is on

            if (visited.find(id) != visited.end()) continue;
            visited.emplace(id);

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
    float x2 = (dist01*dist01 + dist02*dist02 - dist12*dist12) / dist01 / 2.f;
    float y2 = std::sqrt(dist02*dist02 - x2*x2);
    mr_graph->vertices[base[2]].pos = glm::vec3(x2, y2, 0.f);
    
    if (base.size() == 4 && m_dimensions+1 == 4) return;
    float dist03 = find_dist(base[0], base[3]);
    float dist13 = find_dist(base[1], base[3]);
    float dist23 = find_dist(base[2], base[3]);

    if (abs(y2) < 1e-5){ // if first three were colinear
        float x3 = (dist01*dist01 + dist02*dist02 - dist12*dist12) / dist01 / 2.f;
        float y3 = std::sqrt(dist02*dist02 - x3*x3);
        mr_graph->vertices[base[2]].pos = glm::vec3(x3, y3, 0.f);
    }
    else{
        float x3 = (dist01*dist01 + dist03*dist03 - dist13*dist13) / (2.0f * dist01);
        float y3 = (x2*x2 + y2*y2 + dist03*dist03 - dist23*dist23 - 2.0f*x2*x3) / (2.0f * y2);
        float z3 = std::sqrt(dist03*dist03 - x3*x3 - y3*y3);
        mr_graph->vertices[base[3]].pos = glm::vec3(x3, y3, z3);
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
    
    // we do a placement using a simple barycenter method, this is because in the original paper two methods are outlined,
    // simple bc is one and the other is three closest neighbours which is explained in 2D and i dont understand how to apply in 3D
    // this is because in 3D the equations for that method have infinitely many solutions due to the added dimension
    // this could be changed with adding a 4th point so we get a tetrahedron, but nothing is said about that in the paper
    // in any case, testing that will be a TODO, and we will stick to the barycenter calculation of 3 neighbours

    glm::vec3 bc(0.f);
    for(int i = 0; i < found; i++) bc += mr_graph->vertices.at(n[ids[i]].first).pos;
    bc /= static_cast<float>(found);

    const float mag = 1e-3f * m_defaultEdgeLength;
    static uniform_real_distribution<float> dist(-mag, mag); // dont forget to change this if you make edgeLength changeable
    glm::vec3 jitter(dist(m_rng), dist(m_rng), dist(m_rng));

    mr_graph->vertices.at(ID).pos = bc + jitter;
}

void GRIP::calc_temp(float& oldTemp, float& oldCos, const glm::vec3& oldDisp, const glm::vec3& force) const {
    if (glm::length(oldDisp) == 0.f) return;
    
    float heat = oldTemp, cos = glm::dot(force, oldDisp) / (glm::length(force) * glm::length(oldDisp));
    if (oldCos * cos > 0.f)
        heat += oldTemp * cos * m_temperatureGain * m_temperatureNarrowGainIncrease;
    else
        heat += oldTemp * cos * m_temperatureGain;
    
    heat = std::max<float>(0.f, heat);

    oldCos = cos;
    oldTemp = heat;
}

glm::vec3 GRIP::compute_KKforce(int ID, const vector<pair<int, int>>& n) const {
    glm::vec3 force(0.f);
    glm::vec3& POS = mr_graph->vertices.at(ID).pos;
    for(auto& [OTHER_ID, d]:n){
        glm::vec3 delta = mr_graph->vertices.at(OTHER_ID).pos - POS;
        float distR = glm::length2(delta);
        float edgeL = find_edge_length(ID, OTHER_ID);
        float distG = glm::pow<float>(edgeL * static_cast<float>(d), 2);
        force += delta * (distR / distG - 1.f);
    }
    return force;
}

glm::vec3 GRIP::compute_FRforce(int ID, const vector<pair<int, int>>& n) const {
    glm::vec3 force(0.f);
    glm::vec3& POS = mr_graph->vertices.at(ID).pos;
    for(auto& OTHER_ID:m_adjListG.at(ID)){
        glm::vec3 delta = mr_graph->vertices.at(OTHER_ID).pos - POS;
        float edgeL = find_edge_length(ID, OTHER_ID);
        float factor = glm::length2(delta) / (edgeL*edgeL);
        force += delta * factor;
    }
    for(auto& [OTHER_ID, d]:n){
        glm::vec3 delta = POS - mr_graph->vertices.at(OTHER_ID).pos;
        float edgeL = find_edge_length(ID, OTHER_ID);
        float factor = edgeL*edgeL / std::max<float>(glm::length2(delta), 1e-4f);
        force += delta * factor * m_scalingFactor;
    }
    return force;
}


GRIP::GRIP(Graph& g) : mr_graph(&g) {
    for(size_t i = 0; i < mr_graph->vertices.size(); i++) // touch all of the indices so they at least exist
        m_adjListG[i] = vector<int>();
    for(size_t i = 0; i < mr_graph->edges.size(); i++){ // fill in the connections
        auto& e = mr_graph->edges.at(i);
        m_adjListG[e.start].push_back(e.end);
        m_adjListG[e.end].push_back(e.start);
    }

    // there must not be disjoint unions for GRIP to work
    // we must separate the graph into multiple graphs if it is a disjoint union
    unordered_set<int> visited_ind;
    for(auto& vertex:mr_graph->vertices){
        if (visited_ind.find(vertex.id) != visited_ind.end()) continue;

        mr_noDsu_Graphs.emplace_back();
        vector<int>& current_graph = mr_noDsu_Graphs.back();

        queue<int> q; q.push(vertex.id);
        while(!q.empty()){
            int current = q.front(); q.pop();
            if (visited_ind.find(current) != visited_ind.end()) continue;

            current_graph.push_back(current);

            visited_ind.insert(current);
            for(auto& e:m_adjListG[current])
                if (visited_ind.find(e) == visited_ind.end())
                    q.push(e);
        }
    }

    // fill edge lengths
    for(auto& e:mr_graph->edges)
        if (e.length != 1)
            m_edgeLengths[sorted_pair<int>(e.start, e.end)] = e.length;
}

void GRIP::runGraph(int graphId)
{
    // for logic behind how the algorithm works read up on GRIP: Graph Drawing with Intelligent Placement
    // i recommend the original papers by Pawel Gajer, Michael T. Goodrich, and Stephen G. Kobourov
    // at the time of writing, all info can be found here: https://www2.cs.arizona.edu/~kobourov/GRIP/

    size_t N = mr_graph->vertices.size(); // to simplify code
    vector<int>& G = mr_noDsu_Graphs.at(graphId);

    // --- AVERAGE DEGREE ---

    float avgDegG = 0.f;
    for(auto& ind:G)
        avgDegG += static_cast<float>(m_adjListG[ind].size());
    avgDegG /= static_cast<float>(m_adjListG.size());

    // ------------------------
    // --- CREATING FILTERS ---
    // ------------------------

    // filters V_0, V_1, V_2 ... (V_i is a subset of V_i-1), filters[0] is V_0
    vector<vector<int>> filters;
    filters.push_back(G); // create first filter layer (identical to the whole graph)
    
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
                    layer_copy.pop_back(); // and pop so removal is O(1)
                    layer_copy_indmap[layer_copy[index]] = index; // update the index value of the swapped element
                }

                if (d < maxDepth){
                    for(auto& v:m_adjListG.at(id)){
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
    if (K >= 2 && N >= static_cast<size_t>(m_dimensions+1))
    {
        // fill with the elements in the previous filter which are farthest away
        vector<int>& last_filter = filters.back();
        vector<int>& almost_last_filter = filters.at(K-2);
        while(last_filter.size() < static_cast<size_t>(m_dimensions+1))
        {
            float maxdist = 0.f; int saved_v;
            set<int> f_back_has(last_filter.begin(), last_filter.end());
            for(auto v:almost_last_filter){
                if (f_back_has.find(v) != f_back_has.end()) continue; // dont duplicate nodes

                // calc the avg graph distance from all base nodes
                float thisdist = 0.f;
                for(auto u:last_filter) thisdist += find_dist(u, v);
                thisdist /= static_cast<float>(last_filter.size());

                // if bigger save as farthest element yet
                if (thisdist > maxdist){
                    maxdist = thisdist;
                    saved_v = v;
                }
            }
            last_filter.push_back(saved_v); // and add the farthest element
        }
    }

    // a structure for checking if element is a member of a filter
    vector<unordered_set<int>> filter_finder;
    for(auto& layer:filters)
        filter_finder.emplace_back(unordered_set<int>(layer.begin(), layer.end()));

    // --- NBRS array and neighbourhoods ---

    // TODO: check if nbrs is really meant to be implemented like this
    vector<size_t> nbrs(K);
    size_t maxComplexity = max(
        static_cast<size_t>(llround(static_cast<float>(N) * avgDegG)), 
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
            float schedule = clamp<float>((float)i/(float(m_smallGraphLimit)), 1.f, 2.f);
            nbrs[i] = min<size_t>((size_t)floor(schedule * (float)maxComplexity / (float)L), L-1);
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
    vector<glm::vec3> displacements(N, glm::vec3(0.f));
    vector<float> oldCos(N, 0.f); // a cosine angle between a previous displacement and a new one
    vector<float> heat(N, m_defaultEdgeLength/6.f); // default heat is a sixth of edge length

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
                glm::vec3 force;
                if (i == 0) force = compute_FRforce(ID, neighbourhoods[ID][i]); // last filter special treatment
                else force = compute_KKforce(ID, neighbourhoods[ID][i]);

                if (glm::length(force) < 1e-4) continue;

                calc_temp(heat[ID], oldCos[ID], displacements[ID], force);
                displacements[ID] = heat[ID] * glm::normalize(force);
            }
            for(int ID:current_filter)
                mr_graph->vertices.at(ID).pos += displacements[ID];
        }

        for(int ID:current_filter){
            auto& POS = mr_graph->vertices.at(ID).pos;
            float l = glm::length(POS);
            if (l != l) spdlog::warn("Grip: NaN Detected for ID {} at filter {} of {}", ID, i, K-1);
        }
    }
}

void GRIP::run(){
    double totalRadius = 0.f;
    vector<pair<double, int>> graphRadii; // pairs of nodsu graph id and its radius
    for(size_t i = 0; i < mr_noDsu_Graphs.size(); i++){
        runGraph(i);

        // calculate the sphere barycenter
        glm::dvec3 bc(0.0);
        for(auto ID:mr_noDsu_Graphs.at(i))
            bc += mr_graph->vertices.at(ID).pos;
        bc /= static_cast<double>(mr_noDsu_Graphs.at(i).size());
        
        // and its radius
        double radius = 0.0;
        for(auto ID:mr_noDsu_Graphs.at(i)){
            glm::dvec3 loc = mr_graph->vertices.at(ID).pos;
            radius = std::max<double>(radius, glm::distance(bc, loc));
        }

        radius += 0.5; // add some clearance

        // center the graph
        for(auto ID:mr_noDsu_Graphs.at(i))
            mr_graph->vertices.at(ID).pos -= bc;
        
        graphRadii.emplace_back(std::make_pair(radius, i));
        totalRadius += radius;
    }

    std::sort(graphRadii.begin(), graphRadii.end());
    
    // now allign then all
    double sqrad = std::sqrt(totalRadius);
    double ypos = 0.0, xpos = 0.0;
    for(auto& [radius, GraphID]:graphRadii){      
        if (xpos + radius > sqrad && xpos != 0.0){
            xpos = 0.0;
            ypos += radius;
        }
        
        for(auto ID:mr_noDsu_Graphs.at(GraphID))
            mr_graph->vertices.at(ID).pos += glm::vec3(xpos + radius, ypos + radius, 0.f);
        
        xpos += radius;
    }

}