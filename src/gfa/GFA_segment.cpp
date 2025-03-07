#include "GFA_segment.hpp"

GFA_segment::GFA_segment(const std::string& n) : name(n), seq_exists(false), seq_file_is_gfa(false) {}

void GFA_segment::setSequence(std::ifstream& input, const std::string& path){
    // the program doesnt need to store an entire sequence in ram
    // it can just store a reference to where it can find it if needed and get the data at the moment it gets requested
    if (input.peek() != '*'){
        seq_file = path;
        seq_loc_gfa = input.tellg();
        seq_exists = seq_file_is_gfa = true;
    }

    // advance the stream over the sequence fragment
    for (char c = '0'; c != '\t' ; input.get(c)) continue;
}

void GFA_segment::setSequence(const std::string& uri_path_str, const std::string& gfa_path_str)
{
    // TODO: implement recognizing if this is an uri or a path, currently we dont consider an uri option

    std::filesystem::path gfa_path = gfa_path_str, local_path = uri_path_str;
    if (local_path.has_root_path())
        seq_file = (gfa_path.parent_path() / uri_path_str).lexically_normal(); // parent path since the gfa path will have a .gfa file at the end
    else
        seq_file = local_path;

    if (!std::filesystem::exists(seq_file)){
        // TODO: throw an error here to be caught by the parser
    }
    
    seq_exists = true;
}