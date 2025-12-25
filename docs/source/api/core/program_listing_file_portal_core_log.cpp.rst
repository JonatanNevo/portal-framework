
.. _program_listing_file_portal_core_log.cpp:

Program Listing for File log.cpp
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_log.cpp>` (``portal\core\log.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "log.h"
   
   #include <spdlog/sinks/basic_file_sink.h>
   #include <spdlog/sinks/stdout_color_sinks.h>
   #include <spdlog/pattern_formatter.h>
   
   #include <filesystem>
   #include <iostream>
   #include <map>
   #include <ranges>
   #include <utility>
   
   #include "portal/platform/core/hal/platform_logger.h"
   
   namespace portal
   {
   Log::Settings g_settings;
   
   // Format: [date] [#thread_id] [file:line function] [name] colored{[level] message} extra
   constexpr auto default_pattern = "[%Y-%m-%d %H:%M:%S.%f] [%t] [%*] [%-12n] %^[%=7l] %v%$ %&";
   
   class source_location_flag_formatter final : public spdlog::custom_flag_formatter
   {
   private:
       size_t width_;
   
   public:
       explicit source_location_flag_formatter(const size_t width = 30) : width_(width) {}
   
       void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
       {
           if (msg.source.filename && msg.source.line > 0)
           {
               // Extract filename from a full path manually
               const std::string full_path = msg.source.filename;
               std::string filename = full_path;
   
               // Find last occurrence of path separator
               const auto last_slash = full_path.find_last_of("/\\");
               if (last_slash != std::string::npos)
               {
                   filename = full_path.substr(last_slash + 1);
               }
   
               std::string location = std::format("{}:{}", filename, msg.source.line);
   
               // Pad to specified width, left-aligned
               if (location.length() < width_)
               {
                   location += std::string(width_ - location.length(), ' ');
               }
               else if (location.length() > width_)
               {
                   location = location.substr(0, width_);
               }
   
               dest.append(location.data(), location.data() + location.size());
           }
           else
           {
               // Fallback: pad with spaces
               const std::string empty_location(width_, ' ');
               dest.append(empty_location.data(), empty_location.data() + empty_location.size());
           }
       }
   
       [[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override
       {
           return spdlog::details::make_unique<source_location_flag_formatter>(width_);
       }
   };
   
   void Log::init()
   {
       const Settings settings;
       init(settings);
   }
   
   void Log::init(const Settings& settings)
   {
       g_settings = settings;
   
       const std::string log_directory = "logs";
       if (!std::filesystem::exists(log_directory))
           std::filesystem::create_directory(log_directory);
   
       // Create a custom formatter and register the custom flag
       const auto formatter = std::make_unique<spdlog::pattern_formatter>();
       formatter->add_flag<source_location_flag_formatter>('*', 30);
       formatter->set_pattern(default_pattern);
   
       auto& sinks = platform::get_platform_sinks();
   
       for (const auto& sink : sinks)
       {
           sink->set_formatter(std::unique_ptr(formatter->clone()));
       }
   
       auto& loggers = get_loggers();
       for (const auto& logger : loggers | std::views::values)
       {
           logger->sinks() = sinks;
           logger->set_level(static_cast<spdlog::level::level_enum>(settings.default_log_level));
       }
   
       const auto default_logger = std::make_shared<spdlog::logger>(settings.default_logger_name, begin(sinks), end(sinks));
       default_logger->set_level(static_cast<spdlog::level::level_enum>(settings.default_log_level));
   
       spdlog::set_default_logger(default_logger);
       loggers["default"] = default_logger;
   
       LOG_INFO("Logger initialized");
   }
   
   void Log::shutdown()
   {
       LOG_INFO("Shutting down logger");
       auto& loggers = get_loggers();
       for (auto& logger : loggers | std::views::values)
       {
           logger.reset();
       }
       spdlog::drop_all();
   }
   
   std::shared_ptr<spdlog::logger> Log::get_logger(const std::string& tag_name)
   {
       auto& loggers = get_loggers();
       if (loggers.contains(tag_name))
           return loggers[tag_name];
   
       auto& sinks = platform::get_platform_sinks();
   
       // Create a new logger if it doesn't exist
       const auto logger = std::make_shared<spdlog::logger>(tag_name, begin(sinks), end(sinks));
       logger->set_level(static_cast<spdlog::level::level_enum>(g_settings.default_log_level));
       loggers[tag_name] = logger;
       return loggers[tag_name];
   }
   
   void Log::set_default_log_level(LogLevel level, const bool apply_to_all)
   {
       g_settings.default_log_level = level;
   
       // Set the default logger level
       spdlog::default_logger_raw()->set_level(static_cast<spdlog::level::level_enum>(level));
       // Apply to all existing loggers if requested
       if (apply_to_all)
       {
           auto& loggers = get_loggers();
           for (const auto& logger : loggers | std::views::values)
           {
               logger->set_level(static_cast<spdlog::level::level_enum>(level));
           }
       }
   }
   
   bool Log::has_tag(const std::string& tag_name)
   {
       auto& loggers = get_loggers();
       return loggers.contains(tag_name);
   }
   
   void Log::set_tag_level(const std::string& tag_name, LogLevel level)
   {
       const auto& logger = get_logger(tag_name);
       logger->set_level(static_cast<spdlog::level::level_enum>(level));
   }
   
   void Log::enable_tag(const std::string& tag_name, const bool enable)
   {
       // TODO: Save old state?
       const auto& logger = get_logger(tag_name);
       if (enable)
       {
           logger->set_level(static_cast<spdlog::level::level_enum>(g_settings.default_log_level));
       }
       else
       {
           logger->set_level(spdlog::level::off);
       }
   }
   
   void Log::print_message_tag(
       const spdlog::source_loc& loc,
       LogLevel level,
       const std::string_view tag,
       const std::string_view message
   )
   {
       if (const auto& logger = get_logger(tag.data()))
       {
           logger->log(loc, static_cast<spdlog::level::level_enum>(level), message);
       }
   }
   
   void Log::print_message(
       const std::shared_ptr<spdlog::logger>& logger,
       const spdlog::source_loc& loc,
       LogLevel level,
       const std::string_view message
   )
   {
       logger->log(loc, static_cast<spdlog::level::level_enum>(level), message);
   }
   
   bool Log::print_assert_message(
       const std::string_view file,
       const int line,
       const std::string_view function,
       const std::string_view message
   )
   {
       print_message_tag(
           spdlog::source_loc{file.data(), line, function.data()},
           LogLevel::Error,
           "assertion",
           std::format("assert ({}) failed", message)
       );
   
       return platform::print_assert_dialog(file, line, function, message);
   }
   
   std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>& Log::get_loggers()
   {
       static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> logger_map;
       return logger_map;
   }
   } // namespace portal
