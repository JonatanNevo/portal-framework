//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once


#include <format>
#include <string>

#include <portal/core/glm.h>

namespace portal::debug::fields
{
/**
 * @brief Base Field Interface class
 */
struct Base
{
    std::string label;

    explicit Base(const std::string& label) : label{label} {}
    virtual ~Base() = default;

    virtual const std::string to_string() = 0;
};

/**
 * @brief Static Field Implementation
 *
 * To be used for values that do not change often.
 */
template <typename T>
struct Static : Base
{
    T value;

    Static(const std::string& label, const T& value) : Base(label), value{value} {}

    const std::string to_string() override
    {
        return std::format("{}", value);
    }
};

/**
 * @brief Dynamic Field Implementation
 *
 * To be used for values that change frequently.
 */
template <typename T>
struct Dynamic : public Base
{
    T& value;

    Dynamic(const std::string& label, T& value) : Base(label), value{value} {}

    const std::string to_string() override
    {
        return std::format("{}", value);
    }
};

/**
 * @brief Vector Field Implementation
 *
 * To be used for values that have an X, Y and Z value.
 */
template <typename T>
struct Vector final : Static<T>
{
    T x, y, z;

    Vector(const std::string& label, const glm::vec3& vec) : Vector(label, vec.x, vec.y, vec.z) {}

    Vector(const std::string& label, T x, T y, T z) : Static<T>(label, x), x{x}, y{y}, z{z} {}

    const std::string to_string() override
    {
        return std::format("x: {} y: {} z: {}", x, y, z);
    }
};

/**
 * @brief MinMax Field Implementation
 *
 * To be used for numbers that change a lot, keeping track of the high/low values.
 */
template <typename T> requires std::is_arithmetic_v<T>
struct MinMax final : public Dynamic<T>
{
    T min, max;

    MinMax(const std::string& label, T& value) : Dynamic<T>(label, value), min{value}, max{value} {}

    const std::string to_string() override
    {
        if (Dynamic<T>::value > max)
        {
            max = Dynamic<T>::value;
        }
        if (Dynamic<T>::value < min)
        {
            min = Dynamic<T>::value;
        }
        if (min == 0)
        {
            min = Dynamic<T>::value;
        }

        return std::format("current: {} min: {} max: {} ", Dynamic<T>::value, min, max);
    }
};
}
