
.. _program_listing_file_portal_engine_renderer_shaders_shader.cpp:

Program Listing for File shader.cpp
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_shaders_shader.cpp>` (``portal\engine\renderer\shaders\shader.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "shader.h"
   
   #include <ranges>
   #include <nlohmann/detail/input/parser.hpp>
   
   #include "portal/core/reflection/property_concepts.h"
   #include "portal/engine/renderer/shaders/shader_compiler.h"
   #include "portal/serialization/serialize.h"
   
   namespace portal::renderer
   {
   static auto logger = Log::get_logger("Shader");
   
   Shader::Shader(const StringId& id) : Resource(id)
   {}
   
   void Shader::load_source(Buffer&& new_source, const std::filesystem::path& shader_path)
   {
       source_path = shader_path;
       source = std::move(new_source);
   }
   
   uint64_t Shader::compile_with_permutations(const std::vector<ShaderDefine>& permutations)
   {
       const auto permutations_hash = calculate_permutations_hash(permutations);
   
       std::lock_guard lock(shader_cache_lock);
       if (!shaders.contains(permutations_hash))
       {
           LOGGER_DEBUG("Compiling shader variant: {} [{}]", id, permutations_hash);
           ShaderCompiler compiler;
           shaders[permutations_hash] = compiler.compile({.name = id, .shader_path = source_path, .shader_data = source, .defines = permutations});
       }
   
       return permutations_hash;
   }
   
   
   uint64_t Shader::calculate_permutations_hash(const std::vector<ShaderDefine>& permutations) const
   {
       uint64_t hash = id.id;
       for (const auto& [name, value] : permutations)
       {
           hash ^= hash::rapidhash(name);
       }
       return hash;
   }
   }
