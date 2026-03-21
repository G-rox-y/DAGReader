#pragma once

// abstract class for adding the segment_length property to classes
class segmentLength{
private:
    long long int segment_length = -1;
public:
    virtual ~segmentLength() = default;
    void setSegmentLength(const long long int& c) { segment_length = c; }
    long long int getSegmentLength() const { return segment_length; }
};

// abstract class for adding the read_count property to classes
class readCount{
private:
    long long int read_count = -1;
public:
    virtual ~readCount() = default;
    void setReadCount(const long long int& c) { read_count = c; }
    long long int getReadCount() const { return read_count; }
};

// abstract class for adding the fragment_count property to classes
class fragmentCount{
private:
    long long int fragment_count = -1;
public:
    virtual ~fragmentCount() = default;
    void setFragmentCount(const long long int& c) { fragment_count = c; }
    long long int getFragmentCount() const { return fragment_count; }
};

// abstract class for adding the kmer_count property to classes
class kmerCount{
private:
    long long int kmer_count = -1;
public:
    virtual ~kmerCount() = default;
    void setKmerCount(const long long int& c) { kmer_count = c; }
    long long int getKmerCount() const { return kmer_count; }
};

// abstract class for adding the hash property to classes
class hash256{
private:
    std::string hash_sha256; // sha256 checksum of the sequence (if provided)
public:
    virtual ~hash256() = default;
    void setHash(const std::string& h) { hash_sha256 = h; }
};

// abstract class for adding the mapping_quality property to classes
class mappingQuality{
private:
    long long int mapping_quality = -1;
public:
    virtual ~mappingQuality() = default;
    void setMappingQuality(const long long int& q) { mapping_quality = q; }
    long long int getMappingQuality() const { return mapping_quality; }
};

// abstract class for adding the mismatch_gaps_number property to classes
class numOfMismatchsGaps{
private:
    long long int mismatch_gaps_number = -1;
public:
    virtual ~numOfMismatchsGaps() = default;
    void setNumOfMismatchsGaps(const long long int& n) { mismatch_gaps_number = n; }
    long long int getNumOfMismatchGaps() const { return mismatch_gaps_number; }
};

// abstract class for adding the edge_identifier property to classes
class edgeIdentifier{
private:
    std::string edge_identifier;
public:
    virtual ~edgeIdentifier() = default;
    void setEdgeIdentifier(const std::string& s) { edge_identifier = s; }
    std::string getEdgeIdentifier() const { return edge_identifier; }
};

// abstract class for adding the overlap property to classes
class overlap {
private:
    bool overlap_exists = false; // is the string in the file (its definition can be skipped using '*')
    std::filesystem::path overlap_file; // file where the overlap can be found
    std::streampos overlap_loc_gfa=0; // stream position inside the gfa where cigar can be found
public:
    virtual ~overlap() = default;
    void noOverlap() { overlap_exists = false; }
    void setOverlap(std::filesystem::path path, std::streampos pos) { overlap_file = path; overlap_loc_gfa = pos; overlap_exists = true; }
    bool isOverlapAvailable() const { return overlap_exists; }
    std::optional<std::tuple<std::filesystem::path, std::streampos>> provideOverlap() const {
        if (!overlap_exists) return std::nullopt;
        return std::make_tuple(overlap_file, overlap_loc_gfa);
    }
};
