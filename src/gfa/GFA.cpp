#include "GFA.hpp"


void GFA::parser_error(const std::string& description, const int line_n) const {
    if (line_n != -1) spdlog::error("GFA Parser error [at line {}]: {}", line_n, description);
    else spdlog::error("GFA Parser error: {}", description);
    throw std::runtime_error("GFA Parser error (check logs)");
}
void GFA::parser_error(const std::string& description) const {
    parser_error(description, -1);
}

GFA::GFA(const std::string& path) : version_string("")
{
    // open the requested file
    std::ifstream file(path);
    if (!file.is_open())
        parser_error("File not found: " + path);

    // TODO: a lot of parser debugging

    // to better understand what the following parser does, read https://github.com/GFA-spec/GFA-spec

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

    // when checking fields, we will need to check for specific tag type value combinations, for each tag we will use a lambda
    // lambda definitions ahead...

    // check and convert (cnc) integer (i) value
    auto cnc_i = [this](const std::string& in, const std::string& field_str, const int& line_n) -> long long int {
        // convert value to ulli
        unsigned long long int val;
        try { 
            val = std::stoull(in);
        } catch(const std::exception& e){
            parser_error("Impossible to convert VALUE to integer\n\tfull field: \n\tException: " + std::string(e.what()) + field_str, line_n);    
        }
        return val;
    };

    // check (c) hash (H) value
    auto c_H = [this](const std::string& in, const std::string& field_str, const int& line_n){
        if (in.empty()) parser_error("H VALUE (hash) can't be empty\n\tfull field: " + field_str, line_n);
        // TODO: implement this
    };

    // check (c) printable string (Z) value
    auto c_Z = [this](const std::string& in, const std::string& field_str, const int& line_n){
        if (in.empty()) parser_error("Z VALUE (printable string) can't be empty\n\tfield_str: " + field_str, line_n);
        // TODO: implement this
    };


    // check version number field
    auto check_vn = [this, &c_Z](const GFA_field& f, const std::string& field_str, const int line_n){
        if (f.tag == "VN"){
            if (f.type == "Z"){
                c_Z(f.value, field_str, line_n);
                if (version_string.empty()) version_string = f.value;
                else parser_error("Redefinition of a version string (already previously defined)", line_n);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'Z'\n\tfull field:" + field_str, line_n);
        }
    };

    // check segment length field
    auto check_ln = [this, &cnc_i](const GFA_field& f, segmentLength& element, const std::string& field_str, const int line_n){
        if (f.tag == "LN"){
            if (f.type == "i") element.setSegmentLength(cnc_i(f.value, field_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check read count field
    auto check_rc = [this, &cnc_i](const GFA_field& f, readCount& element, const std::string& field_str, const int line_n){
        if (f.tag == "RC"){
            if (f.type == "i") element.setReadCount(cnc_i(f.value, field_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check fragment count field
    auto check_fc = [this, &cnc_i](const GFA_field& f, fragmentCount& element, const std::string& field_str, const int line_n){
        if (f.tag == "FC"){
            if (f.type == "i") element.setFragmentCount(cnc_i(f.value, field_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check k-mer count
    auto check_kc = [this, &cnc_i](const GFA_field& f, kmerCount& element, const std::string& field_str, const int line_n){
        if (f.tag == "KC"){
            if (f.type == "i") element.setKmerCount(cnc_i(f.value, field_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check sha256 checksum of the sequence
    auto check_sh = [this, &c_H](const GFA_field& f, hash& element, const std::string& field_str, const int line_n){
        if (f.tag == "SH"){
            if (f.type == "H"){
                c_H(f.value, field_str, line_n);
                element.setHash(f.value);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'H'\n\tfull field:" + field_str, line_n);
        }
    };
    
    // check the uri to the sequence
    auto check_ur = [this, &c_Z, &path](const GFA_field& f, GFA_segment& segment, const std::string& field_str, const int line_n){
        if (f.tag == "UR"){
            if (f.type == "Z"){
                c_Z(f.value, field_str, line_n);
                try{
                    segment.setSequence(f.value, path);
                } catch (const std::exception& e) {
                    parser_error("Wrong VALUE in field\n\tfull field: " + field_str + "\n\t" + std::string(e.what()), line_n);
                }
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'Z'\n\tfull field:" + field_str, line_n);
        }
    };

    // check mapping quality
    auto check_mq = [this, &cnc_i](const GFA_field& f, mappingQuality& element, const std::string& field_str, const int line_n){
        if (f.tag == "MQ"){
            if (f.type == "i") element.setMappingQuality(cnc_i(f.value, field_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check number of mismatches/gaps
    auto check_nm = [this, &cnc_i](const GFA_field& f, numOfMismatchsGaps& element, const std::string& field_str, const int line_n){
        if (f.tag == "NM"){
            if (f.type == "i") element.setNumOfMismatchsGaps(cnc_i(f.value, field_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check the edge identifier to the sequence
    auto check_id = [this, &c_Z](const GFA_field& f, edgeIdentifier& element, const std::string& field_str, const int line_n){
        if (f.tag == "ID"){
            if (f.type == "Z"){
                c_Z(f.value, field_str, line_n);
                element.setEdgeIdentifier(f.value);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'Z'\n\tfull field:" + field_str, line_n);
        }
    };

    // now we can finally implement the parser
    char first_char;
    for (int line_n = 1; file.get(first_char); line_n++){
        while(file.peek() == '\t') file.get(); // remove the tab(s)
        std::string field_str; // a string to store individual (tab delimited) fields

        // segments are expected to have astronomical sizes, and therefore must not be allowed to be read through a file
        if (first_char == 'S')
        {
            std::string name; // each segment has a name
            while (file.good() && file.peek() != '\n' && file.peek() != '\t') name += file.get(); // load the name
            segments.emplace_back(GFA_segment(name)); // create the segment
            GFA_segment& segment = segments.back(); // reference it
            segment_lookup.emplace(segment.getName(), segments.size()-1);
            while(file.peek() == '\t') file.get(); // remove the tab(s)
            try{
                segment.setSequence(file, path); // input the sequence through the stream
            } catch (const std::runtime_error& e) {
                parser_error("Error while examining the sequence:\n\t" + std::string(e.what()), line_n);
            }
            while(file.peek() == '\t') file.get(); // remove the tab(s)

            // now check optional fields
            for(char c = file.get(); file.good() && c != '\n'; c = file.get()){
                if (c != '\t' && c != '\n') field_str += c;
                else{
                    GFA_field f = getTTV(field_str, line_n);
                    check_ln(f, segment, field_str, line_n);
                    check_rc(f, segment, field_str, line_n);
                    check_fc(f, segment, field_str, line_n);
                    check_kc(f, segment, field_str, line_n);
                    check_sh(f, segment, field_str, line_n);
                    check_ur(f, segment, field_str, line_n);
                    field_str.clear(); // prepare for the next field
                }
            }
            continue;
        }
        else if (first_char == 'P')
        {
            std::string name;
            while (file.good() && file.peek() != '\n' && file.peek() != '\t') name += file.get(); // load the name
            paths.emplace_back(GFA_path(name));
            GFA_path& path = paths.back();
            while(file.peek() == '\t') file.get(); // remove the tab(s)
            try{
                path.setSegments(file);
            } catch(const std::exception& e){
                parser_error("Error while parsing path segments:\n\t" + std::string(e.what()));
            }
            while(file.peek() == '\t') file.get(); // remove the tab(s)
            path.setOverlaps(file, links, link_lookup);
            continue;
        }

        // for any other type we aren't expected to parse such a long input string so getlines and stringstreams can be used for simplifying the code
        // TODO: however switching from getline to character by character input may be a good idea performance-wise, but its not an urgent one

        std::string ln;
        std::getline(file, ln, '\n');
        std::stringstream ss(ln); // stringstream to load from

        if (first_char == '#') continue;
        else if (first_char == 'H')
        {
            while(std::getline(ss, field_str, '\t')){
                GFA_field f = getTTV(field_str, line_n);
                check_vn(f, field_str, line_n);
            }
        }
        
        else if (first_char == 'L')
        {
            std::string from, from_ori_str, to, to_ori_str, overlap;
            bool from_ori, to_ori;

            std::getline(ss, from, '\t'); // TODO: check against regex if this is ok
            std::getline(ss, from_ori_str, '\t'); // TODO: check against regex if this is ok
            if (from_ori_str == "+") from_ori = true;
            else if (from_ori_str == "-") from_ori = false;
            else parser_error("field 'from orientation' can only be + or - however, '" + from_ori_str + "' was provided", line_n);

            std::getline(ss, to, '\t'); // TODO: check against regex if this is ok
            std::getline(ss, to_ori_str, '\t'); // TODO: check against regex if this is ok
            if (to_ori_str == "+") to_ori = true;
            else if (to_ori_str == "-") to_ori = false;
            else parser_error("field 'to orientation' can only be + or - however, '" + from_ori_str + "' was provided", line_n);

            links.emplace_back(GFA_link(from, from_ori, to, to_ori));
            GFA_link& link = links.back();
            link_lookup.emplace(make_pair(link.getFromName(), link.getToName()), links.size()-1);

            std::getline(ss, overlap, '\t'); // TODO: check against regex if this is ok
            link.setOverlap(overlap);

            while(std::getline(ss, field_str, '\t')){
                GFA_field f = getTTV(field_str, line_n);
                check_mq(f, link, field_str, line_n);
                check_nm(f, link, field_str, line_n);
                check_rc(f, link, field_str, line_n);
                check_fc(f, link, field_str, line_n);
                check_kc(f, link, field_str, line_n);
                check_id(f, link, field_str, line_n);
            }
        }
        else if (first_char == 'C')
        {
            std::string container, container_ori_str, contained, contained_ori_str, pos_str, overlap;
            bool container_ori, contained_ori;
            int pos;

            std::getline(ss, container, '\t'); // TODO: check against regex if this is ok
            std::getline(ss, container_ori_str, '\t'); // TODO: check against regex if this is ok
            if (container_ori_str == "+") container_ori = true;
            else if (container_ori_str == "-") container_ori = false;
            else parser_error("field 'from orientation' can only be + or - however, '" + container_ori_str + "' was provided", line_n);

            std::getline(ss, contained, '\t'); // TODO: check against regex if this is ok
            std::getline(ss, contained_ori_str, '\t'); // TODO: check against regex if this is ok
            if (contained_ori_str == "+") contained_ori = true;
            else if (contained_ori_str == "-") contained_ori = false;
            else parser_error("field 'to orientation' can only be + or - however, '" + contained_ori_str + "' was provided", line_n);
            
            pos = cnc_i(pos_str, ln, line_n);

            containments.emplace_back(GFA_containment(container, container_ori, contained, contained_ori, pos));
            GFA_containment& containment = containments.back();

            std::getline(ss, overlap, '\t'); // TODO: check against regex if this is ok
            containment.setOverlap(overlap);

            while(std::getline(ss, field_str, '\t')){
                GFA_field f = getTTV(field_str, line_n);
                check_rc(f, containment, field_str, line_n);
                check_nm(f, containment, field_str, line_n);
                check_id(f, containment, field_str, line_n);
            }
        }
        else parser_error("Unrecognized record type: " + std::string(1, first_char) + "\n", line_n);
    }

    file.close();
}

void GFA::fillGraph(Graph& g) const {
    std::unordered_map<std::string, int> verts;

    for(auto& s:segments){
        int id = g.vertices.size();
        g.vertices.emplace_back(id);
        verts[s.getName() + "START"] = id;
        g.vertices.emplace_back(id + 1);
        verts[s.getName() + "END"] = id + 1;
        g.edges.emplace_back(id, id + 1, s.getSegmentLength());
        g.edges.back().isSegmentPart(true);
    }

    for(auto& l:links){
        int id1 = verts[l.getFromName() + "START"];
        int id2 = verts[l.getToName() + "END"];
        g.edges.emplace_back(id1, id2);
    }
}