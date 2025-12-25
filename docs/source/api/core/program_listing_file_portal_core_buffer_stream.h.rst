
.. _program_listing_file_portal_core_buffer_stream.h:

Program Listing for File buffer_stream.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_buffer_stream.h>` (``portal\core\buffer_stream.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <streambuf>
   #include <ostream>
   #include <istream>
   
   #include "buffer.h"
   
   namespace portal
   {
   class BufferStreamReader final : std::streambuf, public std::istream
   {
   public:
       explicit BufferStreamReader(const Buffer& buffer);
       size_t position() const { return _position; }
   
   protected:
       std::streamsize xsgetn(char* s, std::streamsize n) override;
       std::streampos seekoff(std::streamoff off, seekdir dir, openmode which) override;
   
   private:
       const Buffer& buffer;
       size_t _position;
   };
   
   class BufferStreamWriter final : std::streambuf, public std::ostream
   {
   public:
       explicit BufferStreamWriter(Buffer& buffer);
       Buffer get_buffer() const { return {buffer.data, position}; }
   
       size_t size() const { return position; }
       bool full() const { return position == buffer.size; }
   
   protected:
       std::streambuf::int_type overflow(std::streambuf::int_type ch) override;
       std::streamsize xsputn(const char* s, std::streamsize n) override;
   
   private:
       Buffer& buffer;
       size_t position;
   };
   }
