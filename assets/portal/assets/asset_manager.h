//
// Created by Jonatan Nevo on 20/03/2025.
//

#pragma once
#include <unordered_set>
#include <shared_mutex>

#include "portal/assets/asset.h"
#include "portal/assets/asset_registry.h"
#include "portal/assets/asset_system.h"
#include "portal/assets/asset_types.h"
#include "portal/core/file_system.h"


namespace portal
{

class AssetManager: public CountedReference
{
public:
    AssetManager();
    ~AssetManager() override;

    virtual void shutdown();

    virtual AssetType get_asset_Type(AssetHandle handle);
    virtual Reference<Asset> get_asset(AssetHandle handle);
    virtual AsyncAssetResult<Asset> get_asset_async(AssetHandle handle);

    virtual void add_memory_only_asset(Reference<Asset> asset);
    virtual bool reload_data(AssetHandle handle);
    virtual void reload_data_async(AssetHandle handle);
    virtual bool ensure_current(AssetHandle handle);
    virtual bool ensure_all_loaded_current();

    /**
     * Checks if the asset handle is valid, does not check the underlying asset
     * @param handle The asset handle.
     * @return true if the handle is valid, false otherwise.
     */
    virtual bool is_asset_handle_valid(AssetHandle handle);

    /**
     * If the asset is memory only, returns the asset, otherwise returns nullptr.
     * This is more efficient than `is_memory_asset` followed by `get_asset`.
     *
     * @param handle The asset handle.
     * @return The asset if it is memory only, nullptr otherwise.
     */
    virtual Reference<Asset> get_memory_asset(AssetHandle handle);

    /**
     * Checks if the asset has been loaded from the file, the asset can still be invalid
     *
     * @param handle The asset handle.
     * @return true if the asset has been loaded, false otherwise.
     */
    virtual bool is_asset_loaded(AssetHandle handle);

    /**
     * Checks if the asset has been loaded, and the data is valid (no file corruption, or unknown format)
     *
     * @param handle The asset handle.
     * @return true if the asset is valid, false otherwise.
     */
    virtual bool is_asset_valid(AssetHandle handle);

    /**
     * Checks if the asset exists
     *
     * @param handle The asset handle.
     * @return true if the asset exists, false otherwise.
     */
    virtual bool is_asset_missing(AssetHandle handle);

    virtual bool is_memory_asset(AssetHandle handle);
    virtual bool is_physical_asset(AssetHandle handle);
    virtual bool remove_asset(AssetHandle handle);

    /**
     * Registers a dependency between two assets.
     * For example: the dependent is a material and the dependency is a texture the material uses
     *
     * @param dependency The asset that the dependent depends on
     * @param dependent The asset that depends on the dependency
     */
    virtual void register_dependency(AssetHandle dependency, AssetHandle dependent);

    /**
     * Unregisters a dependency between two assets.
     *
     * @param dependency The asset that the dependent depends on
     * @param dependent The asset that depends on the dependency
     */
    virtual void unregister_dependency(AssetHandle dependency, AssetHandle dependent);

    /**
     * Unregister all dependencies for a given asset
     *
     * @param handle The asset that depends on the dependencies
     */
    virtual void unregister_dependencies(AssetHandle handle);

    /**
     * Get all dependencies for a given asset
     * @param handle The asset that depends on the dependencies
     * @return A set of asset handles that the given asset depends on
     */
    virtual std::unordered_set<AssetHandle> get_dependencies(AssetHandle handle);

    virtual void sync_with_asset_thread();

    virtual std::unordered_set<AssetHandle> get_all_assets_with_type(AssetType type);
    virtual const std::unordered_map<AssetHandle, Reference<Asset>>& get_loaded_assets();



    WeakReference<AssetRegistry> get_asset_registry() const { return WeakReference{registry}; }

    std::unordered_map<AssetHandle, Reference<Asset>> get_memory_assets();

    // note: GetMetadata(AssetHandle) is the ONLY EditorAssetManager function that it is safe to call
    //       from any thread.
    //       All other methods on EditorAssetManager are thread-unsafe and should only be called from the main thread.
    //       SetMetadata() must only be called from main-thread, otherwise it will break safety of all the other
    //       unsynchronized EditorAssetManager functions.
    //
    // thread-safe access to metadata
    // This function returns an AssetMetadata (specifically not a reference) as with references there is no guarantee
    // that the referred to data doesn't get modified (or even destroyed) by another thread
    AssetMetadata get_metadata(AssetHandle handle);

    // thread-safe modification of metadata
    void set_metadata(const AssetMetadata& metadata);

    AssetHandle import_asset(const std::filesystem::path& path);
    AssetHandle get_asset_handle_from_file(const std::filesystem::path& path);

    AssetType get_asset_type_from_extension(const std::string& extension);
    std::string get_default_extension_for_asset_type(AssetType type);
    AssetType get_asset_type_from_path(const std::filesystem::path& path);

    std::filesystem::path get_filesystem_path(AssetHandle handle);
    std::filesystem::path get_filesystem_path(const AssetMetadata& metadata);
    std::string get_filesystem_path_string(const AssetMetadata& metadata);
    std::filesystem::path get_relative_path(const std::filesystem::path& path);

    bool file_exists(const AssetMetadata& metadata);

    template<typename T, typename... Args> requires std::is_base_of_v<Asset, T>
    Reference<T> create_or_replace_asset(const std::filesystem::path& path, Args&&... args)
    {
        // Check if asset for this file already exists.
        // If it does, and its the same type we just replace existing asset
        // Otherwise we create a whole new asset.
        auto relative_path = get_relative_path(path);
        auto handle = get_asset_handle_from_file(relative_path);
        AssetMetadata metadata = handle ? get_metadata(handle) : AssetMetadata{};
        if (metadata.type != T::GetStaticType())
            metadata = {};

        bool replace_asset = false;
        if (metadata.handle == 0)
        {
            metadata.handle = {};
            metadata.path = relative_path;
            metadata.type = T::GetStaticType();
            metadata.is_data_loaded = true;
            set_metadata(metadata);
            write_registry_to_file();
        }
        else
            replace_asset = true;

        auto asset = Reference<T>::Create(std::forward<Args>(args)...);
        asset->handle = handle;
        asset_thread->mark_asset_as_loaded(handle, asset);

        //AssetImporter::Serialize(metadata, asset);

        // Read serialized timestamp
        auto absulute_path = get_filesystem_path(metadata);
        metadata.file_last_write_time = FileSystem::get_last_write_time(absulute_path);
        set_metadata(metadata);

        if (replace_asset)
        {
            LOG_CORE_INFO_TAG("Assets", "Replaced assets {}", metadata.path.string());
            update_dependents(metadata.handle);
            //Application::Get().DispatchEvent<AssetReloadedEvent, /*DispatchImmediately=*/true>(metadata.Handle);
        }

        return asset;
    }

    void replace_loaded_asset(const AssetHandle& handle, const Reference<Asset>& asset)
    {
        asset_thread->mark_asset_as_loaded(handle, asset);
    }

private:
    Reference<Asset> get_asset_including_invalid(AssetHandle handle);

    void load_asset_registry();
    void process_directory(const std::filesystem::path& path);
    void reload_assets();
    void write_registry_to_file();

    void on_asset_renamed(AssetHandle handle, const std::filesystem::path& new_path);
    void on_asset_deleted(AssetHandle handle);

    void update_dependents(AssetHandle handle);

private:
    // These can be accessed from all threads
    std::unordered_map<AssetHandle, Reference<Asset>> memory_assets;
    std::shared_mutex memory_asset_mutex;

    std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> assets_dependents;
    std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> assets_dependencies;
    std::shared_mutex assets_dependencies_mutex;

    Reference<AssetRegistry> registry;
    Reference<AssetSystem> asset_thread;

    friend class AssetSystem;
};

} // portal
