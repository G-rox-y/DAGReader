#pragma once

// abstract class for adding the segment_length property to classes
class segmentLength{
private:
    long long int segment_length;
public:
    virtual ~segmentLength() = default;
    void setSegmentLength(const long long int& c) { segment_length = c; }
};

// abstract class for adding the read_count property to classes
class readCount{
private:
    long long int read_count;
public:
    virtual ~readCount() = default;
    void setReadCount(const long long int& c) { read_count = c; }
};

// abstract class for adding the fragment_count property to classes
class fragmentCount{
private:
    long long int fragment_count;
public:
    virtual ~fragmentCount() = default;
    void setFragmentCount(const long long int& c) { fragment_count = c; }
};

// abstract class for adding the kmer_count property to classes
class kmerCount{
private:
    long long int kmer_count;
public:
    virtual ~kmerCount() = default;
    void setKmerCount(const long long int& c) { kmer_count = c; }
};

// abstract class for adding the hash property to classes
class hash{
private:
    std::string hash; // sha256 checksum of the sequence (if provided)
public:
    virtual ~hash() = default;
    void setHash(const std::string& h) { hash = h; }
};

// abstract class for adding the mapping_quality property to classes
class mappingQuality{
private:
    long long int mapping_quality;
public:
    virtual ~mappingQuality() = default;
    void setMappingQuality(const long long int& q) { mapping_quality = q; }
};

// abstract class for adding the mismatch_gaps_number property to classes
class numOfMismatchsGaps{
private:
    long long int mismatch_gaps_number;
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