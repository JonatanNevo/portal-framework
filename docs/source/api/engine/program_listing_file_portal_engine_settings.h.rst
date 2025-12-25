
.. _program_listing_file_portal_engine_settings.h:

Program Listing for File settings.h
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_settings.h>` (``portal\engine\settings.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <filesystem>
   #include <ranges>
   
   #include "portal/core/concurrency/reentrant_spin_lock.h"
   #include "portal/core/concurrency/spin_lock.h"
   #include "portal/serialization/archive.h"
   
   namespace portal
   {
   enum class SettingsArchiveType
   {
       Json
   };
   
   class Settings final : public ArchiveObject
   {
   public:
       static void init(SettingsArchiveType type, const std::filesystem::path& path);
       static void shutdown();
       static Settings& get();
   
       ~Settings() override;
   
       void load();
       void dump() const;
   
       template <typename T>
       std::optional<T> get_setting(const PropertyName& name)
       {
           auto split_view = name | std::ranges::views::split('.');
           auto split_size = std::ranges::distance(split_view);
   
           ArchiveObject* current_node = root.get();
           for (auto part : split_view)
           {
               std::string_view part_str{part};
   
   
               if (--split_size == 0)
               {
                   T out;
                   if (current_node->get_property(part_str, out))
                       return out;
                   return std::nullopt;
               }
   
               current_node = current_node->get_object(part_str);
               if (!current_node)
               {
                   LOG_ERROR_TAG("Settings", "Failed to get setting \"{}\"", name);
                   return std::nullopt;
               }
           }
   
           return std::nullopt;
       }
   
       void debug_print() const;
   
   private:
       void debug_print(std::string base_name, const ArchiveObject& object) const;
       void debug_print_scalar(const std::string& name, const reflection::Property& prop) const;
       void debug_print_array(const std::string& name, const reflection::Property& prop) const;
   
       explicit Settings(SettingsArchiveType type, const std::filesystem::path& path);
   
   protected:
       reflection::Property& get_property_from_map(PropertyName name) override;
       reflection::Property& add_property_to_map(PropertyName name, reflection::Property&& property) override;
   
   private:
       ReentrantSpinLock<> lock;
       SettingsArchiveType type;
   
       std::filesystem::path settings_path;
       // Memory ownership of properties
       std::unique_ptr<ArchiveObject> root;
   };
   } // portal
