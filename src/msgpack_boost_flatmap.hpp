/*
  MessagePack for C++ static resolution routine

  Copyright (C) 2014-2015 KONDO Takatoshi, 2017 Kaan Genç

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt)


  The code in this file is the MessagePack for C++'s unordered_map
  static resolution routine with some trivial modifications that adapt
  it to work with Boost's flat_map.
 */
#pragma once
#include <boost/container/flat_map.hpp>
#include <msgpack.hpp>


namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {


template <typename K, typename V>
struct as<
    boost::container::flat_map<K, V>,
    typename std::enable_if<msgpack::has_as<K>::value || msgpack::has_as<V>::value>::type> {
    boost::container::flat_map<K, V> operator()(msgpack::object const & o) const {
        if (o.type != msgpack::type::MAP) { throw msgpack::type_error(); }
        msgpack::object_kv* p(o.via.map.ptr);
        msgpack::object_kv* const pend(o.via.map.ptr + o.via.map.size);
        boost::container::flat_map<K, V> v;
        for (; p != pend; ++p) {
            v.emplace(p->key.as<K>(), p->val.as<V>());
        }
        return v;
    }
};

template <typename K, typename V>
struct convert<boost::container::flat_map<K, V>> {
    msgpack::object const& operator()(msgpack::object const& o, boost::container::flat_map<K, V> & v) const {
        if(o.type != msgpack::type::MAP) { throw msgpack::type_error(); }
        msgpack::object_kv* p(o.via.map.ptr);
        msgpack::object_kv* const pend(o.via.map.ptr + o.via.map.size);
        boost::container::flat_map<K, V> tmp;
        for(; p != pend; ++p) {
            K key;
            p->key.convert(key);
            p->val.convert(tmp[std::move(key)]);
        }
        v = std::move(tmp);
        return o;
    }
};

template <typename K, typename V>
struct pack<boost::container::flat_map<K, V>> {
    template <typename Stream>
        msgpack::packer<Stream>& operator()(msgpack::packer<Stream> & o, const boost::container::flat_map<K, V> & v) const {
        uint32_t size = checked_get_container_size(v.size());
        o.pack_map(size);
        for(typename boost::container::flat_map<K, V>::const_iterator it(v.begin()), it_end(v.end());
            it != it_end; ++it) {
            o.pack(it->first);
            o.pack(it->second);
        }
        return o;
    }
};

template <typename K, typename V>
struct object_with_zone<boost::container::flat_map<K, V>> {
    void operator()(msgpack::object::with_zone& o, const boost::container::flat_map<K, V> & v) const {
        o.type = msgpack::type::MAP;
        if(v.empty()) {
            o.via.map.ptr  = MSGPACK_NULLPTR;
            o.via.map.size = 0;
        } else {
            uint32_t size = checked_get_container_size(v.size());
            msgpack::object_kv* p = static_cast<msgpack::object_kv*>(o.zone.allocate_align(sizeof(msgpack::object_kv)*size));
            msgpack::object_kv* const pend = p + size;
            o.via.map.ptr  = p;
            o.via.map.size = size;
            typename boost::container::flat_map<K, V>::const_iterator it(v.begin());
            do {
                p->key = msgpack::object(it->first, o.zone);
                p->val = msgpack::object(it->second, o.zone);
                ++p;
                ++it;
            } while(p < pend);
        }
    }
};


}
}
}
