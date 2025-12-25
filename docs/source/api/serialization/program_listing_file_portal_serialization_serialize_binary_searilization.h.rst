
.. _program_listing_file_portal_serialization_serialize_binary_searilization.h:

Program Listing for File binary_searilization.h
===============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_serialization_serialize_binary_searilization.h>` (``portal\serialization\serialize\binary_searilization.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/serialization/serialize.h"
   
   namespace portal
   {
   constexpr std::string_view MAGIC = "PS";
   constexpr uint8_t VERSION = 1;
   
   struct BinarySerializationParams
   {
       bool encode_header = true;
       bool large_element_size = false;
       // Will not write the sizes of vectors into memory, will not allow deserializing automatically
       bool pack_elements = false;
   };
   
   class BinarySerializer final : public Serializer
   {
   public:
       explicit BinarySerializer(std::ostream& output);
       BinarySerializer(std::ostream& output, BinarySerializationParams params);
   
   protected:
       void add_property(reflection::Property property) override;
   
   private:
       BinarySerializationParams params;
       std::ostream& output;
   };
   
   class BinaryDeserializer final : public Deserializer
   {
   public:
       explicit BinaryDeserializer(std::istream& input, bool read_header = true);
       BinaryDeserializer(std::istream& input, BinarySerializationParams params);
   
   protected:
       reflection::Property get_property() override;
   
   private:
       std::istream& input;
       BinarySerializationParams params;
   
       std::vector<char> buffer;
       size_t cursor = 0;
   };
   } // namespace portal
