//
// Created by Jonatan Nevo on 21/03/2025.
//

#include "uuid.h"

#include <random>

namespace portal
{

static std::random_device s_random_device;
static std::mt19937_64 s_engine(s_random_device());
static std::uniform_int_distribution<uint64_t> s_distribution;

static std::mt19937_64 s_engine_32(s_random_device());
static std::uniform_int_distribution<uint32_t> s_distribution_32;

UUID::UUID(): uuid(s_distribution(s_engine)) {}

UUID::UUID(const uint64_t uuid): uuid(uuid) {}

UUID::UUID(const UUID& other) = default;

UUID32::UUID32(): uuid(s_distribution_32(s_engine_32)) {}

UUID32::UUID32(const uint32_t uuid): uuid(uuid) {}

UUID32::UUID32(const UUID32& other) = default;


} // portal