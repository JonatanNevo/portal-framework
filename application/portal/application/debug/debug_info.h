//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once

#include <vector>
#include "fields.h"

namespace portal::debug
{
/**
 * @brief Manages the debug information
 */
class DebugInfo
{
public:
    const std::vector<std::unique_ptr<fields::Base>>& get_fields() const { return fields; }

    /**
     * @brief   Calculates the field label with the most amount of characters
     * @returns The length of the longest label
     */
    float get_longest_label() const;

    /**
     * @brief Constructs and inserts a new field of type C<T>
     *
     * Replaces the field if it is of type static.
     */
    template <template <typename> class C, typename T, typename... A> requires std::is_base_of_v<fields::Base, C<T>>
    void insert(const std::string& label, A&&... args)
    {
        for (auto& field : fields)
        {
            if (field->label == label)
            {
                if (dynamic_cast<fields::Static<T>*>(field.get()))
                {
                    field = std::make_unique<C<T>>(label, args...);
                }
                return;
            }
        }

        auto field = std::make_unique<C<T>>(label, std::forward<A>(args)...);
        fields.push_back(std::move(field));
    }

private:
    std::vector<std::unique_ptr<fields::Base>> fields;
};
}
