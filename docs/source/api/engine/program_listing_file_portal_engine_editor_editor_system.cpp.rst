
.. _program_listing_file_portal_engine_editor_editor_system.cpp:

Program Listing for File editor_system.cpp
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_editor_editor_system.cpp>` (``portal\engine\editor\editor_system.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "editor_system.h"
   
   #include <imgui.h>
   
   #include "portal/engine/components/base_camera_controller.h"
   #include "portal/engine/components/camera.h"
   #include "portal/engine/components/mesh.h"
   #include "portal/engine/components/transform.h"
   
   namespace portal
   {
   void EditorGuiSystem::execute(ecs::Registry& registry)
   {
       print_cameras(registry);
       print_scene_graph(registry);
   }
   
   void EditorGuiSystem::print_cameras(ecs::Registry& registry)
   {
       ImGui::Begin("Camera");
       const auto camera_view = registry.view<CameraComponent, BaseCameraController, TransformComponent, NameComponent>();
       for (auto entity : camera_view)
       {
           auto& camera = entity.get_component<CameraComponent>();
           auto& controller = entity.get_component<BaseCameraController>();
           auto& transform = entity.get_component<TransformComponent>();
           auto& name = entity.get_component<NameComponent>();
   
           ImGui::Text("Camera: %s", name.name.string.data());
           if (entity.has_component<MainCameraTag>()) ImGui::Text("Main Camera");
           ImGui::Text("position %f %f %f", transform.get_translation().x, transform.get_translation().y, transform.get_translation().z);
           ImGui::Text("direction %f %f %f", controller.forward_direction.x, controller.forward_direction.y, controller.forward_direction.z);
   
           ImGui::SliderFloat("Camera Speed", &controller.speed, 0.1f, 10.0f);
           ImGui::InputFloat("Near Clip", &camera.near_clip);
           ImGui::InputFloat("Far Clip", &camera.far_clip);
       }
       ImGui::End();
   }
   
   void EditorGuiSystem::print_scene_graph(ecs::Registry& registry)
   {
       const auto relationship_group = group(registry);
       relationship_group.sort(
           [&registry](const entt::entity lhs_raw, const entt::entity rhs_raw)
           {
               auto lhs = registry.entity_from_id(lhs_raw);
               auto rhs = registry.entity_from_id(rhs_raw);
   
               const auto& [left_name] = lhs.get_component<NameComponent>();
               const auto& [right_name] = rhs.get_component<NameComponent>();
   
               return left_name.string < right_name.string;
           }
       );
   
       auto draw_node = [](auto& self, Entity entity, int& node_id) -> void
       {
           auto& relationship = entity.get_component<RelationshipComponent>();
           auto& name = entity.get_component<NameComponent>();
   
           ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
           if (relationship.children == 0)
           {
               flags |= ImGuiTreeNodeFlags_Leaf;
           }
   
           ImGui::PushID(node_id++);
   
           const bool is_mesh = entity.has_component<StaticMeshComponent>();
           if (is_mesh)
           {
               ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
           }
   
           const bool open = ImGui::TreeNodeEx(name.name.string.data(), flags);
   
           if (is_mesh)
           {
               ImGui::PopStyleColor();
           }
   
           if (ImGui::IsItemHovered())
           {
               auto& transform = entity.get_component<TransformComponent>();
   
               ImGui::BeginTooltip();
               const auto& translate = glm::vec3(transform.get_world_matrix()[3]);
               ImGui::Text("Position: %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
   
               if (is_mesh)
               {
                   auto& mesh = entity.get_component<StaticMeshComponent>();
   
                   ImGui::Text("Mesh: %s", mesh.mesh->get_id().string.data());
                   for (auto& material : mesh.materials)
                   {
                       ImGui::Text("Material: %s", material->get_id().string.data());
                   }
               }
               ImGui::EndTooltip();
           }
   
           if (open)
           {
               for (const auto& child : entity.children())
               {
                   self(self, child, node_id);
               }
               ImGui::TreePop();
           }
   
           ImGui::PopID();
       };
   
       ImGui::Begin("Scene");
   
       ImGui::Text("Scene Graph");
       ImGui::Separator();
       int node_id = 0;
   
       for (auto&& [entity, _, relationship, __] : relationship_group.each())
       {
           // Iterating only on the root entities
           if (relationship.parent != null_entity)
               continue;
   
           draw_node(draw_node, registry.entity_from_id(entity), node_id);
       }
       ImGui::End();
   }
   } // portal
