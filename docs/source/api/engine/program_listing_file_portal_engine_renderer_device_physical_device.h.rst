
.. _program_listing_file_portal_engine_renderer_device_physical_device.h:

Program Listing for File physical_device.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_device_physical_device.h>` (``portal\engine\renderer\device\physical_device.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <cstdint>
   #include <string>
   
   namespace portal::renderer
   {
   class Surface;
   
   struct DriverVersion
   {
       uint16_t major;
       uint16_t minor;
       uint16_t patch;
   };
   
   class PhysicalDevice
   {
   public:
       virtual ~PhysicalDevice() = default;
   
       [[nodiscard]] virtual DriverVersion get_driver_version() const = 0;
   
       [[nodiscard]] virtual bool is_extension_supported(std::string_view extensions_name) const = 0;
   
       [[nodiscard]] virtual bool supports_present(Surface& surface, uint32_t queue_family_index) const = 0;
   
       // TODO: should I add a api agnostic feature get/set?
   };
   } // portal
