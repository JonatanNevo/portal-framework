
.. _program_listing_file_portal_core_reflection_property.h:

Program Listing for File property.h
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_reflection_property.h>` (``portal\core\reflection\property.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <portal/core/buffer.h>
   
   namespace portal::reflection
   {
   enum class PropertyType : uint8_t
   {
       binary           = 0,
       integer8         = 1,
       integer16        = 2,
       integer32        = 3,
       integer64        = 4,
       integer128       = 5,
       floating32       = 6,
       floating64       = 7,
       character        = 8,
       boolean          = 9,
       object           = 10,
       null_term_string = 11,
       string           = 12,
       invalid          = 255
   };
   
   enum class PropertyContainerType : uint8_t
   {
       invalid          = 0,
       scalar           = 1,
       array            = 2,
       string           = 3,
       null_term_string = 4,
       vector           = 5,
       matrix           = 6,
       object           = 7,
   };
   
   struct Property
   {
       Buffer value{};
       PropertyType type = PropertyType::invalid;
       PropertyContainerType container_type = PropertyContainerType::invalid;
       size_t elements_number = 0;
   
       bool operator==(const Property& other) const
       {
           return other.type == type && other.container_type == container_type && other.elements_number == elements_number;
       }
   };
   
   constexpr PropertyType get_integer_type(const size_t size)
   {
       if (size == 1)
           return PropertyType::integer8;
       if (size == 2)
           return PropertyType::integer16;
       if (size == 4)
           return PropertyType::integer32;
       if (size == 8)
           return PropertyType::integer64;
       if (size == 16)
           return PropertyType::integer128;
       return PropertyType::invalid;
   }
   
   constexpr PropertyType get_float_type(const size_t size)
   {
       if (size == 4)
           return PropertyType::floating32;
       if (size == 8)
           return PropertyType::floating64;
       return PropertyType::invalid;
   }
   
   constexpr size_t get_property_size(const Property& prop)
   {
       size_t base_size = 0;
       switch (prop.type)
       {
       case PropertyType::binary:
           base_size = sizeof(std::byte);
           break;
       case PropertyType::integer8:
           base_size = sizeof(std::uint8_t);
           break;
       case PropertyType::integer16:
           base_size = sizeof(std::uint16_t);
           break;
       case PropertyType::integer32:
           base_size = sizeof(std::uint32_t);
           break;
       case PropertyType::integer64:
           base_size = sizeof(std::uint64_t);
           break;
       case PropertyType::integer128:
           base_size = sizeof(portal::uint128_t);
           break;
       case PropertyType::floating32:
           base_size = sizeof(float);
           break;
       case PropertyType::floating64:
           base_size = sizeof(double);
           break;
       case PropertyType::character:
           base_size = sizeof(char);
           break;
       case PropertyType::boolean:
           base_size = sizeof(bool);
           break;
       case PropertyType::object:
           LOG_WARN_TAG("Reflection", "Attempting to fetch size of object property");
           base_size = 0;
           break;
       case PropertyType::null_term_string:
       case PropertyType::string:
           base_size = sizeof(char);
           break;
       case PropertyType::invalid:
           LOG_WARN_TAG("Reflection", "Attempting to fetch size of an invalid property");
           base_size = 0;
           break;
       }
   
       return base_size * prop.elements_number;
   }
   }
   
   template <>
   struct std::formatter<portal::reflection::Property>
   {
       static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
       {
           return ctx.begin();
       }
   
       template <typename FormatContext>
       auto format(const portal::reflection::Property& prop, FormatContext& ctx) const
       {
           if (prop.value.data == nullptr)
               return std::format_to(
                   ctx.out(),
                   "Property(.type={}, .container_type={}, .elements_number={})",
                   prop.type,
                   prop.container_type,
                   prop.elements_number
               );
   
           return std::format_to(
               ctx.out(),
               "Property(.value=Buffer(.size={}), .type={}, .container_type={}, .elements_number={})",
               prop.value.size,
               prop.type,
               prop.container_type,
               prop.elements_number
           );
       }
   };
