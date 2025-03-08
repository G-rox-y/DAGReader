#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <exception>

#include "GFA_virtuals.hpp"

class GFA_segment : public segmentLength, public readCount, public fragmentCount, public kmerCount, public hash{
private:
    std::string name;

    bool seq_exists; // is sequence defined in the file (its definition can be skipped using '*')
    std::filesystem::path seq_file; // sequence file
    bool seq_file_is_gfa; // sequence is stored in the orignal gfa
    std::streampos seq_loc_gfa; // stream position inside the gfa where the sequence can be found

public:
    GFA_segment(const std::string& n);

    void setSequence(std::ifstream& input, const std::string& path);

    // this function will throw an std::runtime_error if the path provided doesnt work
    void setSequence(const std::string& uri_path_str, const std::string& gfa_path_str);
};
