// //
// // Copyright © 2025 Jonatan Nevo.
// // Distributed under the MIT license (see LICENSE file).
// //
//
// #include "resource_registry.h"
//
// namespace portal::ng
// {
//
// static auto logger = Log::get_logger("Resources");
//
// ResourceRegistry::ResourceRegistry(ReferenceManager& ref_manager)
//     : reference_manager(ref_manager)
// {
//     resource_loader_thread = Thread{
//         "Resource Loader Thread",
//         [this](const std::stop_token& token)
//         {
//             this->resource_load_loop(token);
//         }
//     };
// }
//
// std::expected<Resource*, ResourceState> ResourceRegistry::get_resource(const ResourceHandle handle) const
// {
//     if (resources.contains(handle))
//         return resources.at(handle).get();
//
//     if (pending_resources.contains(handle))
//         return std::unexpected{ResourceState::Pending};
//
//     LOG_ERROR("Attempted to get resource with handle {} that does not exist", handle);
//     return std::unexpected{ResourceState::Missing};
// }
//
// ResourceHandle ResourceRegistry::create_resource(const StringId& resource_id, [[maybe_unused]] ResourceType type)
// {
//     const auto handle = to_resource_handle(resource_id);
//
//     if (resources.contains(handle) || pending_resources.contains(handle))
//         return handle;
//
//     // TODO: create future of resource in pending resources
//
//     return handle;
// }
//
// ResourceHandle ResourceRegistry::to_resource_handle(const StringId& resource_id)
// {
//     return resource_id.id;
// }
//
// // void ResourceRegistry::resource_load_loop(const std::stop_token& stoken)
// // {
// //     PORTAL_PROF_ZONE();
// //     ResourceRequest request;
// //     while (!stoken.stop_requested())
// //     {
// //         if (resource_load_queue.try_dequeue(request))
// //         {
// //             load_resource(request.resource_id, request.source);
// //         }
// //         else
// //         {
// //             // TODO: Should we sleep here instead?
// //             std::this_thread::yield();
// //         }
// //     };
// // }
//
//
//
// } // portal
