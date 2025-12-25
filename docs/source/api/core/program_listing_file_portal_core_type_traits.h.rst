
.. _program_listing_file_portal_core_type_traits.h:

Program Listing for File type_traits.h
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_type_traits.h>` (``portal\core\type_traits.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <type_traits>
   
   namespace portal
   {
   template <typename, template <typename...> typename>
   struct is_specialization_of : std::false_type {};
   
   template <template <typename...> typename Template, typename... Args>
   struct is_specialization_of<Template<Args...>, Template> : std::true_type {};
   
   template <typename T, template <typename...> typename Template>
   inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;
   
   template <typename...>
   struct type_list {};
   
   template <typename...>
   struct concat;
   
   template <>
   struct concat<>
   {
       using type = type_list<>;
   };
   
   template <typename... Ts>
   struct concat<type_list<Ts...>>
   {
       using type = type_list<Ts...>;
   };
   
   template <typename... Ts, typename... Us, typename... Rest>
   struct concat<type_list<Ts...>, type_list<Us...>, Rest...>
   {
       using type = typename concat<type_list<Ts..., Us...>, Rest...>::type;
   };
   
   template <typename... Lists>
   using concat_t = typename concat<Lists...>::type;
   
   // Filter types based on a predicate
   template <template <typename> class Pred, typename... Ts>
   struct filter;
   
   template <template <typename> class Pred>
   struct filter<Pred>
   {
       using type = type_list<>;
   };
   
   template <template <typename> class Pred, typename T, typename... Ts>
   struct filter<Pred, T, Ts...>
   {
       using rest = typename filter<Pred, Ts...>::type;
       using type = std::conditional_t<
           Pred<T>::value,
           concat_t<type_list<T>, rest>,
           rest
       >;
   };
   
   template <template <typename> class Pred, typename... Ts>
   using filter_t = typename filter<Pred, Ts...>::type;
   }
