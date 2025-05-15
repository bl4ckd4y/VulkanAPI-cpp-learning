#include "VulkanDevice.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

#include "VulkanLogger.h"

VulkanDevice::VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface)
    : m_vkInstance(instance), m_vkSurface(surface), m_vkPhysicalDevice(nullptr)
{
}

VulkanDevice::~VulkanDevice()
{
  // Очистка ресурсов
  cleanup();
}

int VulkanDevice::init()
{
  try
  {
    // Выбор физического устройства
    pickPhysicalDevice();

    // Создание логического устройства
    createLogicalDevice();

    // Вывод информации о расширениях
    printDeviceExtensionsInfo();

    VulkanLogger::info("VulkanDevice инициализирован успешно!");
    return 0;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при инициализации VulkanDevice: " + std::string(e.what()));
    return -1;
  }
}

void VulkanDevice::cleanup()
{
  // Логическое устройство уничтожается автоматически через RAII (vk::UniqueDevice)
}

// Вывод информации о расширениях устройства
void VulkanDevice::printDeviceExtensionsInfo()
{
  try
  {
    // Получаем доступные расширения
    auto availableExtensions = m_vkPhysicalDevice.enumerateDeviceExtensionProperties();

    // Формируем содержимое лога
    std::stringstream logContent;
    logContent << "\n============ Информация о расширениях устройства ============\n";
    logContent << "Доступно расширений: " << availableExtensions.size() << std::endl;
    logContent << "Список всех расширений:" << std::endl;
    for (const auto& extension : availableExtensions)
    {
      logContent << "  - " << extension.extensionName << " (версия: " << extension.specVersion
                 << ")" << std::endl;
    }
    logContent << "Используемые расширения:" << std::endl;
    for (const auto& requiredExt : m_deviceExtensions)
    {
      bool found = false;
      for (const auto& ext : availableExtensions)
      {
        if (strcmp(requiredExt, ext.extensionName) == 0)
        {
          found = true;
          break;
        }
      }
      logContent << "  - " << requiredExt << ": "
                 << (found ? "поддерживается" : "НЕ поддерживается!") << std::endl;
    }
    logContent << "============================================================\n" << std::endl;

    // Записываем в файл через метод VulkanLogger
    if (!VulkanLogger::logToFile("device_extensions.log", logContent.str(), false))
    {
      // Если не удалось открыть файл, предупреждение уже будет выведено в logToFile
      VulkanLogger::warning(
          "Не удалось записать информацию о расширениях в файл device_extensions.log");
    }

    // --- В лог выводим только используемые расширения ---
    VulkanLogger::info("Используемые расширения устройства:");
    for (const auto& requiredExt : m_deviceExtensions)
    {
      bool found = false;
      for (const auto& ext : availableExtensions)
      {
        if (strcmp(requiredExt, ext.extensionName) == 0)
        {
          found = true;
          break;
        }
      }
      VulkanLogger::info("  - " + std::string(requiredExt) + ": " +
                         (found ? "поддерживается" : "НЕ поддерживается!"));
    }
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при получении информации о расширениях: " + std::string(e.what()));
  }
}

// Выбор физического устройства
void VulkanDevice::pickPhysicalDevice()
{
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Выбор физического устройства (GPU)
  // В Vulkan разделяют физические и логические устройства:
  // - Физическое устройство (VkPhysicalDevice): представляет конкретную видеокарту
  // - Логическое устройство (VkDevice): интерфейс для работы с выбранной видеокартой
  //
  // При выборе физического устройства для Vulkan необходимо:
  // 1. Получить список всех доступных GPU
  // 2. Проверить, что устройство поддерживает нужные возможности (графическую очередь, поверхность,
  // расширения)
  // 3. Выбрать наиболее подходящее устройство (дискретная видеокарта предпочтительнее
  // интегрированной)

  VulkanLogger::info("Выбор физического устройства...");

  // Получаем список физических устройств
  std::vector<vk::PhysicalDevice> devices = m_vkInstance.enumeratePhysicalDevices();

  if (devices.empty())
  {
    throw std::runtime_error("Не найдено устройств с поддержкой Vulkan!");
  }

  VulkanLogger::info("Найдено устройств с поддержкой Vulkan: " + std::to_string(devices.size()));

  // Выбираем первое подходящее устройство
  for (const auto& device : devices)
  {
    // Получаем информацию об устройстве для лучшего логирования
    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    std::string                  deviceName       = deviceProperties.deviceName;

    VulkanLogger::info("Проверка устройства: " + deviceName);

    // УЧЕБНЫЙ КОММЕНТАРИЙ: Типы устройств в Vulkan
    // vk::PhysicalDeviceType::eDiscreteGpu - дискретная видеокарта (предпочтительно)
    // vk::PhysicalDeviceType::eIntegratedGpu - интегрированная видеокарта
    // vk::PhysicalDeviceType::eVirtualGpu - виртуальная видеокарта
    // vk::PhysicalDeviceType::eCpu - программная эмуляция на CPU
    // vk::PhysicalDeviceType::eOther - другой тип устройства

    std::string deviceType;
    switch (deviceProperties.deviceType)
    {
      case vk::PhysicalDeviceType::eDiscreteGpu:
        deviceType = "Дискретная видеокарта";
        break;
      case vk::PhysicalDeviceType::eIntegratedGpu:
        deviceType = "Интегрированная видеокарта";
        break;
      case vk::PhysicalDeviceType::eVirtualGpu:
        deviceType = "Виртуальная видеокарта";
        break;
      case vk::PhysicalDeviceType::eCpu:
        deviceType = "Программная эмуляция (CPU)";
        break;
      default:
        deviceType = "Другой тип устройства";
        break;
    }

    VulkanLogger::info("Тип устройства: " + deviceType);

    // Проверяем, подходит ли устройство
    if (isDeviceSuitable(device))
    {
      VulkanLogger::info("Выбрано устройство: " + deviceName);
      m_vkPhysicalDevice = device;

      // Запоминаем индексы семейств очередей
      m_queueFamilyIndices = findQueueFamilies(device);

      // Вывод детальной информации об устройстве
      VulkanLogger::info("ID устройства: " + std::to_string(deviceProperties.deviceID));
      VulkanLogger::info("Версия драйвера: " + std::to_string(deviceProperties.driverVersion));
      VulkanLogger::info(
          "Версия Vulkan: " + std::to_string(VK_VERSION_MAJOR(deviceProperties.apiVersion)) + "." +
          std::to_string(VK_VERSION_MINOR(deviceProperties.apiVersion)) + "." +
          std::to_string(VK_VERSION_PATCH(deviceProperties.apiVersion)));

      return;
    }
  }

  throw std::runtime_error("Не найдено подходящее устройство с поддержкой Vulkan!");
}

// Создание логического устройства
void VulkanDevice::createLogicalDevice()
{
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Создание логического устройства Vulkan
  // Логическое устройство - это интерфейс для взаимодействия с физическим устройством (GPU).
  // При создании логического устройства необходимо:
  // 1. Указать, какие семейства очередей мы будем использовать
  // 2. Указать, какие расширения устройства нам нужны (например, swapchain)
  // 3. Указать, какие функции устройства нам требуются
  // 4. Получить интерфейсы для работы с очередями

  VulkanLogger::info("Создание логического устройства...");

  // Получаем индексы семейств очередей для графики и отображения
  QueueFamilyIndices indices = m_queueFamilyIndices;

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Очереди в Vulkan
  // Vulkan использует очереди для выполнения команд на GPU.
  // Разные типы очередей поддерживают разные операции:
  // - Графические очереди (VK_QUEUE_GRAPHICS_BIT): для рендеринга графики
  // - Вычислительные очереди (VK_QUEUE_COMPUTE_BIT): для вычислений на GPU
  // - Очереди передачи (VK_QUEUE_TRANSFER_BIT): для копирования данных
  // - Очереди презентации: для отображения на экран (требуют расширение KHR)
  //
  // Устройство может иметь несколько семейств очередей с разной функциональностью

  // Создаем структуры для настройки очередей
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t>                     uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                                                indices.presentFamily.value()};

  // Приоритет очереди (от 0.0 до 1.0)
  float queuePriority = 1.0f;

  // Создаем информацию для каждого уникального семейства очередей
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    vk::DeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.queueFamilyIndex          = queueFamily;
    queueCreateInfo.queueCount                = 1;  // Создаем только одну очередь в семействе
    queueCreateInfo.pQueuePriorities          = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // Функции устройства (пока не используем никаких дополнительных)
  vk::PhysicalDeviceFeatures deviceFeatures = {};

  // Информация о создании устройства
  vk::DeviceCreateInfo createInfo = {};
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos    = queueCreateInfos.data();
  createInfo.pEnabledFeatures     = &deviceFeatures;

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Расширения устройства
  // Расширения устройства добавляют дополнительную функциональность.
  // Самые важные расширения:
  // - VK_KHR_swapchain: для создания цепочки обмена (отображение на экран)
  // - VK_KHR_maintenance1, VK_KHR_maintenance2, и т.д.: улучшения базового API
  // - VK_KHR_dedicated_allocation: для эффективного управления памятью

  // Указываем необходимые расширения устройства
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(m_deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

  VulkanLogger::info("Запрошенные расширения устройства:");
  for (const auto& extension : m_deviceExtensions)
  {
    VulkanLogger::info("  - " + std::string(extension));
  }

  // В Vulkan 1.0 приходилось указывать слои на уровне устройства, но в новых версиях
  // это не требуется, однако для совместимости указываем их и для устройства
  if (m_enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
    createInfo.ppEnabledLayerNames = m_validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  // Создаем логическое устройство
  try
  {
    // УЧЕБНЫЙ КОММЕНТАРИЙ: RAII в Vulkan (продолжение)
    // vk::UniqueDevice - это RAII-обертка для VkDevice.
    // При уничтожении объекта автоматически вызывается vkDestroyDevice
    m_vkDevice = m_vkPhysicalDevice.createDeviceUnique(createInfo);
    VulkanLogger::info("Логическое устройство создано успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать логическое устройство: " + std::string(e.what()));
  }

  // Получаем очереди от созданного устройства
  m_vkGraphicsQueue = m_vkDevice->getQueue(indices.graphicsFamily.value(), 0);
  VulkanLogger::info("Получена графическая очередь");

  m_vkPresentQueue = m_vkDevice->getQueue(indices.presentFamily.value(), 0);
  VulkanLogger::info("Получена очередь презентации");
}

// Проверка пригодности устройства
bool VulkanDevice::isDeviceSuitable(vk::PhysicalDevice device)
{
  // Поиск необходимых семейств очередей
  QueueFamilyIndices indices = findQueueFamilies(device);

  // Проверка поддержки расширений для swap chain
  bool extensionsSupported = checkDeviceExtensionSupport(device);

  return indices.isComplete() && extensionsSupported;
}

// Проверка поддержки расширений устройством
bool VulkanDevice::checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
  // Получение поддерживаемых расширений
  auto availableExtensions = device.enumerateDeviceExtensionProperties();

  // Создание набора необходимых расширений
  std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

  // Проверка поддержки каждого расширения
  for (const auto& extension : availableExtensions)
  {
    requiredExtensions.erase(extension.extensionName);
  }

  // Если все расширения поддерживаются, набор должен быть пустым
  return requiredExtensions.empty();
}

// Поиск семейств очередей
QueueFamilyIndices VulkanDevice::findQueueFamilies(vk::PhysicalDevice device)
{
  QueueFamilyIndices indices;

  // Получение свойств всех семейств очередей
  auto queueFamilyProperties = device.getQueueFamilyProperties();

  // Поиск семейства очередей с поддержкой графических операций
  uint32_t i = 0;
  for (const auto& queueFamily : queueFamilyProperties)
  {
    // Проверка поддержки графических операций
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
    {
      indices.graphicsFamily = i;
    }

    // Проверка поддержки презентации
    VkBool32              presentSupport = false;
    [[maybe_unused]] auto supportResult =
        device.getSurfaceSupportKHR(i, m_vkSurface, &presentSupport);
    if (presentSupport)
    {
      indices.presentFamily = i;
    }

    // Если оба индекса найдены, выход из цикла
    if (indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}