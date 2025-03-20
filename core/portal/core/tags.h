//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include <vector>

namespace portal
{

/**
 * @brief Used to represent a tag
 */
typedef void (*TagID)();

/**
 * @brief Tag acts as a unique identifier to categories objects
 *
 * Tags are uniquely defined using different type names. The easiest way of creating a new tag is to use an empty struct
 * struct TagName{};
 * struct DifferentTag{};
 * Tag<TagName>::ID == Tag<TagName>::member != Tag<DifferentTag>:ID
 *
 * @tparam TAGS A set of tags
 */
template <typename... TAGS>
class Tag
{
public:
    Tag()
    {
        tags = {Tag<TAGS>::ID...};
    }

    static void member() {};

    /**
     * @brief Unique TagID for a given Tag<TagName>
     */
    constexpr static TagID ID = &member;

    static bool has_tag(const TagID id)
    {
        return std::ranges::find(tags, id) != tags.end();
    }

    template <typename C>
    static bool has_tag()
    {
        return has_tag(Tag<C>::ID);
    }

    template <typename... C>
    static bool has_tags()
    {
        const std::vector<TagID> query = {Tag<C>::ID...};
        bool res = true;
        for (const auto id : query)
        {
            res &= has_tag(id);
        }
        return res;
    }

private:
    static std::vector<TagID> tags;
};

template <typename... TAGS>
std::vector<TagID> Tag<TAGS...>::tags;

}
