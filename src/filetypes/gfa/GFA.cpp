#include "GFA.hpp"

void GFA::parser_warning(const std::string& description, const int line_n) const {
    if (line_n != -1) spdlog::warn("GFA Parser warning [at line {}]: {}", line_n, description);
    else spdlog::warn("GFA Parser warning: {}", description);
}

[[noreturn]]
void GFA::parser_error(const std::string& description, const int line_n) const {
    if (line_n != -1) spdlog::error("GFA Parser error [at line {}]: {}", line_n, description);
    else spdlog::error("GFA Parser error: {}", description);
    throw std::runtime_error("GFA Parser error (check logs)");
}

GFA::GFA(const std::string& path, bool minimalMemory) : parser(path)
{
    // some lambda helpers we will need

    // a lambda for extracting layer TAG:TYPE:VALUE data out of a tag fragment
    auto getTTV = [this](const std::string& field, const int line_n) -> GFA_field {
        std::stringstream ss(field);
        std::string tag_type_value[3];
        for(int i = 0; i < 3; i++){
            std::getline(ss, tag_type_value[i], ':');
            if (ss.fail())
                parser_error("\n\tExpected string of shape 'TAG:TYPE:VALUE' but got: " + field + "\n", line_n);
        }
        return {tag_type_value[0], tag_type_value[1], tag_type_value[2]};
    };

    // check and convert (cnc) integer (i) value
    auto CNC_I = [this](const GFA_field& f, const char* tag, const std::string& field_str, int line_n) -> std::optional<long long int> {
        if (f.tag != tag) return std::nullopt;
        if (f.type != "i") parser_error("Expected type 'i' for '" + std::string(tag) + "'\n\tfull field: " + field_str, line_n);
        try {
            return std::stoll(f.value);
        } catch (const std::exception& e) {
            parser_error("Invalid integer value\n\tfull field: " + field_str, line_n);
        }
        __builtin_unreachable(); // stfu compiler, we will never end up here
    };

    // check (c) hash (H) value
    auto C_H = [this](const GFA_field& f, const char* tag, const std::string& field_str, int line_n) -> std::optional<std::string> {
        if (f.tag != tag) return std::nullopt;
        if (f.type != "H") parser_error("Expected type 'H' for '" + std::string(tag) + "'\n\tfull field: " + field_str, line_n);
        // TODO: validate hex format
        return f.value;
    };

    // check (c) printable string (Z) value
    auto C_Z = [this](const GFA_field& f, const char* tag, const std::string& field_str, const int& line_n) -> std::optional<std::string> {
        if (f.tag != tag) return std::nullopt;
        if (f.type != "Z") parser_error("Expected type 'Z' for '" + std::string(tag) + "'\n\tfull field: " + field_str, line_n);
        if (f.value.empty()) parser_error("Empty string value\n\tfull field: " + field_str, line_n);
        return f.value;
    };

    auto parseOrientation = [this](const std::string& ori_str, const std::string& fieldName, int line_n) -> bool {
        if (ori_str == "+") return true;
        if (ori_str == "-") return false;
        parser_error("field '" + fieldName + "' can only be + or - however, '" + ori_str + "' was provided", line_n);
        __builtin_unreachable(); // stfu compiler, we will never end up here
    };

    // enum signifying what kind of an exit occured
    enum readExit{TAB, NEWLINE, FILEEND};
    // return the next string before file end/tab delimitor/next line
    auto readNextString = [](std::ifstream& file, size_t reserve=64) -> std::tuple<std::string, readExit> {
        std::string ret;
        ret.reserve(reserve);
        readExit breakSignal = FILEEND;
        for (auto c = file.get();; c = file.get()){
            if (c == '\t') {breakSignal = TAB; break;}
            if (c == '\n') {breakSignal = NEWLINE; break;}
            if (!file.good()) {breakSignal = FILEEND; break;}
            ret += static_cast<char>(c);
        }
        return {ret, breakSignal};
    };
    // ignore the next string before file end/tab delimitor/next line
    auto ignoreNextString = [](std::ifstream& file, size_t* num = nullptr) -> readExit {
        if (num != nullptr) *num = 0;
        for (auto c = file.get(); file.good(); c = file.get()){
            if (c == '\t') return TAB;
            if (c == '\n') return NEWLINE;
            if (num != nullptr) *num += 1;
        }
        return FILEEND;
    };

    auto skipTabs = [](std::ifstream& file) -> void {
        while(file.peek() == '\t') file.get();
    };

    // first we will do some preprocessing

    // open the requested file
    std::ifstream file(path);
    if (!file.is_open())
        parser::parser_error("File not found: " + path);

    // to better understand what the following parser does, read https://github.com/GFA-spec/GFA-spec

    // remember what is stored at which lines
    std::unordered_map<char, std::vector<std::pair<std::streampos, size_t>>> lines;

    size_t lineNum = 0;
    for (char c; file.get(c); lineNum++) {
        lines[c].emplace_back(std::make_pair(file.tellg(), lineNum));
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip to the next line
    }
    file.clear();

    // do the header line before anything else
    for (auto [seekpos, line_n] : lines['H']){
        file.seekg(seekpos); skipTabs(file);

        // check optional fields
        for(auto [field_str, field_sig] = readNextString(file);;
            std::tie(field_str, field_sig) = readNextString(file))
        {
            GFA_field f = getTTV(field_str, line_n);
            if (auto v = C_Z(f, "VN", field_str, line_n)) version_string = *v;

            if (field_sig != TAB) break;
        }
    }

    file.close(); // we wont need this now

    // we need this to work with large files, and so we will have to reserve memory to avoid vector overshooting
    links.resize(lines['L'].size());
    segments.resize(lines['S'].size());
    paths.resize(lines['P'].size());
    containments.resize(lines['C'].size());

    segment_associations.resize(segments.size());

    // process segments first
    #ifdef HAS_OPENMP
        unsigned int available_threads = std::max<unsigned int>(std::thread::hardware_concurrency()-2 , 2);
        spdlog::info("splitting parser the workload, max threads: {}", available_threads);
    
        #pragma omp parallel num_threads(available_threads)
    #endif
    {
        std::ifstream local_file(path);

        #ifdef HAS_OPENMP
            #pragma omp for schedule(dynamic, 1)
        #endif
        for (size_t i = 0; i < lines['S'].size(); i++){
            auto [seekpos, line_n] = lines['S'][i];
            local_file.seekg(seekpos); skipTabs(local_file);

            auto [name, signal] = readNextString(local_file);
            if (signal != TAB){
                parser_warning("Segment with name: '" + name + "' lacks data (only name provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            // create the segment and reference it
            segments[i] = GFA_segment(name);
            GFA_segment& segment = segments[i]; 

            // the program doesnt need to store an entire sequence in ram
            // it can just store a reference to where it can find it if needed and get the data at the moment it gets requested
            if (local_file.good() && local_file.peek() == '*') segment.noSequence();
            else segment.setSequence(path, local_file.tellg());
            size_t segsize = 0;
            signal = ignoreNextString(local_file, &segsize);
            segment.setSegmentLength(segsize);
            if (signal != TAB) continue; // other things are optional
            skipTabs(local_file);

            // check optional fields
            for(auto [field_str, field_sig] = readNextString(local_file);;
                std::tie(field_str, field_sig) = readNextString(local_file))
            {
                GFA_field f = getTTV(field_str, line_n);
                if (auto v = CNC_I(f, "LN", field_str, line_n)) segment.setSegmentLength(*v);
                if (auto v = CNC_I(f, "RC", field_str, line_n)) segment.setReadCount(*v);
                if (auto v = CNC_I(f, "FC", field_str, line_n)) segment.setFragmentCount(*v);
                if (auto v = CNC_I(f, "KC", field_str, line_n)) segment.setKmerCount(*v);
                if (auto v = C_H(f, "SH", field_str, line_n)) segment.setHash(*v);
                if (auto v = C_Z(f, "UR", field_str, line_n)) segment.setUriSequence(*v, path);

                if (field_sig != TAB) break;
            }
        }

        #ifdef HAS_OPENMP
            #pragma omp barrier
            #pragma omp single
        #endif
        { // make the lookup
            for (size_t i = 0; i < segments.size(); i++) segment_lookup[segments[i].getName()] = i;
        }

        // then links
        #ifdef HAS_OPENMP
            #pragma omp for schedule(dynamic, 1)
        #endif
        for (size_t i = 0; i < lines['L'].size(); i++){
            auto [seekpos, line_n] = lines['L'][i];
            local_file.seekg(seekpos); skipTabs(local_file);

            auto [fromName, signal_fn] = readNextString(local_file);
            auto it_fn = segment_lookup.find(fromName);
            if (it_fn == segment_lookup.end()) {
                parser_warning("Unknown segment: " + fromName, line_n);
                continue;
            }
            size_t fromID = it_fn->second;

            if (signal_fn != TAB){
                parser_warning("Link starting at: '" + fromName + "' lacks data (only this name provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [fromOri_str, signal_fos] = readNextString(local_file);
            bool fromOri = parseOrientation(fromOri_str, "From Orientation", line_n);
            if (signal_fos != TAB){
                parser_warning("Link starting at: '" + fromName + "' lacks data (only name and orientation provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [toName, signal_tn] = readNextString(local_file);
            auto it_tn = segment_lookup.find(toName);
            if (it_tn == segment_lookup.end()) {
                parser_warning("Unknown segment: " + toName, line_n);
                continue;
            }
            size_t toID = it_tn->second;

            if (signal_tn != TAB){
                parser_warning("Link starting at: '" + fromName + "' lacks data (only names provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [toOri_str, signal_tos] = readNextString(local_file);
            bool toOri = parseOrientation(toOri_str, "To Orientation", line_n);
            if (signal_tos != TAB){
                parser_warning("Link starting at: '" + fromName + "' lacks CIGAR data", line_n);
                continue;
            }
            skipTabs(local_file);
            
            links[i] = GFA_link(fromID, fromOri, toID, toOri);
            GFA_link& link = links[i];
            
            // the program doesnt need to store an entire cigar string in ram
            // it can just store a reference to where it can find it if needed and get the data at the moment it gets requested
            if (local_file.good() && local_file.peek() == '*') link.noOverlap();
            else link.setOverlap(path, local_file.tellg());
            auto signal = ignoreNextString(local_file);
            if (signal != TAB) continue; // other things are optional
            skipTabs(local_file);

            // check optional fields
            for(auto [field_str, field_sig] = readNextString(local_file);;
                std::tie(field_str, field_sig) = readNextString(local_file))
            {
                GFA_field f = getTTV(field_str, line_n);
                if (auto v = CNC_I(f, "MQ", field_str, line_n)) link.setMappingQuality(*v);
                if (auto v = CNC_I(f, "NM", field_str, line_n)) link.setNumOfMismatchsGaps(*v);
                if (auto v = CNC_I(f, "RC", field_str, line_n)) link.setReadCount(*v);
                if (auto v = CNC_I(f, "FC", field_str, line_n)) link.setFragmentCount(*v);
                if (auto v = CNC_I(f, "KC", field_str, line_n)) link.setKmerCount(*v);
                if (auto v = C_Z(f, "ID", field_str, line_n)) link.setEdgeIdentifier(*v);

                if (field_sig != TAB) break;
            }
        }
        
        #ifdef HAS_OPENMP
            #pragma omp barrier
            #pragma omp single
        #endif
        { // make segment associations to links
            for (size_t i = 0; i < links.size(); i++){
                auto p = std::make_pair(i, mapType::LINK);
                segment_associations.at(links[i].getFromID()).push_back(p);
                segment_associations.at(links[i].getToID()).push_back(p);
            }
        }

        #ifdef HAS_OPENMP
            #pragma omp for schedule(dynamic, 1)
        #endif
        for (size_t i = 0; i < lines['P'].size(); i++){
            if(minimalMemory) continue;

            auto [seekpos, line_n] = lines['P'][i];
            local_file.seekg(seekpos); skipTabs(local_file);

            auto [name, signal_nam] = readNextString(local_file);
            if (signal_nam != TAB){
                parser_warning("Path with name: '" + name + "' lacks segment data", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [segmentString, signal_seg] = readNextString(local_file);
            if (signal_seg != TAB){
                parser_warning("Path with name: '" + name + "' lacks overlap data", line_n);
                continue;
            }
            skipTabs(local_file);

            std::vector<size_t> segIDs;
            std::vector<bool> oris;
            std::stringstream ss(segmentString);
            for(std::string S; getline(ss, S, ','); ){
                if (S.size() < 2) continue;
                std::string SS = S.substr(0, S.size()-1);

                auto it = segment_lookup.find(SS);
                if (it == segment_lookup.end()) {
                    parser_warning("Unknown segment: " + SS, line_n);
                    continue;
                }
                size_t id = it->second;

                segIDs.push_back(id);
                if (S.back() == '+') oris.emplace_back(true);
                else oris.emplace_back(false);
            }

            paths[i] = GFA_path(name, std::move(segIDs), std::move(oris));
            GFA_path& Path = paths[i];
            
            // the program doesnt need to store an entire cigar string in ram
            // it can just store a reference to where it can find it if needed and get the data at the moment it gets requested
            if (local_file.good() && local_file.peek() == '*') Path.noOverlap();
            else Path.setOverlap(path, local_file.tellg());
            auto signal = ignoreNextString(local_file);
            if (signal == TAB) // other things are to be ignored
                local_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        #ifdef HAS_OPENMP
            #pragma omp barrier
            #pragma omp single
        #endif
        {   // make the path lookup
            for(size_t i = 0; i < paths.size(); i++) path_lookup[paths[i].getName()] = i;
        }
        
        #ifdef HAS_OPENMP
            #pragma omp barrier

            #pragma omp for schedule(dynamic, 1)
        #endif
        for (size_t i = 0; i < lines['C'].size(); i++){
            if(minimalMemory) continue;

            auto [seekpos, line_n] = lines['C'][i];
            local_file.seekg(seekpos); skipTabs(local_file);

            auto [containerName, signal_cn] = readNextString(local_file);
            auto it_cn = segment_lookup.find(containerName);
            if (it_cn == segment_lookup.end()) {
                parser_warning("Unknown segment: " + containerName, line_n);
                continue;
            }
            size_t containerID = it_cn->second;

            if (signal_cn != TAB){
                parser_warning("Container in: '" + containerName + "' lacks data (only container name provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [containerOri_str, signal_cos] = readNextString(local_file);
            bool containerOri = parseOrientation(containerOri_str, "Container Orientation", line_n);
            if (signal_cos != TAB){
                parser_warning("Container in: '" + containerName + "' lacks data (only container name provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [containedName, signal_cdn] = readNextString(local_file);
            auto it_cdn = segment_lookup.find(containedName);
            if (it_cdn == segment_lookup.end()) {
                parser_warning("Unknown segment: " + containedName, line_n);
                continue;
            }
            size_t containeDID = it_cdn->second;

            if (signal_cdn != TAB){
                parser_warning("Container in: '" + containerName + "' lacks data (only names provided)", line_n);
                continue;
            }
            skipTabs(local_file);

            auto [containedOri_str, signal_cdos] = readNextString(local_file);
            bool containedOri = parseOrientation(containedOri_str, "ContaineD Orientation", line_n);
            if (signal_cdos != TAB){
                parser_warning("Container in: '" + containerName + "' lacks data (only names/orientations provided)", line_n);
                continue;
            }
            skipTabs(local_file);


            auto [pos_str, signal_ps] = readNextString(local_file);
            long long int pos = 0;
            try{
                pos = std::stoll(pos_str);
            } catch (const std::exception& e) {
                parser_warning("Container in: '" + containerName + "' has incorrect positon data: " + e.what(), line_n);
            }
            if (signal_ps != TAB){
                parser_warning("Container in: '" + containerName + "' lacks CIGAR data", line_n);
                continue;
            }
            skipTabs(local_file);

            containments[i] = GFA_containment(containerID, containerOri, containeDID, containedOri, pos);
            GFA_containment& containment = containments[i];

            if (local_file.good() && local_file.peek() == '*') containment.noOverlap();
            else containment.setOverlap(path, local_file.tellg());
            auto signal = ignoreNextString(local_file);
            if (signal != TAB) continue; // other things are optional
            skipTabs(local_file);

            // check optional fields
            for(auto [field_str, field_sig] = readNextString(local_file);;
                std::tie(field_str, field_sig) = readNextString(local_file))
            {
                GFA_field f = getTTV(field_str, line_n);
                if (auto v = CNC_I(f, "RC", field_str, line_n)) containment.setReadCount(*v);
                if (auto v = CNC_I(f, "NM", field_str, line_n)) containment.setNumOfMismatchsGaps(*v);
                if (auto v = C_Z(f, "ID", field_str, line_n)) containment.setEdgeIdentifier(*v);

                if (field_sig != TAB) break;
            }
        }

        local_file.close();
    }

    size_t knownTypes = lines.count('S') + lines.count('L') + lines.count('P') + lines.count('C') + lines.count('H') + lines.count('#');
    size_t incompatibleLines = lines.size() - knownTypes;
    if (incompatibleLines)
        parser_warning(std::to_string(incompatibleLines) + " unrecognized lines in GFA file");

    // readjust vectors just in case
    segments.shrink_to_fit();
    links.shrink_to_fit();
    containments.shrink_to_fit();
    paths.shrink_to_fit();
}

void GFA::fillData(std::vector<Vertex>& v, std::vector<Edge>& e) {
    std::unordered_map<std::string, size_t> verts;

    for(size_t i = 0; i < segments.size(); i++){
        auto& s = segments.at(i);
        size_t vid = v.size(); // this is not an edge id not an object id, its an id of a vector in the graph
        v.emplace_back(vid);
        verts[s.getName() + "START"] = vid;
        v.emplace_back(vid + 1);
        verts[s.getName() + "END"] = vid + 1;
        size_t eid = e.size(); // this is an edge id
        e.emplace_back(eid, vid, vid + 1, s.getSegmentLength());
        e.back().segPart = true;
        edgeMap[eid] = std::make_pair(i, mapType::SEGMENT); // make an entry that links that eid with this segment
        s.setExternalID(eid);
    }

    for(size_t i = 0; i < links.size(); i++){
        auto& l = links.at(i);
        size_t id1 = verts[segments.at(l.getFromID()).getName() + ((l.getFromOrientation() == '+') ? "END" : "START")];
        size_t id2 = verts[segments.at(l.getToID()).getName() + ((l.getToOrientation() == '+') ? "START" : "END")];
        size_t eid = e.size();
        e.emplace_back(eid, id1, id2);
        e.back().setOrientations(l.getFromOrientation() == '+', l.getToOrientation() == '+');
        edgeMap[eid] = std::make_pair(i, mapType::LINK); // make an entry that links that eid with this link
        l.setExternalID(eid);
    }
}

std::map<std::string, dataProperties> GFA::retrieveObjectData(size_t oid, char type, bool verbose) const {
    std::map<std::string, dataProperties> ret = {};
    
    if (type == mapType::SEGMENT){
        if (segments.size() <= oid) return {};
        auto& s = segments.at(oid);
        long long int KC = s.getKmerCount(), RC = s.getReadCount(), FC = s.getFragmentCount(), L = s.getSegmentLength();
        ret["Name_SW"] = std::string_view(s.getName());
        ret["Length_LLI"] = L;
        if (L > 0){
            if (KC >= 0) ret["Depth_D"] = static_cast<double>(KC) / static_cast<double>(L);
            else if (RC >= 0) ret["Depth_D"] = static_cast<double>(RC) / static_cast<double>(L);
            else if (FC >= 0) ret["Depth_D"] = static_cast<double>(FC) / static_cast<double>(L);
        }
        if (verbose){
            if (KC != -1) ret["KmerCount_LLI"] = KC;
            if (RC != -1) ret["ReadCount_LLI"] = RC;
            if (FC != -1) ret["FragmentCount_LLI"] = FC;
            ret["SequenceAvailable_B"] = s.isSequenceAvailable();
        }
    }
    else if (type == mapType::LINK){
        if (links.size() <= oid) return {};
        auto& l = links.at(oid);
        ret["FromName_SW"] = std::string_view(segments.at(l.getFromID()).getName());
        ret["FromOrientation_C"] = l.getFromOrientation();
        ret["ToName_SW"] = std::string_view(segments.at(l.getToID()).getName());
        ret["ToOrientation_C"] = l.getToOrientation();
        if (verbose){
            ret["CigarAvailable_B"] = l.isOverlapAvailable();
            ret["EdgeIdentifier_SW"] = std::string_view(l.getEdgeIdentifier());
            long long int KC = l.getKmerCount(), RC = l.getReadCount(), FC = l.getFragmentCount(),
                MMC = l.getNumOfMismatchGaps(), MQ = l.getMappingQuality();
            if (KC != -1) ret["KmerCount_LLI"] = KC;
            if (RC != -1) ret["ReadCount_LLI"] = RC;
            if (FC != -1) ret["FragmentCount_LLI"] = FC;
            if (MMC != -1) ret["MismatchGaps_LLI"] = MMC;
            if (MQ != -1) ret["MappingQuality_LLI"] = MQ;
        }
    }
    else if (type == mapType::CONTAINMENT){
        if (containments.size() <= oid) return {};
        auto& c = containments.at(oid);
        ret["ContainerName_SW"] = std::string_view(segments.at(c.getContainerID()).getName());
        ret["ContainerOrientation_C"] = c.getContainerOrientation();
        ret["ContainedName_SW"] = std::string_view(segments.at(c.getContainedID()).getName());
        ret["ContainedOrientation_C"] = c.getContainedOrientation();
        ret["Position_LLI"] = c.getPositon();
        if (verbose){
            ret["CigarAvailable_B"] = c.isOverlapAvailable();
            long long int MMC = c.getNumOfMismatchGaps(), RC = c.getReadCount();
            if (MMC != -1) ret["MismatchGaps_LLI"] = MMC;
            if (RC != -1) ret["ReadCount_LLI"] = RC;
            if (c.hasEdgeIdentifier()) ret["EdgeIdentifier_SW"] = std::string_view(c.getEdgeIdentifier());
        }
    }
    else if (type == mapType::PATH){
        if (paths.size() <= oid) return {};
        auto& p = paths.at(oid);
        ret["Name_SW"] = std::string_view(p.getName());
        if (verbose){
            ret["CigarAvailable_B"] = p.isOverlapAvailable();
        }
    }
    ret["Type_C"] = type;
    return ret;
}

std::map<std::string, dataProperties> GFA::retrieveEdgeData(size_t eid, bool verbose) const {
    auto it = edgeMap.find(eid);
    if (it == edgeMap.end()) return {};
    auto& [oID, type] = it->second;
    if (type != mapType::LINK && type != mapType::SEGMENT) return {}; // only these two have external edge ids (for now)
    return retrieveObjectData(oID, type, verbose);
}

std::map<std::string, dataProperties> GFA::retrieveGeneralData() const {
    std::map<std::string, dataProperties> ret = {};
    ret["GFA_version_SW"] = std::string_view(version_string);
    ret["SegmentNumber_ULLI"] = segments.size();
    ret["ContainmentNumber_ULLI"] = containments.size();
    ret["LinkNumber_ULLI"] = links.size();
    ret["PathNumber_ULLI"] = paths.size();
    return ret;
}

std::optional<std::tuple<std::filesystem::path, std::streampos>> GFA::retrieveSequence(size_t eid) const {
    auto it = edgeMap.find(eid);
    if (it == edgeMap.end()) return std::nullopt;
    if (it->second.second != mapType::SEGMENT) return std::nullopt;
    return segments.at(it->second.first).provideSequence();
}

std::optional<std::tuple<std::filesystem::path, std::streampos>> GFA::retrieveCIGAR(size_t eid) const {
    auto it = edgeMap.find(eid);
    if (it == edgeMap.end()) return std::nullopt;
    if (it->second.second != mapType::LINK) return std::nullopt;
    return links.at(it->second.first).provideOverlap();
}

std::vector<std::tuple<size_t, GFA::mapType, std::optional<size_t>>> GFA::searchStrictForName(const std::string& nameStr, char filters) const {
    std::vector<std::tuple<size_t, GFA::mapType, std::optional<size_t>>> res;

    if (filters & mapType::SEGMENT){
        auto it = segment_lookup.find(nameStr);
        if (it != segment_lookup.end()){
            auto ID = it->second;
            res.emplace_back(std::make_tuple(ID, mapType::SEGMENT, segments.at(ID).getExternalID()));
            if (filters & mapType::LINK || filters & mapType::CONTAINMENT){
                for(auto& el : segment_associations[ID]){
                    if (el.second == mapType::LINK)
                        res.emplace_back(std::make_tuple(el.first, mapType::LINK, links.at(el.first).getExternalID()));
                    else if (el.second == mapType::CONTAINMENT)
                        res.emplace_back(std::make_tuple(el.first, mapType::CONTAINMENT, std::nullopt));
                }
            }
        }
    }

    if (filters & mapType::PATH){
        auto it = path_lookup.find(nameStr);
        if (it != path_lookup.end())
            res.emplace_back(std::make_tuple(it->second, mapType::PATH, std::nullopt));
    }

    return res;
}

std::vector<std::tuple<size_t, GFA::mapType, std::optional<size_t>>> GFA::searchFuzzyForName(const std::string& nameStr, char filters) const {
    return {};
}