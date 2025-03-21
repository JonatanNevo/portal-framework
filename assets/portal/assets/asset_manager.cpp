// //
// // Created by Jonatan Nevo on 20/03/2025.
// //
//
// #include "asset_manager.h"
//
// namespace portal
// {
//
// AssetManager::AssetManager()
// {
//     asset_thread = Reference<AssetSystem>::create();
//     // AssetImporter::Init();
//
//     load_asset_registry();
//     asset_thread->set_registry(registry);
//     reload_assets();
// }
//
// AssetManager::~AssetManager()
// {
//     AssetManager::shutdown();
// }
//
// void AssetManager::shutdown()
// {
//     asset_thread->stop_and_wait();
//     write_registry_to_file();
// }
//
// AssetType AssetManager::get_asset_Type(const AssetHandle handle)
// {
//     if (!is_asset_handle_valid(handle))
//         return AssetType::Undefined;
//
//     if (is_memory_asset(handle))
//         return get_asset(handle)->get_asset_type();
//
//     const auto& metadata = get_metadata(handle);
//     return metadata.type;
// }
//
// Reference<Asset> AssetManager::get_asset(AssetHandle handle)
// {
//     // HZ_PROFILE_FUNC();
//     // HZ_SCOPE_PERF("AssetManager::GetAsset");
//
//     auto asset = get_asset_including_invalid(handle);
//     return asset && asset->is_valid() ? asset : Reference<Asset>(nullptr);
// }
//
// AsyncAssetResult<Asset> AssetManager::get_asset_async(AssetHandle handle)
// {
//     // HZ_PROFILE_FUNC();
//     // HZ_SCOPE_PERF("AssetManager::GetAssetAsync");
//
//     if (auto asset = get_memory_asset(handle); asset)
//     {
//         return AsyncAssetResult{asset, true};
//     }
//
//     const auto metadata = get_metadata(handle);
//     if (!metadata.is_valid())
//         return AsyncAssetResult{Reference<Asset>(nullptr)};
//
//     Reference<Asset> asset = nullptr;
//     if (metadata.is_data_loaded)
//     {
//         // PORTAL_CORE_ASSERT(is_physical_asset(handle), "Asset is not loaded but marked as loaded");
//     }
// }
//
// void AssetManager::add_memory_only_asset(Reference<Asset> asset) {
// }
//
// bool AssetManager::reload_data(AssetHandle handle) {
// }
//
// void AssetManager::reload_data_async(AssetHandle handle) {
// }
//
// bool AssetManager::ensure_current(AssetHandle handle) {
// }
//
// bool AssetManager::ensure_all_loaded_current() {
// }
//
// bool AssetManager::is_asset_handle_valid(AssetHandle handle) {
// }
//
// Reference<Asset> AssetManager::get_memory_asset(AssetHandle handle) {
// }
//
// bool AssetManager::is_asset_loaded(AssetHandle handle) {
// }
//
// bool AssetManager::is_asset_valid(AssetHandle handle) {
// }
//
// bool AssetManager::is_asset_missing(AssetHandle handle) {
// }
//
// bool AssetManager::is_memory_asset(AssetHandle handle) {
// }
//
// bool AssetManager::is_physical_asset(AssetHandle handle) {
// }
//
// bool AssetManager::remove_asset(AssetHandle handle) {
// }
//
// void AssetManager::register_dependency(AssetHandle dependency, AssetHandle dependent) {
// }
//
// void AssetManager::unregister_dependency(AssetHandle dependency, AssetHandle dependent) {
// }
//
// void AssetManager::unregister_dependencies(AssetHandle handle) {
// }
//
// std::unordered_set<AssetHandle> AssetManager::get_dependencies(AssetHandle handle) {
// }
//
// void AssetManager::sync_with_asset_thread() {
// }
//
// std::unordered_set<AssetHandle> AssetManager::get_all_assets_with_type(AssetType type) {
// }
//
// const std::unordered_map<AssetHandle, Reference<Asset>>& AssetManager::get_loaded_assets() {
// }
//
// std::unordered_map<AssetHandle, Reference<Asset>> AssetManager::get_memory_assets() {
// }
//
// AssetMetadata AssetManager::get_metadata(AssetHandle handle) {
// }
//
// void AssetManager::set_metadata(const AssetMetadata& metadata) {
// }
//
// AssetHandle AssetManager::import_asset(const std::filesystem::path& path) {
// }
//
// AssetHandle AssetManager::get_asset_handle_from_file(const std::filesystem::path& path) {
// }
//
// AssetType AssetManager::get_asset_type_from_extension(const std::string& extension) {
// }
//
// std::string AssetManager::get_default_extension_for_asset_type(AssetType type) {
// }
//
// AssetType AssetManager::get_asset_type_from_path(const std::filesystem::path& path) {
// }
//
// std::filesystem::path AssetManager::get_filesystem_path(AssetHandle handle) {
// }
//
// std::filesystem::path AssetManager::get_filesystem_path(const AssetMetadata& metadata) {
// }
//
// std::string AssetManager::get_filesystem_path_string(const AssetMetadata& metadata) {
// }
//
// std::filesystem::path AssetManager::get_relative_path(const std::filesystem::path& path) {
// }
//
// bool AssetManager::file_exists(const AssetMetadata& metadata) {
// }
//
// Reference<Asset> AssetManager::get_asset_including_invalid(AssetHandle handle) {
// }
//
// void AssetManager::load_asset_registry() {
// }
//
// void AssetManager::process_directory(const std::filesystem::path& path) {
// }
//
// void AssetManager::reload_assets() {
// }
//
// void AssetManager::write_registry_to_file() {
// }
//
// void AssetManager::on_asset_renamed(AssetHandle handle, const std::filesystem::path& new_path) {
// }
//
// void AssetManager::on_asset_deleted(AssetHandle handle) {
// }
//
// void AssetManager::update_dependents(AssetHandle handle) {
// }
// } // portal