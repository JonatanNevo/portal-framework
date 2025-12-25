.. _exhale_class_classportal_1_1ecs_1_1SystemBase:

Class SystemBase
================

- Defined in :ref:`file_portal_engine_ecs_system_base.h`


Inheritance Relationships
-------------------------

Derived Types
*************

- ``public portal::ecs::System< BaseCameraSystem, ecs::Owns< BaseCameraController >, ecs::Views< CameraComponent >, ecs::Views< TransformComponent > >`` (:ref:`exhale_class_classportal_1_1ecs_1_1System`)
- ``public portal::ecs::System< BasePlayerInputSystem, ecs::Owns< InputComponent >, ecs::Views< BaseCameraController >, ecs::Views< PlayerTag > >`` (:ref:`exhale_class_classportal_1_1ecs_1_1System`)
- ``public portal::ecs::System< EditorGuiSystem, ecs::Owns< NameComponent >, ecs::Views< RelationshipComponent >, ecs::Views< TransformComponent > >`` (:ref:`exhale_class_classportal_1_1ecs_1_1System`)
- ``public portal::ecs::System< SceneRenderingSystem, ecs::Owns< StaticMeshComponent >, ecs::Views< TransformComponent > >`` (:ref:`exhale_class_classportal_1_1ecs_1_1System`)
- ``public portal::ecs::System< TransformHierarchySystem, ecs::Owns< TransformDirtyTag >, ecs::Owns< TransformComponent > >`` (:ref:`exhale_class_classportal_1_1ecs_1_1System`)
- ``public portal::ecs::System< Derived, Components >`` (:ref:`exhale_class_classportal_1_1ecs_1_1System`)


Class Documentation
-------------------


.. doxygenclass:: portal::ecs::SystemBase
   :project: portal engine
   :members:
   :protected-members:
   :undoc-members: