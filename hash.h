#pragma once

#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <string>
#include <vector>

struct IHash {
    virtual void hash(const std::string& data, std::vector<uint8_t>& h) = 0;
    virtual std::size_t size() = 0;
};

struct Hash_crc32 : IHash {
    void hash(const std::string& data, std::vector<uint8_t>& h) override
    {
        boost::crc_32_type crc;
        crc.process_bytes(data.data(), data.size());
        int sum = crc.checksum();

        h[0] = (sum >> 24) & 0xff;
        h[1] = (sum >> 16) & 0xff;
        h[2] = (sum >> 8) & 0xff;
        h[3] = (sum >> 0) & 0xff;    
    }
    std::size_t size() {
        return 4;
    }
};

struct Hash_md5 : IHash {
    using md5 = boost::uuids::detail::md5;
    void hash(const std::string& data, std::vector<uint8_t>& h) override
    {
        md5 hash;
        md5::digest_type sum;
        hash.process_bytes(data.data(), data.size());
        hash.get_digest(sum); 
        for (int i = 0; i < 4; ++i) {
            h[4*i + 0] = (sum[i] >> 24) & 0xff;
            h[4*i + 1] = (sum[i] >> 16) & 0xff;
            h[4*i + 2] = (sum[i] >> 8) & 0xff;
            h[4*i + 3] = (sum[i] >> 0) & 0xff;    
        }
    }
    std::size_t size() {
        return sizeof(md5::digest_type);
    }
};
