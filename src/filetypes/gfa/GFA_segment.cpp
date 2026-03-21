#include "GFA_segment.hpp"

void GFA_segment::setUriSequence(const std::string& uri_path_str, const std::string& gfa_path_str)
{
    // TODO: implement recognizing if this is an uri or a path, currently we dont consider an uri option

    std::filesystem::path gfa_path = gfa_path_str, local_path = uri_path_str;
    if (local_path.has_root_path())
        seq_file = (gfa_path.parent_path() / uri_path_str).lexically_normal(); // parent path since the gfa path will have a .gfa file at the end
    else
        seq_file = local_path;

    if (!std::filesystem::exists(seq_file)){
        throw std::runtime_error("The provided path is invalid\n\tpath: " + seq_file.string());
    }
    
    seq_exists = true;
}
