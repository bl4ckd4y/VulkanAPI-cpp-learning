#pragma once

#define SDL_MAIN_HANDLED
#include <memory>
#include <vulkan/vulkan.hpp>

#include "VulkanCore.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanSwapChain.h"

/**
 * @brief Главный класс приложения, управляющий всеми компонентами Vulkan.
 */
class VulkanApp
{
public:
  VulkanApp();
  ~VulkanApp();

  /**
   * @brief Запуск приложения
   * @return Код завершения (0 - успех)
   */
  int run();

private:
  // Компоненты приложения
  std::unique_ptr<VulkanCore>      m_core;       // Базовый компонент Vulkan
  std::unique_ptr<VulkanDevice>    m_device;     // Компонент управления устройством
  std::unique_ptr<VulkanSwapChain> m_swapChain;  // Компонент управления swap chain
  std::unique_ptr<VulkanRenderer>  m_renderer;   // Компонент рендеринга

  // Основной цикл приложения
  void mainLoop();

  // Инициализация компонентов
  bool initComponents();
};