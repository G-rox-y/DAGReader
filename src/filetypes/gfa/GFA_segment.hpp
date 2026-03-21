#pragma once

#include "pch.hpp"

#include "gfa/GFA_virtuals.hpp"

class GFA_segment : public segmentLength, public readCount, public fragmentCount, public kmerCount, public hash256{
private:
    std::string name;

    bool seq_exists = false; // is sequence defined in the file (its definition can be skipped using '*')
    std::filesystem::path seq_file; // sequence file
    bool seq_file_is_gfa; // sequence is stored in the orignal gfa
    std::streampos seq_loc_gfa=0; // stream position inside the gfa where the sequence can be found

public:
    GFA_segment() = default;
    GFA_segment(const std::string& n) : name(n) {}

    void noSequence() { seq_exists = seq_file_is_gfa = false;}
    void setSequence(const std::string& path, std::streampos loc){ seq_file = path; seq_loc_gfa = loc; seq_exists = seq_file_is_gfa = true; }

    // this function will throw an std::runtime_error if the path provided doesnt work
    void setUriSequence(const std::string& uri_path_str, const std::string& gfa_path_str);

    bool isSequenceAvailable() const { return seq_exists; }

    const std::string& getName() const { return name; }

    std::optional<std::tuple<std::filesystem::path, std::streampos>> provideSequence() const {
        if (!seq_exists) return std::nullopt;
        return std::make_tuple(seq_file, seq_loc_gfa);
    }
};
