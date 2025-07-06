//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "portal/serialization/archive/archive.h"

namespace portal
{

ArchiveObject::ArchiveObject(TreeArchiveBase* target): property_map(target->allocator), paret_archive(target) {}

ArchiveObject::~ArchiveObject() {
    erase_all();
}

float ArchiveObject::get_version() const
{
    return paret_archive ? paret_archive->get_version() : -1.0f;
}

void ArchiveObject::set_property(const std::string_view name, bool b)
{
    add_property_to_map(
        archiving::Property{
            .value = Buffer::copy(b),
            .type = archiving::PropertyType::boolean,
            .container_type = archiving::PropertyContainerType::scalar,
            .elements_number = 1
        },
        name
        );
}

void ArchiveObject::set_property(PropertyName name, const std::string& str)
{
    add_property_to_map()
}



bool ArchiveObject::get_property(PropertyName name, int& n) {
}

bool ArchiveObject::get_property(PropertyName name, int64_t& n) {
}

bool ArchiveObject::get_property(PropertyName name, uint64_t& n) {
}

bool ArchiveObject::get_property(PropertyName name, float& f) {
}

bool ArchiveObject::get_property(PropertyName name, double& f) {
}

bool ArchiveObject::get_property(PropertyName name, bool& b) {
}

bool ArchiveObject::get_property(PropertyName name, std::string& str) {
}

void ArchiveObject::set_binary_block(PropertyName name, const llvm::SmallVector<byte>& data) {
}

bool ArchiveObject::get_binary_block(PropertyName name, llvm::SmallVector<byte>& out) const {
}

ArchiveObject* ArchiveObject::get_object(PropertyName name)
{
    return const_cast<ArchiveObject*>(static_cast<const ArchiveObject*>(this)->get_object(name));
}

const ArchiveObject* ArchiveObject::get_object(PropertyName name) const
{
    const auto* object = find_property(name);
    if (!object)
        return nullptr;

    if (object->type == archiving::PropertyType::invalid)
        return nullptr;

    PORTAL_ASSERT(
        object->type == archiving::PropertyType::object,
        "Property {} is not an object, type: {}",
        name,
        static_cast<uint8_t>(object->type)
        );

    return static_cast<const ArchiveObject*>(object->value.data);
}

ArchiveObject* ArchiveObject::create_child(PropertyName name)
{
    archiving::Property prop;
    prop.type = archiving::PropertyType::object;
    auto buffer = Buffer();
    buffer.allocate(sizeof(ArchiveObject));
    new(buffer.data) ArchiveObject(paret_archive);
    prop.value = std::move(buffer);

    const auto& node = add_property_to_map(std::move(prop), name);

    return static_cast<ArchiveObject*>(node.value.data);
}

ArchiveObject* ArchiveObject::child(PropertyName name)
{
    auto& node = access_property_in_map(name);
    PORTAL_ASSERT(
        node.type == archiving::PropertyType::invalid || node.type == archiving::PropertyType::object,
        "Property {} already exists or is not an object",
        name
        );
    if (node.type != archiving::PropertyType::object)
    {
        node.type = archiving::PropertyType::object;
        auto buffer = Buffer();
        buffer.allocate(sizeof(ArchiveObject));
        new(buffer.data) ArchiveObject(paret_archive);
        node.value = std::move(buffer);
    }

    return static_cast<ArchiveObject*>(node.value.data);
}

void ArchiveObject::delete_object(PropertyName name) {
}

ArchiveObjectIterator ArchiveObject::get_first_object() {
}

ArchiveObjectIterator ArchiveObject::begin() {
}

ArchiveObjectIterator ArchiveObject::end() {
}

ArchiveObjectIterator ArchiveObject::begin() const {
}

ArchiveObjectIterator ArchiveObject::end() const {
}

llvm::SmallVector<ArchiveObject::PropertyDefinition, 20> ArchiveObject::get_properties() const {
}

archiving::Property& ArchiveObject::add_property_to_map(archiving::Property&& property, PropertyName name)
{
    auto& prop = access_property_in_map(name);
    // TODO: add checks?
    prop = std::move(property);
    return prop;
}

archiving::Property& ArchiveObject::access_property_in_map(PropertyName name)
{
    return property_map[name];
}

const archiving::Property* ArchiveObject::find_property(PropertyName name) const
{
    const auto it = property_map.find(name);
    if (it == property_map.end())
        return nullptr;

    return &it->second;
}


void ArchiveObject::erase_archive_object(ArchiveObject* object) {
    for (auto& [_, prop]: object->property_map)
    {
        if (prop.type == archiving::PropertyType::object)
            erase_archive_object(prop.value.as<ArchiveObject>());

        if (prop.value.allocated)
            prop.value.release();
    }
}


void ArchiveObject::erase_all() {
    erase_archive_object(this);
    property_map.clear();
}

void ArchiveObject::delete_property(std::string_view object_name) {
}

const std::type_info ArchiveObject::get_property_type(std::string_view name) const {
}

void ArchiveObject::set_property(std::string_view name, uint128_t n) {
}

void ArchiveObject::set_property(std::string_view name, std::string_view str) {
}

void ArchiveObject::set_property(std::string_view name, const char* str) {
}
}
