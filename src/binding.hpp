#ifndef __CARMEN_BINDING_HPP__
#define __CARMEN_BINDING_HPP__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <nan.h>
#include <exception>
#include <string>
#include <map>
#include <list>
#include <vector>
#pragma clang diagnostic pop
#include <iostream>
#include <fstream>
#include <tuple>

#define USE_REDUCED_TRIE 1
#include "cedar.h"

namespace carmen {

class noncopyable
{
protected:
    constexpr noncopyable() = default;
    ~noncopyable() = default;
    noncopyable( noncopyable const& ) = delete;
    noncopyable& operator=(noncopyable const& ) = delete;
};

class Cache: public node::ObjectWrap {
public:
    ~Cache();
    typedef std::string key_type;
    typedef uint64_t value_type;
    // list + map as simple LRU cache
    typedef cedar::da<int32_t> shard_trie;
    typedef std::tuple<std::string,std::string> shard_pair;
    typedef std::pair<std::string,shard_pair> message_pair;
    typedef std::list<message_pair> message_list;
    typedef std::map<std::string,message_list::iterator> message_cache;
    // fully cached item
    typedef std::vector<value_type> intarray;
    typedef std::vector<key_type> keyarray;
    typedef std::map<key_type,intarray> arraycache;
    typedef std::map<std::string,arraycache> memcache;
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Handle<v8::Object> target);
    static NAN_METHOD(New);
    static NAN_METHOD(_shard);
    static NAN_METHOD(has);
    static NAN_METHOD(loadSync);
    static NAN_METHOD(pack);
    static NAN_METHOD(merge);
    static NAN_METHOD(list);
    static NAN_METHOD(_get);
    static NAN_METHOD(_set);
    static NAN_METHOD(unload);
    static NAN_METHOD(coalesce);
    explicit Cache();
    memcache cache_;
    message_cache msg_;
    message_list msglist_;
    unsigned cachesize = 131072;
};

#define CACHE_MESSAGE 1
#define CACHE_ITEM 1

#define SHARD_PREFIX_LENGTH 4

}


#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstdint>

std::string string_to_hex(const std::string& in) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    for (size_t i = 0; in.length() > i; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(in[i]));
    }

    return ss.str();
}
#endif // __CARMEN_BINDING_HPP__
