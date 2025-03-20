//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "portal/application/platform/platform.h"

namespace portal::scene_graph
{
class Node;
class Component;

/// @brief A collection of nodes organized in a tree structure.
///		   It can contain more than one root node.
class Scene
{
public:
    Scene() = default;
    explicit Scene(const std::string& name);

    void set_name(const std::string& name);
    const std::string& get_name() const;

    void set_nodes(std::vector<std::unique_ptr<Node>>&& nodes);
    void add_node(std::unique_ptr<Node>&& node);
    void add_child(Node& child) const;

    std::unique_ptr<Component> get_model(uint32_t index = 0);

    void add_component(std::unique_ptr<Component>&& component);
    void add_component(std::unique_ptr<Component>&& component, Node& node);

    /**
     * @brief Set list of components for the given type
     * @param type_info The type of the component
     * @param components The list of components (retained)
     */
    void set_components(const std::type_index& type_info, std::vector<std::unique_ptr<Component>>&& components);

    /**
     * @brief Set list of components casted from the given template type
     */
    template <class T>
    void set_components(std::vector<std::unique_ptr<T>>&& components)
    {
        std::vector<std::unique_ptr<Component>> result(components.size());
        std::ranges::transform(
            components,
            result.begin(),
            [&](auto& component)
            {
                return std::unique_ptr<Component>(std::move(component));
            }
        );
        set_components(typeid(T), std::move(result));
    }

    /**
     * @brief Clears a list of components
     */
    template <class T>
    void clear_components()
    {
        set_components(typeid(T), {});
    }

    /**
     * @return List of pointers to components casted to the given template type
     */
    template <class T>
    std::vector<T*> get_components() const
    {
        std::vector<T*> result;
        if (has_component(typeid(T)))
        {
            auto& scene_components = get_components(typeid(T));

            result.resize(scene_components.size());
            std::ranges::transform(
                scene_components,
                result.begin(),
                [&](auto& component)
                {
                    return dynamic_cast<T*>(component.get());
                }
            );
        }

        return result;
    }

    /**
     * @return List of components for the given type
     */
    const std::vector<std::unique_ptr<Component>>& get_components(const std::type_index& type_info) const;

    template <class T>
    bool has_component() const
    {
        return has_component(typeid(T));
    }

    bool has_component(const std::type_index& type_info) const;


    Node* find_node(const std::string& name) const;
    void set_root_node(Node& node);
    Node& get_root_node() const;

private:
    std::string name;
    /// List of all the nodes
    std::vector<std::unique_ptr<Node>> nodes;
    Node* root = nullptr;
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> components;
};
} // portal
