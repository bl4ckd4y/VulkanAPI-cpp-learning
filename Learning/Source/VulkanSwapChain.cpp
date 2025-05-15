#include "VulkanSwapChain.h"

#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "VulkanLogger.h"

VulkanSwapChain::VulkanSwapChain(VulkanDevice& device, vk::SurfaceKHR surface, SDL_Window* window)
    : m_device(device), m_vkSurface(surface), m_pWindow(window)
{
}

VulkanSwapChain::~VulkanSwapChain()
{
  // Очистка ресурсов
  cleanup();
}

int VulkanSwapChain::init()
{
  try
  {
    // Создание swap chain
    createSwapChain();

    // Создание image views
    createImageViews();

    VulkanLogger::info("VulkanSwapChain инициализирован успешно!");
    return 0;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при инициализации VulkanSwapChain: " + std::string(e.what()));
    return -1;
  }
}

void VulkanSwapChain::cleanup()
{
  // Очистка ресурсов
  cleanupImageViews();
  // m_vkSwapChain очищается автоматически через RAII (vk::UniqueSwapchainKHR)
}

void VulkanSwapChain::cleanupImageViews()
{
  // Очистка image views происходит автоматически через RAII (vk::UniqueImageView)
  m_vkSwapChainImageViews.clear();
}

void VulkanSwapChain::createSwapChain()
{
  VulkanLogger::debug("Начало создания swap chain");

  // Запрос поддержки swap chain
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_device.getPhysicalDevice());

  // Выбор оптимальных параметров swap chain
  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR   presentMode   = chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D         extent        = chooseSwapExtent(swapChainSupport.capabilities);

  // Определение количества изображений в swap chain
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VulkanLogger::debug("Выбрано количество изображений в swap chain: " + std::to_string(imageCount));

  // Создание информации о swap chain
  vk::SwapchainCreateInfoKHR createInfo = {};
  createInfo.surface                    = m_vkSurface;
  createInfo.minImageCount              = imageCount;
  createInfo.imageFormat                = surfaceFormat.format;
  createInfo.imageColorSpace            = surfaceFormat.colorSpace;
  createInfo.imageExtent                = extent;
  createInfo.imageArrayLayers           = 1;
  createInfo.imageUsage                 = vk::ImageUsageFlagBits::eColorAttachment;

  // Указание режима использования изображений из разных семейств очередей
  QueueFamilyIndices indices    = m_device.getQueueFamilyIndices();
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily)
  {
    // Если индексы разные, используем режим совместного использования
    createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;

    VulkanLogger::debug("Используется режим совместного использования изображений между очередями");
  }
  else
  {
    // Если индексы одинаковые, используем эксклюзивный режим
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;

    VulkanLogger::debug("Используется эксклюзивный режим использования изображений");
  }

  // Дополнительные параметры
  createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode    = presentMode;
  createInfo.clipped        = VK_TRUE;
  createInfo.oldSwapchain   = nullptr;

  // Создание swap chain
  try
  {
    m_vkSwapChain = m_device.getDevice().createSwapchainKHRUnique(createInfo);
    VulkanLogger::info("Swap chain создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать swap chain: " + std::string(e.what()));
  }

  // Получение изображений из swap chain
  m_vkSwapChainImages      = m_device.getDevice().getSwapchainImagesKHR(*m_vkSwapChain);
  m_vkSwapChainImageFormat = surfaceFormat.format;
  m_vkSwapChainExtent      = extent;

  VulkanLogger::info("Количество изображений в swap chain: " +
                     std::to_string(m_vkSwapChainImages.size()));
}

void VulkanSwapChain::createImageViews()
{
  VulkanLogger::debug("Начало создания image views");

  // Очистка старых image views, если они есть
  cleanupImageViews();

  // Изменение размера вектора для хранения image views
  m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());

  // Создание image view для каждого изображения из swap chain
  for (size_t i = 0; i < m_vkSwapChainImages.size(); i++)
  {
    vk::ImageViewCreateInfo createInfo = {};
    createInfo.image                   = m_vkSwapChainImages[i];
    createInfo.viewType                = vk::ImageViewType::e2D;
    createInfo.format                  = m_vkSwapChainImageFormat;

    // Компоненты цвета
    createInfo.components.r = vk::ComponentSwizzle::eIdentity;
    createInfo.components.g = vk::ComponentSwizzle::eIdentity;
    createInfo.components.b = vk::ComponentSwizzle::eIdentity;
    createInfo.components.a = vk::ComponentSwizzle::eIdentity;

    // Диапазон и тип изображения
    createInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    try
    {
      m_vkSwapChainImageViews[i] = m_device.getDevice().createImageViewUnique(createInfo);
    }
    catch (const vk::SystemError& e)
    {
      throw std::runtime_error("Не удалось создать image view: " + std::string(e.what()));
    }
  }

  VulkanLogger::info("Image views созданы успешно");
}

SwapChainSupportDetails VulkanSwapChain::querySwapChainSupport(vk::PhysicalDevice device)
{
  SwapChainSupportDetails details;

  // Получение возможностей поверхности
  details.capabilities = device.getSurfaceCapabilitiesKHR(m_vkSurface);

  // Получение поддерживаемых форматов поверхности
  details.formats = device.getSurfaceFormatsKHR(m_vkSurface);

  // Получение поддерживаемых режимов презентации
  details.presentModes = device.getSurfacePresentModesKHR(m_vkSurface);

  return details;
}

vk::SurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
  // Поиск предпочтительного формата (SRGB и 8-битный на канал)
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
    {
      return availableFormat;
    }
  }

  // Если предпочтительного формата нет, выбираем первый доступный
  return availableFormats[0];
}

vk::PresentModeKHR VulkanSwapChain::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
  // Поиск предпочтительного режима презентации (Mailbox - тройная буферизация)
  for (const auto& availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox)
    {
      return availablePresentMode;
    }
  }

  // Если режима Mailbox нет, используем FIFO (всегда поддерживается)
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
  // Если текущее значение не является максимальным, Vulkan требует точное совпадение
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    // Получение фактических размеров окна
    int width, height;
    SDL_GetWindowSize(m_pWindow, &width, &height);

    vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    // Ограничение размеров согласно возможностям устройства
    actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                     capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
  }
}