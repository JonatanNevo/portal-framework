//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesmer_twisted_random.h"

namespace portal
{

void MesmerTwistedRandom::init()
{
    init(std::random_device()());
}


void MesmerTwistedRandom::init(const uint32_t seed)
{
    engine.seed(seed);
}

uint32_t MesmerTwistedRandom::get_uint()
{
    return int_distribution(engine);
}

uint32_t MesmerTwistedRandom::get_uint(const uint32_t min, const uint32_t max)
{
    return min + get_uint() % (max - min);
}

float MesmerTwistedRandom::get_float()
{
    return float_distribution(engine);
}

float MesmerTwistedRandom::get_float(const float min, const float max)
{
    return min + get_float() * (max - min);
}

} // namespace portal
