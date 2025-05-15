/**
 * @file VulkanSwapChain.h
 * @brief Класс для управления цепочкой обмена изображениями в Vulkan.
 *
 * УЧЕБНЫЙ КОММЕНТАРИЙ:
 * Swap Chain (цепочка обмена) - это набор буферов (изображений), которые используются
 * для отображения на экран. В отличие от OpenGL, Vulkan требует явного управления
 * этими буферами через специальный объект VkSwapchainKHR.
 *
 * Класс VulkanSwapChain отвечает за:
 * 1. Проверку поддержки Swap Chain устройством
 * 2. Выбор оптимального формата изображений (цветовой формат и цветовое пространство)
 * 3. Выбор режима презентации (FIFO, MAILBOX и др.)
 * 4. Выбор размеров изображений
 * 5. Создание Image Views для доступа к изображениям
 *
 * KHR в именах (например, VkSwapchainKHR) означает, что функциональность
 * доступна через расширение Khronos, а не через базовую спецификацию Vulkan.
 */

#pragma once

#include <SDL.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VulkanDevice.h"

// УЧЕБНЫЙ КОММЕНТАРИЙ: Параметры Swap Chain
// Перед созданием Swap Chain необходимо запросить его поддержку устройством.
// Эта структура хранит три ключевых компонента:
// 1. capabilities - возможности поверхности (мин/макс количество буферов, размеры и т.д.)
// 2. formats - поддерживаемые форматы изображений (цветовой формат и цветовое пространство)
// 3. presentModes - поддерживаемые режимы презентации (как изображения отображаются на экран)
//
// Режимы презентации:
// - VK_PRESENT_MODE_IMMEDIATE_KHR: мгновенное отображение (может вызывать разрывы)
// - VK_PRESENT_MODE_FIFO_KHR: очередь FIFO с вертикальной синхронизацией (V-Sync)
// - VK_PRESENT_MODE_FIFO_RELAXED_KHR: как FIFO, но если очередь пуста - мгновенное отображение
// - VK_PRESENT_MODE_MAILBOX_KHR: тройная буферизация (перезапись неотображенных буферов)

// Структура для хранения параметров цепочки обмена
struct SwapChainSupportDetails
{
  vk::SurfaceCapabilitiesKHR        capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR>   presentModes;
};

/**
 * @brief Класс для управления цепочкой обмена (swap chain) и связанными ресурсами.
 */
class VulkanSwapChain
{
public:
  /**
   * @brief Конструктор
   * @param device Ссылка на объект VulkanDevice
   * @param surface Поверхность для презентации
   * @param window Окно SDL для определения размеров
   */
  VulkanSwapChain(VulkanDevice& device, vk::SurfaceKHR surface, SDL_Window* window);
  ~VulkanSwapChain();

  /**
   * @brief Инициализация swap chain и связанных ресурсов
   * @return Статус инициализации (0 - успешно)
   */
  int init();

  /**
   * @brief Очистка ресурсов
   */
  void cleanup();

  // Геттеры
  vk::SwapchainKHR              getSwapChain() const { return *m_vkSwapChain; }
  vk::Format                    getImageFormat() const { return m_vkSwapChainImageFormat; }
  vk::Extent2D                  getExtent() const { return m_vkSwapChainExtent; }
  const std::vector<vk::Image>& getImages() const { return m_vkSwapChainImages; }
  std::vector<vk::ImageView>    getImageViews() const
  {
    std::vector<vk::ImageView> imageViews;
    imageViews.reserve(m_vkSwapChainImageViews.size());
    for (const auto& view : m_vkSwapChainImageViews)
    {
      imageViews.push_back(*view);
    }
    return imageViews;
  }

  /**
   * @brief Запрос поддержки swap chain для указанного физического устройства
   * @param device Физическое устройство для проверки
   * @return Структура с деталями поддержки swap chain
   */
  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

private:
  // Ссылки на внешние объекты (не владеет ими)
  VulkanDevice&  m_device;
  vk::SurfaceKHR m_vkSurface;
  SDL_Window*    m_pWindow;

  // Объекты swap chain и связанные ресурсы
  vk::UniqueSwapchainKHR           m_vkSwapChain;             // Swap chain (RAII)
  std::vector<vk::Image>           m_vkSwapChainImages;       // Изображения (не RAII)
  vk::Format                       m_vkSwapChainImageFormat;  // Формат изображений
  vk::Extent2D                     m_vkSwapChainExtent;       // Размеры изображений
  std::vector<vk::UniqueImageView> m_vkSwapChainImageViews;   // Image views (RAII)

  // Вспомогательные методы выбора параметров swap chain
  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR>& availableFormats);
  vk::PresentModeKHR chooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR>& availablePresentModes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

  // Создание ресурсов
  void createSwapChain();    // Создание swap chain
  void createImageViews();   // Создание image views для изображений из swap chain
  void cleanupImageViews();  // Очистка image views
};