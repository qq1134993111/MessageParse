#pragma once
#include <functional>
#include<stdint.h>

template<typename T>
struct MpCustomKey
{
    MpCustomKey() {}
    MpCustomKey(int32_t key_id) :id(key_id) {}
    bool operator==(const MpCustomKey<T>& o) const
    {
        return id == o.id;
    }

    int32_t id;
};

namespace std
{
    template <typename T>
    struct hash<MpCustomKey<T>>
    {
        std::size_t operator()(const MpCustomKey<T>& k) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            //return ((hash<string>()(k.first)
            //    ^ (hash<string>()(k.second) << 1)) >> 1)
            //    ^ (hash<int>()(k.third) << 1);
            return hash<int>()(k.id);
        }
    };
}
