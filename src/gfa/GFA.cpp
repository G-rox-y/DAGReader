#include "GFA.hpp"


void GFA::parser_error(const std::string& description, const int line_n) const {
    // TODO: add this
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

    // the parser works in 3 layers
    // layer 1 is a while loop that gets the entire gfa file and separates it by newlines (gives lines)
    // layer 2 is a while loop that gets an individual line and separates it by whitespaces (gives fields)
    // layer 3 is a for loop that gets an individual field and separates it by semicolons (gives TAG:TYPE:VALUE)

    // a lambda for extracting layer 3 data
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
    auto cnc_i = [this](const std::string& in, const std::string& line_str, const int& line_n) -> long long int {
        // convert value to ulli
        unsigned long long int val;
        try { 
            val = std::stoull(in);
        }
        catch(const std::exception& e){
            parser_error("Impossible to convert VALUE to integer\n\tline: " + line_str, line_n);    
        }
        return val;
    };

    // check (c) hash (H) value
    auto c_H = [this](const std::string& in, const std::string& line_str, const int& line_n){
        if (in.empty()) parser_error("H VALUE (hash) can't be empty\n\tline: " + line_str, line_n);
        // TODO: implement this
    };

    // check (c) printable string (Z) value
    auto c_Z = [this](const std::string& in, const std::string& line_str, const int& line_n){
        if (in.empty()) parser_error("Z VALUE (printable string) can't be empty\n\tline: " + line_str, line_n);
        // TODO: implement this
    };


    // check version number field
    auto check_vn = [this, &c_Z](const GFA_field& f, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "VN"){
            if (f.type == "Z"){
                c_Z(f.value, line_str, line_n);
                if (version_string.empty()) version_string = f.value;
                else parser_error("Redefinition of a version string (already previously defined)\n\tline: " + line_str, line_n);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'Z'\n\tfull field:" + field_str, line_n);
        }
    };

    // check segment length field
    auto check_ln = [this, &cnc_i](const GFA_field& f, segmentLength& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "LN"){
            if (f.type == "i") element.setSegmentLength(cnc_i(f.value, line_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check read count field
    auto check_rc = [this, &cnc_i](const GFA_field& f, readCount& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "RC"){
            if (f.type == "i") element.setReadCount(cnc_i(f.value, line_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check fragment count field
    auto check_fc = [this, &cnc_i](const GFA_field& f, fragmentCount& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "FC"){
            if (f.type == "i") element.setFragmentCount(cnc_i(f.value, line_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check k-mer count
    auto check_kc = [this, &cnc_i](const GFA_field& f, kmerCount& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "KC"){
            if (f.type == "i") element.setKmerCount(cnc_i(f.value, line_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check sha256 checksum of the sequence
    auto check_sh = [this, &c_H](const GFA_field& f, hash& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "SH"){
            if (f.type == "H"){
                c_H(f.value, line_str, line_n);
                element.setHash(f.value);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'H'\n\tfull field:" + field_str, line_n);
        }
    };
    
    // check the uri to the sequence
    auto check_ur = [this, &c_Z](const GFA_field& f, GFA_segment& segment, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "UR"){
            if (f.type == "Z"){
                c_Z(f.value, line_str, line_n);
                //segment.loadSequence(f.value);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'Z'\n\tfull field:" + field_str, line_n);
        }
    };

    // check mapping quality
    auto check_mq = [this, &cnc_i](const GFA_field& f, mappingQuality& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "MQ"){
            if (f.type == "i") element.setMappingQuality(cnc_i(f.value, line_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check number of mismatches/gaps
    auto check_nm = [this, &cnc_i](const GFA_field& f, numOfMismatchsGaps& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "NM"){
            if (f.type == "i") element.setNumOfMismatchsGaps(cnc_i(f.value, line_str, line_n));
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'i'\n\tfull field:" + field_str, line_n);
        }
    };

    // check the edge identifier to the sequence
    auto check_id = [this, &c_Z](const GFA_field& f, edgeIdentifier& element, const std::string& line_str, const std::string& field_str, const int line_n){
        if (f.tag == "ID"){
            if (f.type == "Z"){
                c_Z(f.value, line_str, line_n);
                element.setEdgeIdentifier(f.value);
            }
            else parser_error("Unrecognized TYPE for '" + f.tag + "', expected: 'Z'\n\tfull field:" + field_str, line_n);
        }
    };

    // now we can finally implement the parser
    std::string ln;
    int line_n = 1;
    while (std::getline(file, ln)){ // layer 1 // TODO: this will have to be changed because line sizes could get horrendously large
        std::stringstream ss(ln);
        std::string record_type; // record type

        std::getline(ss, record_type, '\t'); // TODO: consider accepting 2-4 spaces as a tab
        std::string field_str;

        if (record_type == "#") continue;
        else if (record_type == "H")
        {
            while(std::getline(ss, field_str, '\t')){ // layer 2
                GFA_field f = getTTV(field_str, line_n); // layer 3
                check_vn(f, ln, field_str, line_n);
            }
        }
        else if (record_type == "S")
        {
            // each segment has a name and a sequence, so we extract that first
            std::string name, sequence;
            std::getline(ss, name, '\t'); // TODO: check against regex if this is ok
            if (ss.fail())
                parser_error("Name for sequence not provided", line_n);
            std::getline(ss, sequence, '\t'); // TODO: check against regex if this is ok
            if (ss.fail())
                parser_error("Nucleotide sequence not provided (if the sequence is empty a '*' symbol has to be provided)", line_n);

            // TODO: since sequences can be painfully long, make it so that they can be directly loaded into the constructor to save memory
            // make gfa_segment class provide an array reference an then have this piece of code load stuff into it directly
            GFA_segment segment(name);
            // segment.loadSequence(?);

            // now check optional fields
            while(std::getline(ss, field_str, '\t')){
                GFA_field f = getTTV(field_str, line_n);
                check_ln(f, segment, ln, field_str, line_n);  // TODO: dont print entire line strings, this can be tricky because of the lenghts involved
                check_rc(f, segment, ln, field_str, line_n);
                check_fc(f, segment, ln, field_str, line_n);
                check_kc(f, segment, ln, field_str, line_n);
                check_sh(f, segment, ln, field_str, line_n);
                check_ur(f, segment, ln, field_str, line_n);
            }
        }
        else if (record_type == "L")
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

            std::getline(ss, overlap, '\t'); // TODO: check against regex if this is ok

            GFA_link link;

            while(std::getline(ss, field_str, '\t')){
                GFA_field f = getTTV(field_str, line_n);
                check_mq(f, link, ln, field_str, line_n);
                check_nm(f, link, ln, field_str, line_n);
                check_rc(f, link, ln, field_str, line_n);  // TODO: dont print entire line strings, this can be tricky because of the lenghts involved
                check_fc(f, link, ln, field_str, line_n);
                check_kc(f, link, ln, field_str, line_n);
                check_id(f, link, ln, field_str, line_n);
            }
        }
        else if (record_type == "C")
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

            std::getline(ss, overlap, '\t'); // TODO: check against regex if this is ok

            pos = cnc_i(pos_str, ln, line_n);

            GFA_containment containment;

            while(std::getline(ss, field_str, '\t')){
                GFA_field f = getTTV(field_str, line_n);
                check_rc(f, containment, ln, field_str, line_n);  // TODO: dont print entire line strings, this can be tricky because of the lenghts involved
                check_nm(f, containment, ln, field_str, line_n);
                check_id(f, containment, ln, field_str, line_n);
            }
        }
        else if (record_type == "P")
        {
            std::string name, segments, overlaps; // consider using the pipe for this, not a saving it all in ram (like you avoid saving sequences)
            std::getline(ss, name, '\t'); // TODO: check against regex if this is ok
            std::getline(ss, segments, '\t'); // TODO: check against regex if this is ok
            std::getline(ss, overlaps, '\t'); // TODO: check against regex if this is ok

            GFA_path path;
        }
        else parser_error("Unrecognized record type: " + record_type + "\n", line_n);

        line_n++;
    }
}