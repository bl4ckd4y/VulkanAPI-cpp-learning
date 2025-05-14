#include "VulkanDevice.h"

#include <iostream>
#include <set>
#include <stdexcept>

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

    std::cout << "VulkanDevice инициализирован успешно!" << std::endl;
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Ошибка при инициализации VulkanDevice: " << e.what() << std::endl;
    return -1;
  }
}

void VulkanDevice::cleanup()
{
  // Логическое устройство уничтожается автоматически через RAII (vk::UniqueDevice)
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
      std::cout << "Выбрано устройство: " << deviceProperties.deviceName << std::endl;
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
    std::cout << "Логическое устройство создано успешно" << std::endl;
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
    VkBool32 presentSupport = false;
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