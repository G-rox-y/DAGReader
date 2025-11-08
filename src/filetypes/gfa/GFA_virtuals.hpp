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
};

// abstract class for adding the fragment_count property to classes
class fragmentCount{
private:
    long long int fragment_count = -1;
public:
    virtual ~fragmentCount() = default;
    void setFragmentCount(const long long int& c) { fragment_count = c; }
};

// abstract class for adding the kmer_count property to classes
class kmerCount{
private:
    long long int kmer_count = -1;
public:
    virtual ~kmerCount() = default;
    void setKmerCount(const long long int& c) { kmer_count = c; }
};

// abstract class for adding the hash property to classes
class hash{
private:
    std::string hash_sha256; // sha256 checksum of the sequence (if provided)
public:
    virtual ~hash() = default;
    void setHash(const std::string& h) { hash_sha256 = h; }
};

// abstract class for adding the mapping_quality property to classes
class mappingQuality{
private:
    long long int mapping_quality = -1;
public:
    virtual ~mappingQuality() = default;
    void setMappingQuality(const long long int& q) { mapping_quality = q; }
};

// abstract class for adding the mismatch_gaps_number property to classes
class numOfMismatchsGaps{
private:
    long long int mismatch_gaps_number = -1;
public:
    virtual ~numOfMismatchsGaps() = default;
    void setNumOfMismatchsGaps(const long long int& n) { mismatch_gaps_number = n; }
};

// abstract class for adding the edge_identifier property to classes
class edgeIdentifier{
private:
    std::string edge_identifier;
public:
    virtual ~edgeIdentifier() = default;
    void setEdgeIdentifier(const std::string& s) { edge_identifier = s; }
};

// abstract class for adding the overlap property to classes
class overlap{
private:
    std::string overlap_cigar; // Optional CIGAR string describing overlap
public:
    virtual ~overlap() = default;
    void setOverlap(const std::string& ov) { overlap_cigar = (ov == "*") ? "" : ov; } // if ov is '*' that means no CIGAR string is provided
    const std::string& getOverlap() const { return overlap_cigar; }
};