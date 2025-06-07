//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>
#include <random>

#include "portal/core/glm.h"

namespace portal
{

class Random
{
public:
    static void init();
    static void init(uint32_t seed);
    static uint32_t get_uint();
    static uint32_t get_uint(uint32_t min, uint32_t max);
    static float get_float();
    static float get_float(float min, float max);
    static glm::vec3 get_vec3();
    static glm::vec3 get_vec3(float min, float max);

private:
    static std::mt19937 engine;
    static std::uniform_int_distribution<std::mt19937::result_type> int_distribution;
    static std::uniform_real_distribution<float> float_distribution;

};

} // portal
