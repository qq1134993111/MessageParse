#pragma once
#include <functional>
#include <unordered_map>
namespace mp
{
    template <typename KEY_TYPE, typename BASE_CLASS_TYPE, typename... U>
    struct factory
    {
        template<typename T>
        struct register_t
        {
            register_t(const KEY_TYPE& key)
            {
                factory<KEY_TYPE, BASE_CLASS_TYPE, U...>::get().map_.emplace(key, [](U&&... u) { return new T(std::forward<U>(u)...); });
            }

            register_t(const KEY_TYPE& key, std::function<BASE_CLASS_TYPE* (U&&...)>&& f)
            {
                factory<KEY_TYPE, BASE_CLASS_TYPE, U...>::get().map_.emplace(key, std::move(f));
            }
        };

        inline BASE_CLASS_TYPE* create(const KEY_TYPE& key, U&&... u)
        {
            auto it = map_.find(key);
            if (it != map_.end())
            {
                return  it->second(std::forward<U>(u)...);
            }
            return nullptr;
        }

        inline static factory<KEY_TYPE, BASE_CLASS_TYPE, U...>& get()
        {
            static factory<KEY_TYPE, BASE_CLASS_TYPE, U...> instance;
            return instance;
        }

    private:
        factory<KEY_TYPE, BASE_CLASS_TYPE, U...>() {};
        factory<KEY_TYPE, BASE_CLASS_TYPE, U...>(const factory<KEY_TYPE, BASE_CLASS_TYPE, U...>&) = delete;
        factory<KEY_TYPE, BASE_CLASS_TYPE, U...>(factory<KEY_TYPE, BASE_CLASS_TYPE, U...>&&) = delete;
        std::unordered_map<KEY_TYPE, std::function<BASE_CLASS_TYPE* (U&&...)>> map_;
    };
}
