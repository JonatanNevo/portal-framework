// //
// // Copyright © 2025 Jonatan Nevo.
// // Distributed under the MIT license (see LICENSE file).
// //
//
// #pragma once
//
// #include <cstdint>
// #include <cstddef>
// #include <random>
//
// namespace portal
// {
// template <typename T>
// class UniqueIdBase
// {
// public:
//     using value_type = T;
//
// protected:
//     // TODO: should these be protected with locks?
//     inline static std::random_device random_device;
//     inline static std::mt19937 random_engine{random_device()};
//     inline static std::uniform_int_distribution<T> distribution;
//
// public:
//     UniqueIdBase() : uuid(distribution(random_engine)) {}
//     explicit UniqueIdBase(value_type uuid) : uuid(uuid) {}
//
//     [[nodiscard]] operator value_type() const { return uuid; }
//
//     [[nodiscard]] bool operator==(const UniqueIdBase& other) const
//     {
//         return uuid == other.uuid;
//     }
//
//     [[nodiscard]] bool operator==(const value_type& other) const
//     {
//         return uuid == other;
//     }
//
// private:
//     value_type uuid;
// };
//
// using UUID = UniqueIdBase<uint64_t>;
// } // portal
//
// template <>
// struct std::hash<portal::UUID>
// {
//     std::size_t operator()(const portal::UUID& uuid) const noexcept
//     {
//         return uuid.operator portal::UUID::value_type();
//     }
// };
//
// template <>
// struct std::formatter<portal::UUID>
// {
//     static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
//     {
//         return ctx.begin();
//     }
//
//     template <typename FormatContext>
//     auto format(const portal::UUID& id, FormatContext& ctx) const
//     {
//         return format_to(ctx.out(), "{}", static_cast<portal::UUID::value_type>(id));
//     }
// };