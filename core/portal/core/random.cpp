//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "random.h"

namespace portal
{

    std::mt19937 Random::engine;
    std::uniform_int_distribution<std::mt19937::result_type> Random::int_distribution;
    std::uniform_real_distribution<float> Random::float_distribution;

    void Random::init() { init(std::random_device()()); }
    void Random::init(const uint32_t seed) { engine.seed(seed); }

    uint32_t Random::get_uint() { return int_distribution(engine); }
    uint32_t Random::get_uint(const uint32_t min, const uint32_t max) { return min + int_distribution(engine) % (max - min); }

    float Random::get_float() { return float_distribution(engine); }
    auto Random::get_float(const float min, const float max) -> float { return min + float_distribution(engine) * (max - min); }

    glm::vec3 Random::get_vec3() { return {get_float(), get_float(), get_float()}; }
    glm::vec3 Random::get_vec3(const float min, const float max) { return {get_float(min, max), get_float(min, max), get_float(min, max)}; }

} // namespace portal
