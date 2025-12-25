
.. _program_listing_file_portal_engine_renderer_image_texture.cpp:

Program Listing for File texture.cpp
====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_image_texture.cpp>` (``portal\engine\renderer\image\texture.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "texture.h"
   
   namespace portal::renderer
   {
   const StringId Texture::MISSING_TEXTURE_ID = STRING_ID("missing_texture");
   const StringId Texture::WHITE_TEXTURE_ID = STRING_ID("white_texture");
   const StringId Texture::BLACK_TEXTURE_ID = STRING_ID("black_texture");
   }
