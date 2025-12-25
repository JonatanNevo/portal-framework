
.. _program_listing_file_portal_input_input_events.h:

Program Listing for File input_events.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_input_input_events.h>` (``portal\input\input_events.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/core/events/event.h"
   #include "input_types.h"
   
   namespace portal
   {
   class KeyPressedEvent final : public Event
   {
   public:
       KeyPressedEvent(const Key key, const KeyModifierFlag modifiers) : key(key), active_modifiers(modifiers) {}
   
       [[nodiscard]] Key get_key() const { return key; }
       [[nodiscard]] KeyModifierFlag get_modifiers() const { return active_modifiers; }
   
       EVENT_CLASS_TYPE(KeyPressed)
       EVENT_CLASS_CATEGORY(Input)
   
   private:
       Key key;
       KeyModifierFlag active_modifiers;
   };
   
   class KeyReleasedEvent final : public Event
   {
   public:
       explicit KeyReleasedEvent(const Key key) : key(key) {}
   
       [[nodiscard]] Key get_key() const { return key; }
   
       EVENT_CLASS_TYPE(KeyReleased)
       EVENT_CLASS_CATEGORY(Input)
   
   private:
       Key key;
   };
   
   class KeyRepeatEvent final : public Event
   {
   public:
       explicit KeyRepeatEvent(const Key key, const KeyModifierFlag modifiers) : key(key), active_modifiers(modifiers) {}
   
       [[nodiscard]] Key get_key() const { return key; }
       [[nodiscard]] KeyModifierFlag get_modifiers() const { return active_modifiers; }
   
       EVENT_CLASS_TYPE(KeyRepeat)
       EVENT_CLASS_CATEGORY(Input)
   
   private:
       Key key;
       KeyModifierFlag active_modifiers;
   };
   
   
   class MouseMovedEvent final : public Event
   {
   public:
       explicit MouseMovedEvent(glm::vec2 pos) : position(pos) {}
   
       [[nodiscard]] float get_x() const { return position.x; }
       [[nodiscard]] float get_y() const { return position.y; }
       [[nodiscard]] glm::vec2 get_position() const { return position; }
   
       EVENT_CLASS_TYPE(MouseMoved)
       EVENT_CLASS_CATEGORY(Input)
   
   private:
       glm::vec2 position;
   };
   
   class MouseScrolledEvent final : public Event
   {
   public:
       explicit MouseScrolledEvent(glm::vec2 offset) : offset(offset) {}
   
       [[nodiscard]] float get_x_offset() const { return offset.x; }
       [[nodiscard]] float get_y_offset() const { return offset.y; }
       [[nodiscard]] glm::vec2 get_offset() const { return offset; }
   
       EVENT_CLASS_TYPE(MouseScrolled)
       EVENT_CLASS_CATEGORY(Input)
   
   private:
       glm::vec2 offset;
   };
   
   class SetMouseCursorEvent final : public Event
   {
   public:
       explicit SetMouseCursorEvent(const CursorMode mode) : mode(mode) {}
       [[nodiscard]] CursorMode get_mode() const { return mode; }
   
       EVENT_CLASS_TYPE(SetMouseCursor)
       EVENT_CLASS_CATEGORY(Input)
   
   private:
       CursorMode mode;
   };
   }
