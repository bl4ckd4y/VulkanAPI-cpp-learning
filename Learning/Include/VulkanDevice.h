/**
 * @file VulkanDevice.h
 * @brief Класс для управления физическим и логическим устройствами Vulkan.
 *
 * УЧЕБНЫЙ КОММЕНТАРИЙ:
 * VulkanDevice реализует важнейший этап в Vulkan - выбор и настройку устройства (GPU).
 * В Vulkan есть два уровня устройств:
 * 1. Физические устройства (VkPhysicalDevice) - представляют реальные GPU в системе
 * 2. Логические устройства (VkDevice) - интерфейсы для работы с выбранным GPU
 *
 * Класс выполняет следующие ключевые задачи:
 * - Поиск подходящего физического устройства (дискретная видеокарта предпочтительнее)
 * - Поиск семейств очередей (графическая очередь и очередь презентации)
 * - Создание логического устройства с необходимыми расширениями
 * - Получение интерфейсов для очередей команд
 *
 * Принцип RAII применяется при создании логического устройства (vk::UniqueDevice).
 */

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

// УЧЕБНЫЙ КОММЕНТАРИЙ: Очереди в Vulkan
// Команды в Vulkan отправляются в специализированные очереди.
// Разные типы очередей поддерживают разные операции:
// - Графические очереди: для рендеринга
// - Вычислительные очереди: для вычислений на GPU
// - Очереди передачи: для копирования данных
// - Очереди презентации: для отображения на экран
//
// Устройство может поддерживать несколько "семейств очередей" с разной функциональностью.
// Для нашего приложения требуются минимум:
// - Графическое семейство (поддержка команд рендеринга)
// - Семейство презентации (поддержка отображения на экран)
//
// std::optional используется, так как при инициализации у нас еще нет действительных значений

// Структура для хранения индексов семейств очередей
struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

/**
 * @brief Класс для управления физическим и логическим устройствами Vulkan.
 * Отвечает за выбор GPU, создание логического устройства и получение очередей.
 */
class VulkanDevice
{
public:
  /**
   * @brief Конструктор
   * @param instance Экземпляр Vulkan
   * @param surface Поверхность для презентации
   */
  VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface);
  ~VulkanDevice();

  /**
   * @brief Инициализация устройства
   * @return Статус инициализации (0 - успешно)
   */
  int init();

  /**
   * @brief Очистка ресурсов устройства
   */
  void cleanup();

  /**
   * @brief Выводит информацию о расширениях устройства
   */
  void printDeviceExtensionsInfo();

  // Геттеры
  vk::PhysicalDevice getPhysicalDevice() const { return m_vkPhysicalDevice; }
  vk::Device         getDevice() const { return *m_vkDevice; }
  vk::Queue          getGraphicsQueue() const { return m_vkGraphicsQueue; }
  vk::Queue          getPresentQueue() const { return m_vkPresentQueue; }
  QueueFamilyIndices getQueueFamilyIndices() const { return m_queueFamilyIndices; }

  // Поддержка устройств
  bool               checkDeviceExtensionSupport(vk::PhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);

private:
  // Экземпляр Vulkan и поверхность (не владеет ими)
  vk::Instance   m_vkInstance;
  vk::SurfaceKHR m_vkSurface;

  // Физическое устройство
  vk::PhysicalDevice m_vkPhysicalDevice;

  // Логическое устройство (RAII)
  vk::UniqueDevice m_vkDevice;

  // Очереди и семейства очередей
  QueueFamilyIndices m_queueFamilyIndices;
  vk::Queue          m_vkGraphicsQueue;
  vk::Queue          m_vkPresentQueue;

  // Вспомогательные методы
  void pickPhysicalDevice();                         // Выбор физического устройства
  void createLogicalDevice();                        // Создание логического устройства
  bool isDeviceSuitable(vk::PhysicalDevice device);  // Проверка пригодности устройства

  // Константы
  const std::vector<const char*> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  // Валидационные слои
  const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

  // Включение/выключение валидационных слоев
#ifdef NDEBUG
  static constexpr bool m_enableValidationLayers = false;
#else
  static constexpr bool m_enableValidationLayers = true;
#endif
};