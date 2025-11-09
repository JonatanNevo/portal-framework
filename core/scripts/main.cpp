//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <iostream>

#include "../portal/core/strings/rapidhash/rapidhash.h"

int main(const int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "No string argument provided\n";
        return 1;
    }

    const char* first_string = argv[1];
    const size_t length = std::strlen(first_string);
    const uint64_t hash = rapidhash(first_string, length);

    std::cout << hash << "\n";

    return 0;
}