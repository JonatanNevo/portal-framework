
.. _program_listing_file_portal_core_jobs_basic_coroutine.h:

Program Listing for File basic_coroutine.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_jobs_basic_coroutine.h>` (``portal\core\jobs\basic_coroutine.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <coroutine>
   
   namespace portal
   {
   class BasicCoroutine
   {
   public:
       struct Promise
       {
           static BasicCoroutine get_return_object() { return {}; }
   
           static void unhandled_exception() noexcept {}
           static void return_void() noexcept {}
   
           static std::suspend_never initial_suspend() noexcept { return {}; }
           static std::suspend_never final_suspend() noexcept { return {}; }
       };
   
       using promise_type = Promise;
   };
   
   template <typename Awaitable>
   BasicCoroutine execute(Awaitable awaitable)
   {
       co_await awaitable;
   }
   }
