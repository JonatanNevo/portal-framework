//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/resources/resource_reference.h"
#include "portal/core/reflection/property.h"
#include "portal/core/reflection/property_concepts.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/image/texture.h"

namespace portal::renderer
{
class ImageView;
class Image;

/**
 * @struct MaterialProperties
 * @brief Material creation parameters
 */
struct MaterialProperties
{
    StringId id;
    Reference<ShaderVariant> shader;

    size_t set_start_index = 0;
    size_t set_end_index = std::numeric_limits<size_t>::max();

    Reference<Texture> default_texture;
};

/**
 * @class Material
 * @brief Abstract material interface for shader parameter binding
 *
 * Binds shader parameters (uniforms, textures, images) by name using reflection.
 * Supports scalars, vectors, matrices, textures, and images.
 */
class Material : public Resource
{
public:
    DECLARE_RESOURCE(ResourceType::Material);

    explicit Material(const StringId& id) : Resource(id) {}

    /**
     * @brief Binds scalar uniform (int, float, etc.)
     * @tparam T Scalar type
     * @param bind_point Uniform name
     * @param t Value to bind
     */

    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    void set(const StringId bind_point, const T& t)
    {
        set_property(
            bind_point,
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::get_property_type<std::remove_const_t<T>>(),
                reflection::PropertyContainerType::scalar,
                1
            }
        );
    }

    /**
     * @brief Binds vector uniform (vec2, vec3, vec4)
     * @tparam T Vector type
     * @param bind_point Uniform name
     * @param t Value to bind
     */
    template <reflection::IsVec T>
    void set(const StringId bind_point, const T& t)
    {
        set_property(
            bind_point,
            reflection::Property{
                Buffer{&t, sizeof(T)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                T::length()
            }
        );
    }

    /**
     * @brief Binds matrix uniform (mat2, mat3, mat4)
     * @tparam T Matrix type
     * @param bind_point Uniform name
     * @param t Value to bind
     */
    template <reflection::IsMatrix T>
    void set(const StringId bind_point, const T& t)
    {
        set_property(
            bind_point,
            reflection::Property{
                Buffer{&t, sizeof(T)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::matrix,
                T::length() * T::length()
            }
        );
    }

    /** @brief Binds texture (resource reference) */
    virtual void set(StringId bind_point, const ResourceReference<Texture>& texture) = 0;

    /** @brief Binds texture */
    virtual void set(StringId bind_point, const Reference<Texture>& texture) = 0;

    /** @brief Binds image */
    virtual void set(StringId bind_point, const Reference<Image>& image) = 0;

    /** @brief Binds image view */
    virtual void set(StringId bind_point, const Reference<ImageView>& image) = 0;

    /**
     * @brief Gets scalar uniform value
     * @tparam T Scalar type
     * @param bind_point Uniform name
     * @return Reference to uniform value
     */
    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    T& get(const StringId bind_point)
    {
        auto prop = reflection::Property{
            reflection::get_property_type<std::remove_const_t<T>>(),
            reflection::PropertyContainerType::scalar,
            1
        };
        get_property(bind_point, prop);
        return *prop.value.as<T*>();
    }

    /**
     * @brief Gets vector uniform value
     * @tparam T Vector type
     * @param bind_point Uniform name
     * @return Reference to uniform value
     */
    template <reflection::IsVec T>
    T& get(const StringId bind_point)
    {
        auto prop = reflection::Property{
            reflection::get_property_type<typename T::value_type>(),
            reflection::PropertyContainerType::vector,
            T::length()
        };
        get_property(bind_point, prop);
        return *prop.value.as<T*>();
    }

    /**
     * @brief Gets matrix uniform value
     * @tparam T Matrix type
     * @param bind_point Uniform name
     * @return Reference to uniform value
     */
    template <reflection::IsMatrix T>
    T& get(const StringId bind_point)
    {
        auto prop = reflection::Property{
            reflection::get_property_type<typename T::value_type>(),
            reflection::PropertyContainerType::matrix,
            T::length() * T::length()
        };
        get_property(bind_point, prop);
        return *prop.value.as<T*>();
    }

    /**
     * @brief Gets bound texture
     * @param bind_point Texture binding name
     * @return Texture reference
     */
    virtual Reference<Texture> get_texture(const StringId bind_point) = 0;

    /**
     * @brief Gets bound image
     * @param bind_point Image binding name
     * @return Image reference
     */
    virtual Reference<Image> get_image(const StringId bind_point) = 0;

    /**
     * @brief Gets bound image view
     * @param bind_point Image view binding name
     * @return Image view reference
     */
    virtual Reference<ImageView> get_image_view(const StringId bind_point) = 0;

    /** @brief Gets material shader */
    virtual Reference<ShaderVariant> get_shader() = 0;

protected:
    /**
     * @brief Sets property by reflection
     * @param bind_point Property name
     * @param property Property data
     */
    virtual void set_property(StringId bind_point, const reflection::Property& property) = 0;

    /**
     * @brief Gets property by reflection
     * @param bind_point Property name
     * @param property Output property data
     * @return True if property found
     */
    virtual bool get_property(StringId bind_point, reflection::Property& property) const = 0;
};
}
