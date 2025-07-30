#pragma once

#include <functional>
#include <utility>
#include <type_traits>

namespace stdpp {

    template<typename T, typename Compare = std::less<>>
    class sorted_pair{
        static_assert(std::is_invocable_r_v<bool, Compare, const T&, const T&>, "Compare(a, b) must be valid and convertible to bool");
    private:
        std::pair<T, T> p;
    public:
        sorted_pair(T first, T second, Compare comp = {})
        : p(comp(first, second) ? std::make_pair(std::move(first), std::move(second)) : std::make_pair(std::move(second), std::move(first))) {}

        T& first() noexcept { return p.first; }
        const T& first() const noexcept { return p.first; }
        T& second() noexcept { return p.second; }
        const T& second() const noexcept { return p.second; }

        friend bool operator==(const sorted_pair& a, const sorted_pair& b) noexcept { return a.p == b.p; }
        friend bool operator!=(const sorted_pair& x, const sorted_pair& y) noexcept { return !(x == y); }
        friend bool operator<(const sorted_pair& a, const sorted_pair& b) noexcept { return a.p <  b.p; }
    };

}

namespace std {
    template<class T, class Compare>
    struct hash<stdpp::sorted_pair<T, Compare>> {
        size_t operator()(const stdpp::sorted_pair<T, Compare>& sp) const noexcept {
            static_assert(std::is_invocable_r_v<size_t, std::hash<T>, const T&>,"T must be hashable with std::hash<T>");

            const size_t h1 = std::hash<T>{}(sp.first());
            const size_t h2 = std::hash<T>{}(sp.second());

            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };
}