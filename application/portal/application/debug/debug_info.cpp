//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "debug_info.h"

namespace portal::debug
{
float DebugInfo::get_longest_label() const
{
    float column_width = 0.0f;
    for (auto& field : fields)
    {
        const std::string& label = field->label;

        if (label.size() > column_width)
        {
            column_width = static_cast<float>(label.size());
        }
    }
    return column_width;
}
}
