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
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Swap Chain в Vulkan
  // Swap chain (цепочка обмена) - это набор буферов (изображений), используемых для отображения на
  // экран. В отличие от OpenGL, в Vulkan работа с экраном не является частью ядра API и требует
  // расширения.
  //
  // Основные компоненты Swap Chain:
  // 1. Поверхность - связь с окном системы
  // 2. Изображения - буферы для рендеринга
  // 3. Формат изображений - цветовой формат и цветовое пространство
  // 4. Режим презентации - как изображения отображаются на экран
  // 5. Размеры изображений - разрешение отображаемых изображений

  VulkanLogger::info("Создание swap chain...");

  // Запрашиваем параметры поддержки swap chain для текущего устройства
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_device.getPhysicalDevice());

  // Выбираем оптимальные параметры swap chain
  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR   presentMode   = chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D         extent        = chooseSwapExtent(swapChainSupport.capabilities);

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Количество изображений в Swap Chain
  // Минимальное число изображений в swap chain зависит от режима презентации и драйвера.
  // Рекомендуется запрашивать на одно изображение больше минимального для эффективной работы.
  // Это позволяет избежать ожидания драйвера при рендеринге.

  // Запрашиваем количество изображений +1 от минимума
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  // Проверяем, не превышаем ли максимально допустимое количество изображений
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VulkanLogger::info("Запрошено изображений в swap chain: " + std::to_string(imageCount));

  // Информация о создании swap chain
  vk::SwapchainCreateInfoKHR createInfo = {};
  createInfo.surface                    = m_vkSurface;
  createInfo.minImageCount              = imageCount;
  createInfo.imageFormat                = surfaceFormat.format;
  createInfo.imageColorSpace            = surfaceFormat.colorSpace;
  createInfo.imageExtent                = extent;
  createInfo.imageArrayLayers =
      1;  // 1 для обычного рендеринга, >1 для стереоскопического рендеринга

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Использование изображений Swap Chain
  // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: изображения используются как цель рендеринга (render
  // target) VK_IMAGE_USAGE_TRANSFER_DST_BIT: изображения могут быть целью операций копирования
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  // Получаем индексы семейств очередей
  QueueFamilyIndices indices    = m_device.getQueueFamilyIndices();
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Режимы совместного использования изображений Swap Chain
  // Если графическая очередь и очередь презентации различаются, нужно выбрать режим доступа:
  // - VK_SHARING_MODE_EXCLUSIVE: Изображение принадлежит одному семейству очередей и требует
  //   явной передачи владения при использовании в другом семействе (более производительно)
  // - VK_SHARING_MODE_CONCURRENT: Изображение может использоваться несколькими семействами
  //   очередей без явной передачи владения (проще, но менее эффективно)

  // Настройка режима совместного использования
  if (indices.graphicsFamily != indices.presentFamily)
  {
    // Разные семейства очередей - используем concurrent mode
    VulkanLogger::info("Используется CONCURRENT режим доступа к изображениям swap chain");
    createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else
  {
    // Одно семейство очередей - используем exclusive mode
    VulkanLogger::info("Используется EXCLUSIVE режим доступа к изображениям swap chain");
    createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;
  }

  // Можно указать трансформацию для изображений (поворот, отражение и т.д.)
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

  // Blend operation с другими окнами в оконной системе
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

  // Устанавливаем выбранный режим презентации
  createInfo.presentMode = presentMode;

  // Игнорировать цвет пикселей, закрытых другими окнами
  createInfo.clipped = VK_TRUE;

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Обработка изменения размера окна
  // При изменении размера окна может потребоваться пересоздание swap chain.
  // В этом случае указывается ссылка на предыдущий swap chain для более эффективного пересоздания.
  // В данной реализации это пока не поддерживается (oldSwapChain = null).
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  // Создаем swap chain
  try
  {
    m_vkSwapChain = m_device.getDevice().createSwapchainKHRUnique(createInfo);
    VulkanLogger::info("Swap chain создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать swap chain: " + std::string(e.what()));
  }

  // Получаем созданные изображения
  try
  {
    m_vkSwapChainImages = m_device.getDevice().getSwapchainImagesKHR(*m_vkSwapChain);
    VulkanLogger::info("Получено изображений swap chain: " +
                       std::to_string(m_vkSwapChainImages.size()));
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось получить изображения swap chain: " +
                             std::string(e.what()));
  }

  // Сохраняем формат и размеры изображений
  m_vkSwapChainImageFormat = surfaceFormat.format;
  m_vkSwapChainExtent      = extent;

  VulkanLogger::info("Формат изображений: " +
                     std::to_string(static_cast<uint32_t>(m_vkSwapChainImageFormat)));
  VulkanLogger::info("Размеры изображений: " + std::to_string(m_vkSwapChainExtent.width) + "x" +
                     std::to_string(m_vkSwapChainExtent.height));
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
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Выбор формата поверхности для Swap Chain
  // Формат поверхности (vk::SurfaceFormatKHR) состоит из:
  // 1. format - цветовой формат (например, VK_FORMAT_B8G8R8A8_SRGB - 8 бит на канал в BGRA порядке)
  // 2. colorSpace - цветовое пространство (например, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
  //
  // Предпочтительный формат для большинства приложений:
  // - BGRA с 8 битами на канал (32 бита на пиксель)
  // - sRGB цветовое пространство для правильной цветопередачи

  VulkanLogger::info("Выбор формата поверхности для swap chain...");
  VulkanLogger::info("Доступные форматы: " + std::to_string(availableFormats.size()));

  // Если устройство не имеет предпочтений, выбираем наш предпочтительный формат
  if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
  {
    VulkanLogger::info("Устройство не имеет предпочтений по формату, выбираем BGRA8 SRGB");
    return {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
  }

  // Ищем предпочтительный формат среди доступных
  for (const auto& availableFormat : availableFormats)
  {
    // Предпочитаем BGRA8 в sRGB пространстве
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
    {
      VulkanLogger::info("Выбран формат BGRA8 SRGB");
      return availableFormat;
    }
  }

  // Если предпочтительный формат не найден, берём первый доступный
  VulkanLogger::info("Предпочтительный формат не найден, используем первый доступный: " +
                     std::to_string(static_cast<uint32_t>(availableFormats[0].format)));
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