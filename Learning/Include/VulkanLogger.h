#pragma once

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vulkan/vulkan.hpp>

/**
 * @brief Уровни логирования
 */
enum class LogLevel
{
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
};

/**
 * @brief Класс для логирования работы Vulkan приложения
 * Реализует систему логирования в файл и настройку отладочных механизмов Vulkan
 */
class VulkanLogger
{
public:
  /**
   * @brief Инициализирует логгер
   * @param filename Файл для записи логов
   * @param consoleOutput Дублировать вывод в консоль
   * @return true если инициализация прошла успешно
   */
  static bool init(const std::string& filename, bool consoleOutput = true);

  /**
   * @brief Освобождает ресурсы логгера
   */
  static void cleanup();

  /**
   * @brief Логирует сообщение определенного уровня
   * @param level Уровень логирования
   * @param message Сообщение для логирования
   */
  static void log(LogLevel level, const std::string& message);

  /**
   * @brief Краткий вариант для логирования информации
   * @param message Сообщение для логирования
   */
  static void info(const std::string& message);

  /**
   * @brief Краткий вариант для логирования ошибки
   * @param message Сообщение для логирования
   */
  static void error(const std::string& message);

  /**
   * @brief Настраивает отладочный обратный вызов для Vulkan
   * @param instance Экземпляр Vulkan
   * @return true если настройка прошла успешно
   */
  static bool setupDebugMessenger(vk::Instance instance);

  /**
   * @brief Получить отладочные расширения для экземпляра Vulkan
   * @param extensions Вектор, куда будут добавлены расширения
   */
  static void getRequiredExtensions(std::vector<const char*>& extensions);

private:
  // Приватный конструктор (singleton)
  VulkanLogger() = default;

  // Поток для записи в файл
  static std::ofstream m_logFile;

  // Флаг вывода в консоль
  static bool m_consoleOutput;

  // Мьютекс для потокобезопасной записи
  static std::mutex m_mutex;

  // Отладочные объекты Vulkan
  static vk::DebugUtilsMessengerEXT m_debugMessenger;
  static vk::Instance               m_instance;

  // Обратный вызов для сообщений от Vulkan
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT             messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

  // Вспомогательные функции
  static std::string getTimestamp();
  static std::string levelToString(LogLevel level);
};