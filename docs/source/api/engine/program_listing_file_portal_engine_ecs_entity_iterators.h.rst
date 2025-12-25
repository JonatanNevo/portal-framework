
.. _program_listing_file_portal_engine_ecs_entity_iterators.h:

Program Listing for File entity_iterators.h
===========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_ecs_entity_iterators.h>` (``portal\engine\ecs\entity_iterators.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <entt/entt.hpp>
   #include "llvm/ADT/SmallVector.h"
   
   namespace portal
   {
   class Entity;
   
   class ChildIterator
   {
   public:
       using iterator_category = std::forward_iterator_tag;
       using value_type = Entity;
       using difference_type = std::ptrdiff_t;
       using pointer = value_type*;
       using reference = value_type&;
   
       ChildIterator(entt::entity current, entt::registry* registry);
   
       Entity operator*() const;
       Entity operator->() const;
       ChildIterator& operator++();
       ChildIterator operator++(int);
       bool operator==(const ChildIterator& other) const;
       bool operator!=(const ChildIterator& other) const;
   
   private:
       entt::entity current;
       entt::registry* registry;
   };
   
   // Iterator for recursive traversal of all descendants (depth-first)
   class RecursiveChildIterator
   {
   public:
       using iterator_category = std::forward_iterator_tag;
       using value_type = Entity;
       using difference_type = std::ptrdiff_t;
       using pointer = value_type*;
       using reference = value_type&;
   
       RecursiveChildIterator(entt::entity start, entt::registry* registry, bool is_end = false);
   
       Entity operator*() const;
       Entity operator->() const;
       RecursiveChildIterator& operator++();
       RecursiveChildIterator operator++(int);
       bool operator==(const RecursiveChildIterator& other) const;
       bool operator!=(const RecursiveChildIterator& other) const;
   
   private:
       void advance_to_next();
   
       entt::entity current;
       entt::registry* registry;
       llvm::SmallVector<entt::entity, 8> stack;
   };
   
   class ChildRange
   {
   public:
       ChildRange(const Entity& entity);
       ChildIterator begin() const;
       ChildIterator end() const;
   
   private:
       const Entity& entity;
   };
   
   class RecursiveChildRange
   {
   public:
       RecursiveChildRange(const Entity& entity);
       RecursiveChildIterator begin() const;
       RecursiveChildIterator end() const;
   
   private:
       const Entity& entity;
   };
   } // namespace portal
