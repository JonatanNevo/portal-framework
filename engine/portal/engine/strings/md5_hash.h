//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <array>
#include <iostream>
#include <cstdint>

#include "portal/core/common.h"
#include "portal/input/input_types.h"

namespace portal::hash
{
namespace md5_internal
{
    /// MD5 operations
    constexpr uint32_t f(const uint32_t x, const uint32_t y, const uint32_t z)
    {
        return z ^ (x & (y ^ z));
    }

    constexpr uint32_t g(const uint32_t x, const uint32_t y, const uint32_t z)
    {
        return y ^ (z & (x ^ y));
    }

    constexpr uint32_t h(const uint32_t x, const uint32_t y, const uint32_t z)
    {
        return x ^ y ^ z;
    }

    constexpr uint32_t i(const uint32_t x, const uint32_t y, const uint32_t z)
    {
        return y ^ (x | ~z);
    }

    consteval uint32_t step_helper(const uint32_t fun_val, const uint32_t s, const uint32_t b)
    {
        return ((fun_val << s) | ((fun_val & 0xffffffff) >> (32 - s))) + b;
    }

    /**
     * Generic application of the "fun" function
     */
    template <typename Functor>
    consteval uint32_t step(
        Functor fun,
        const uint32_t a,
        const uint32_t b,
        const uint32_t c,
        const uint32_t d,
        const uint32_t x,
        const uint32_t t,
        const uint32_t s
        )
    {
        return step_helper(a + fun(b, c, d) + x + t, s, b);
    }


    /**
     * Retrieve the nth uint32_t in the buffer
     */
    consteval uint32_t data32(const char* data, const size_t n)
    {
        return (static_cast<uint32_t>(data[n * 4]) & 0xff) |
            ((static_cast<uint32_t>(data[n * 4 + 1]) << 8) & 0xff00) |
            ((static_cast<uint32_t>(data[n * 4 + 2]) << 16) & 0xff0000) |
            ((static_cast<uint32_t>(data[n * 4 + 3]) << 24) & 0xff000000);
    }

    /// Constants
    // clang-format off
    constexpr std::array<uint32_t, 64> constants = {{
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,
        0xa8304613,0xfd469501,0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
        0x6b901122,0xfd987193,0xa679438e,0x49b40821,0xf61e2562,0xc040b340,
        0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,
        0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
        0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,0x289b7ec6,0xeaa127fa,
        0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,
        0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
        0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    }};

    constexpr std::array<size_t, 64> shift = {{
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,5,9,14,20,5,9,14,20,
        5,9,14,20,5,9,14,20,4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
        6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
    }};

    constexpr std::array<size_t, 64> indexes = {{
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,6,11,0,5,10,15,4,
        9,14,3,8,13,2,7,12,5,8,11,14,1,4,7,10,13,0,3,6,9,12,15,2,
        0,7,14,5,12,3,10,1,8,15,6,13,4,11,2,9
    }};
    // clang-format on


    constexpr std::array<decltype(f)*, 4> functions = {{
        f,
        g,
        h,
        i
    }};

    /// Initial Buffer Generators


    template <size_t... indexes>
    struct index_tuple {};

    template <size_t head, size_t... indexes>
    struct index_tuple<head, indexes...>
    {
        typedef typename index_tuple<head - 1, head - 1, indexes...>::type type;
    };

    template <size_t... indexes>
    struct index_tuple<0, indexes...>
    {
        typedef index_tuple<indexes...> type;
    };

    template <typename... Args>
    struct index_tuple_maker
    {
        typedef typename index_tuple<sizeof...(Args)>::type type;
    };

    /**
     * Builds the buffer
     *
     * For indexes < string length: output the ith character in the string.
     * For indexes > string length: output 0.
     * If index == string length: output 0x80
     * If index == 56: output string length << 3
     */
    template <size_t n, size_t i>
    struct buffer_builder
    {
        static consteval char make_value(const char* data)
        {
            return (i <= n) ? data[i] : 0;
        }
    };

    template <size_t n>
    struct buffer_builder<n, n>
    {
        static consteval char make_value(const char*)
        {
            return 0x80;
        }
    };

    template <size_t n>
    struct buffer_builder<n, 56>
    {
        static consteval char make_value(const char*)
        {
            return n << 3;
        }
    };

    using buffer_type = std::array<char, 64>;

    template <size_t n, size_t... indexes>
    consteval buffer_type make_buffer_helper(const char (&data)[n], index_tuple<indexes...>)
    {
        return buffer_type{{buffer_builder<n - 1, indexes>::make_value(data)...}};
    }

    template <size_t n>
    consteval buffer_type make_buffer(const char (&data)[n])
    {
        return make_buffer_helper(data, index_tuple<64>::type());
    }
}

using md5_type = uint128_t;

/**
 * There are 64 steps. The ith step has the same structure as the ith + 4 step.
 * That means that we can repeat the same structure, and pick the appropriate
 * constants and function to apply, depending on the step number.
 */
template <size_t n, size_t rot>
struct md5_step;

consteval md5_type make_md5_result(const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d)
{
    const uint64_t lo = static_cast<uint64_t>(a) | (static_cast<uint64_t>(b) << 32);
    const uint64_t hi = static_cast<uint64_t>(c) | (static_cast<uint64_t>(d) << 32);
    return {lo, hi};
}

template <>
struct md5_step<64, 0>
{
    static consteval md5_type do_step(const char*, const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d)
    {
        return make_md5_result(a + 0x67452301, b + 0xefcdab89, c + 0x98badcfe, d + 0x10325476);
    }
};

template <size_t n>
struct md5_step<n, 3>
{
    static consteval md5_type do_step(const char* data, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        return md5_step<n + 1, (n + 1) % 4>::do_step(
            data,
            a,
            md5_internal::step(
                md5_internal::functions[n / 16],
                b,
                c,
                d,
                a,
                md5_internal::data32(data, md5_internal::indexes[n]),
                md5_internal::constants[n],
                md5_internal::shift[n]
                ),
            c,
            d
            );
    }
};

template <size_t n>
struct md5_step<n, 2>
{
    static consteval md5_type do_step(const char* data, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        return md5_step<n + 1, (n + 1) % 4>::do_step(
            data,
            a,
            b,
            md5_internal::step(
                md5_internal::functions[n / 16],
                c,
                d,
                a,
                b,
                md5_internal::data32(data, md5_internal::indexes[n]),
                md5_internal::constants[n],
                md5_internal::shift[n]
                ),
            d
            );
    }
};

template <size_t n>
struct md5_step<n, 1>
{
    static consteval md5_type do_step(const char* data, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        return md5_step<n + 1, (n + 1) % 4>::do_step(
            data,
            a,
            b,
            c,
            md5_internal::step(
                md5_internal::functions[n / 16],
                d,
                a,
                b,
                c,
                md5_internal::data32(data, md5_internal::indexes[n]),
                md5_internal::constants[n],
                md5_internal::shift[n]
                )
            );
    }
};

template <size_t n>
struct md5_step<n, 0>
{
    static consteval md5_type do_step(const char* data, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        return md5_step<n + 1, (n + 1) % 4>::do_step(
            data,
            md5_internal::step(
                md5_internal::functions[n / 16],
                a,
                b,
                c,
                d,
                md5_internal::data32(data, md5_internal::indexes[n]),
                md5_internal::constants[n],
                md5_internal::shift[n]
                ),
            b,
            c,
            d
            );
    }
};

template <size_t n>
consteval md5_type md5(const char (&data)[n])
{
    return md5_step<0, 0>::do_step(md5_internal::make_buffer(data).data(), 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476);
}

namespace literals
{
    template <std::size_t N>
    struct MD5String
    {
        md5_type hashed{};

        consteval MD5String(const char(&str)[N])
        {
            hashed = md5<N>(str);
        }
    };

    template<MD5String T>
    consteval auto operator ""_md5()
    {
        return T.hashed;
    }
}

}

