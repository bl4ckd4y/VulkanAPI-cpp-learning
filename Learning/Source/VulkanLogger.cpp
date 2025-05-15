#include "VulkanLogger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

// Инициализация статических переменных
std::ofstream              VulkanLogger::m_logFile;
bool                       VulkanLogger::m_consoleOutput = true;
std::mutex                 VulkanLogger::m_mutex;
LogLevel                   VulkanLogger::m_logLevel = LogLevel::DEBUG;
vk::DebugUtilsMessengerEXT VulkanLogger::m_debugMessenger;
vk::Instance               VulkanLogger::m_instance;

// Инициализация логгера
bool VulkanLogger::init(const std::string& filename, bool consoleOutput, LogLevel level)
{
  // Блокировка для потокобезопасности
  std::lock_guard<std::mutex> lock(m_mutex);

  // Проверка, не инициализирован ли уже логгер
  if (m_logFile.is_open())
  {
    if (m_consoleOutput)
    {
      std::cout << "Логгер уже инициализирован. Файл: " << filename << std::endl;
    }
    return true;  // Возвращаем true, так как логгер уже инициализирован и работает
  }

  // Настройка флага вывода в консоль и уровня логирования
  m_consoleOutput = consoleOutput;
  m_logLevel      = level;

  // Открытие файла для записи логов
  m_logFile.open(filename, std::ios::out | std::ios::app);
  if (!m_logFile.is_open())
  {
    if (m_consoleOutput)
    {
      std::cerr << "Ошибка: не удалось открыть файл логов " << filename << std::endl;
    }
    return false;
  }

  // Запись заголовка в файл логов
  m_logFile << "\n==========================================================\n";
  m_logFile << "Начало сессии логирования: " << getTimestamp() << "\n";
  m_logFile << "==========================================================\n\n";
  m_logFile.flush();

  if (m_consoleOutput)
  {
    std::cout << "Логгер инициализирован. Файл: " << filename << std::endl;
  }

  return true;
}

// Очистка ресурсов логгера
void VulkanLogger::cleanup()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Уничтожение отладочного мессенджера, если он был создан
  if (m_debugMessenger && m_instance)
  {
    // Получаем функцию уничтожения отладочного мессенджера
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        static_cast<VkInstance>(m_instance), "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
      func(static_cast<VkInstance>(m_instance),
           static_cast<VkDebugUtilsMessengerEXT>(m_debugMessenger), nullptr);
      m_debugMessenger = nullptr;
    }
  }

  // Запись завершающего сообщения и закрытие файла только если файл открыт
  if (m_logFile.is_open())
  {
    m_logFile << "\n==========================================================\n";
    m_logFile << "Завершение сессии логирования: " << getTimestamp() << "\n";
    m_logFile << "==========================================================\n\n";
    m_logFile.close();

    if (m_consoleOutput)
    {
      std::cout << "Логгер завершил работу." << std::endl;
    }
  }
  else if (m_consoleOutput)
  {
    std::cout << "Логгер уже был закрыт или не был инициализирован." << std::endl;
  }
}

// Логирование сообщения
void VulkanLogger::log(LogLevel level, const std::string& message)
{
  // Проверка уровня логирования
  if (level < m_logLevel)
  {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  // Формирование строки лога
  std::stringstream logStream;
  logStream << getTimestamp() << " [" << levelToString(level) << "] " << message;
  std::string logMessage = logStream.str();

  // Запись в файл
  if (m_logFile.is_open())
  {
    m_logFile << logMessage << std::endl;
    m_logFile.flush();
  }

  // Вывод в консоль, если разрешено
  if (m_consoleOutput)
  {
    // Выбор потока вывода в зависимости от уровня
    if (level == LogLevel::ERROR || level == LogLevel::FATAL)
    {
      std::cerr << logMessage << std::endl;
    }
    else
    {
      std::cout << logMessage << std::endl;
    }
  }
}

// Логирование отладочной информации
void VulkanLogger::debug(const std::string& message)
{
  log(LogLevel::DEBUG, message);
}

// Логирование информационного сообщения
void VulkanLogger::info(const std::string& message)
{
  log(LogLevel::INFO, message);
}

// Логирование предупреждения
void VulkanLogger::warning(const std::string& message)
{
  log(LogLevel::WARNING, message);
}

// Логирование ошибки
void VulkanLogger::error(const std::string& message)
{
  log(LogLevel::ERROR, message);
}

// Логирование фатальной ошибки
void VulkanLogger::fatal(const std::string& message)
{
  log(LogLevel::FATAL, message);
}

// Установка уровня логирования
void VulkanLogger::setLogLevel(LogLevel level)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_logLevel = level;

  // Непосредственно записываем сообщение, чтобы избежать рекурсивного вызова info()
  std::stringstream logStream;
  logStream << getTimestamp() << " [" << levelToString(LogLevel::INFO) << "] "
            << "Установлен уровень логирования: " << levelToString(level);
  std::string logMessage = logStream.str();

  if (m_logFile.is_open())
  {
    m_logFile << logMessage << std::endl;
    m_logFile.flush();
  }

  if (m_consoleOutput)
  {
    std::cout << logMessage << std::endl;
  }
}

// Получение текущего уровня логирования
LogLevel VulkanLogger::getLogLevel()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_logLevel;
}

// Запись лога в отдельный файл
bool VulkanLogger::logToFile(const std::string& filename, const std::string& content, bool append)
{
  // Открываем файл для записи без использования мьютекса логгера
  std::ofstream           file;
  std::ios_base::openmode mode = std::ios::out;
  if (append)
  {
    mode |= std::ios::app;
  }

  file.open(filename, mode);
  if (!file.is_open())
  {
    // Используем прямой вывод в консоль вместо error(), чтобы избежать deadlock
    std::string errorMessage = "Не удалось открыть файл для записи: " + filename;
    std::cerr << getTimestamp() << " [ERROR] " << errorMessage << std::endl;

    // Теперь можем безопасно использовать error(), так как мьютекс не заблокирован
    error(errorMessage);
    return false;
  }

  // Записываем содержимое
  file << content;
  file.close();

  // Логируем успешную запись, только если уровень логирования позволяет
  if (m_logLevel <= LogLevel::DEBUG)
  {
    // Создаем сообщение с информацией о размере записанных данных
    std::string debugMessage = "Данные записаны в файл: " + filename +
                               " (размер: " + std::to_string(content.size()) + " байт)";

    // Вызываем метод debug для логирования
    debug(debugMessage);
  }

  return true;
}

// Настройка отладочного обратного вызова Vulkan
bool VulkanLogger::setupDebugMessenger(vk::Instance instance)
{
  // Сохранение экземпляра Vulkan
  m_instance = instance;

  // Используем C-API Vulkan напрямую
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData       = nullptr;

  // Получаем адрес функции vkCreateDebugUtilsMessengerEXT
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      static_cast<VkInstance>(instance), "vkCreateDebugUtilsMessengerEXT");

  if (func == nullptr)
  {
    error("Не удалось найти функцию vkCreateDebugUtilsMessengerEXT");
    return false;
  }

  // Создаем отладочный мессенджер
  VkDebugUtilsMessengerEXT debugMessenger;
  VkResult result = func(static_cast<VkInstance>(instance), &createInfo, nullptr, &debugMessenger);

  if (result != VK_SUCCESS)
  {
    error("Не удалось создать отладочный мессенджер");
    return false;
  }

  // Сохраняем созданный мессенджер
  m_debugMessenger = debugMessenger;

  info("Отладочный мессенджер Vulkan успешно настроен");
  return true;
}

// Получение необходимых расширений для отладки
void VulkanLogger::getRequiredExtensions(std::vector<const char*>& extensions)
{
  // Добавляем расширение для отладочных сообщений
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

// Обратный вызов для отладочных сообщений Vulkan
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanLogger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData [[maybe_unused]])
{
  // Определение уровня логирования на основе серьезности сообщения
  LogLevel level;
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    level = LogLevel::ERROR;
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    level = LogLevel::WARNING;
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    level = LogLevel::INFO;
  }
  else
  {
    level = LogLevel::DEBUG;
  }

  // Формирование сообщения с типом и ID
  std::stringstream message;
  message << "Vulkan: ";

  // Добавляем тип сообщения
  if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
  {
    message << "[Validation] ";
  }
  else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
  {
    message << "[Performance] ";
  }

  // Добавляем сообщение из обратного вызова
  message << pCallbackData->pMessage;

  // Логируем сообщение
  log(level, message.str());

  // Всегда возвращаем VK_FALSE, что означает, что сообщение не будет прервано
  return VK_FALSE;
}

// Получение метки времени
std::string VulkanLogger::getTimestamp()
{
  auto now  = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::stringstream ss;
  // Используем безопасную версию localtime
#ifdef _WIN32
  struct tm timeInfo;
  localtime_s(&timeInfo, &time);
  ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
#else
  struct tm timeInfo;
  localtime_r(&time, &timeInfo);
  ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
#endif
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
}

// Преобразование уровня логирования в строку
std::string VulkanLogger::levelToString(LogLevel level)
{
  switch (level)
  {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO ";
    case LogLevel::WARNING:
      return "WARN ";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}
