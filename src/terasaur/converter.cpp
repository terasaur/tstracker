
#include "terasaur/converter.hpp"

using std::cout;
using std::endl;

namespace terasaur {

void ot_hash_to_sha1_hash(ot_hash* hash, sha1_hash& info_hash) {
    std::string tmp(libtorrent::big_number::size, 0);
    for (size_t i = 0; i < libtorrent::big_number::size; ++i) {
        tmp[i] = static_cast<char>((*hash)[i]);
    }
    info_hash = sha1_hash(tmp);
}

void ot_hash_to_sha1_hash(ot_hash hash, sha1_hash& info_hash) {
    std::string tmp(libtorrent::big_number::size, 0);
    for (size_t i = 0; i < libtorrent::big_number::size; ++i) {
        tmp[i] = static_cast<char>(hash[i]);
    }
    info_hash = sha1_hash(tmp);
}

} // namespace terasaur
