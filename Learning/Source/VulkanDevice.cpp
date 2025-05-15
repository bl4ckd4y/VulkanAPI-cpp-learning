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
  // Получение списка поддерживаемых физических устройств
  auto physicalDevices = m_vkInstance.enumeratePhysicalDevices();

  if (physicalDevices.empty())
  {
    throw std::runtime_error("Не найдено физических устройств с поддержкой Vulkan");
  }

  // Выбор первого подходящего устройства
  for (const auto& device : physicalDevices)
  {
    if (isDeviceSuitable(device))
    {
      m_vkPhysicalDevice = device;

      // Вывод информации о выбранном устройстве
      vk::PhysicalDeviceProperties deviceProperties = m_vkPhysicalDevice.getProperties();
      // Преобразуем deviceName в std::string используя string constructor со старт и длиной
      std::string deviceName(deviceProperties.deviceName.data());
      VulkanLogger::info("Выбрано устройство: " + deviceName);
      break;
    }
  }

  if (m_vkPhysicalDevice == vk::PhysicalDevice(nullptr))
  {
    throw std::runtime_error("Не удалось найти подходящее GPU");
  }
}

// Создание логического устройства
void VulkanDevice::createLogicalDevice()
{
  // Получение индексов семейств очередей
  m_queueFamilyIndices = findQueueFamilies(m_vkPhysicalDevice);

  // Создание информации о создаваемых очередях
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {m_queueFamilyIndices.graphicsFamily.value(),
                                            m_queueFamilyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    vk::DeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.queueFamilyIndex          = queueFamily;
    queueCreateInfo.queueCount                = 1;
    queueCreateInfo.pQueuePriorities          = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // Указание используемых функций устройства
  vk::PhysicalDeviceFeatures deviceFeatures = {};

  // Создание логического устройства
  vk::DeviceCreateInfo createInfo    = {};
  createInfo.pQueueCreateInfos       = queueCreateInfos.data();
  createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pEnabledFeatures        = &deviceFeatures;
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(m_deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

  // Создание логического устройства
  try
  {
    m_vkDevice = m_vkPhysicalDevice.createDeviceUnique(createInfo);
    VulkanLogger::info("Логическое устройство создано успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать логическое устройство: " + std::string(e.what()));
  }

  // Получение очередей
  m_vkGraphicsQueue = m_vkDevice->getQueue(m_queueFamilyIndices.graphicsFamily.value(), 0);
  m_vkPresentQueue  = m_vkDevice->getQueue(m_queueFamilyIndices.presentFamily.value(), 0);
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