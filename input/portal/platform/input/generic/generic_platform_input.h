//
// Created by thejo on 5/4/2025.
//

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "portal/input/input_types.h"

namespace portal::input
{

class GenericPlatformInput
{
public:
    /**
     * Populates the key_codes and key_names vectors with the key mapping, returns the size of the mappings.
     *
     * @param key_codes an output vector to hold the key codes
     * @param key_names an output vector to hold the key names
     * @param max_mapping The maximum number of mappings to return.
     * @return The actual size of the mappings returned.
     */
    static uint32_t get_keymap(std::vector<uint32_t>& key_codes, std::vector<std::string>& key_names, uint32_t max_mapping);
};

} // portal
