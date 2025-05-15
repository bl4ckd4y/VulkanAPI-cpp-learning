#include "VulkanApp.h"

#include <iostream>
#include <stdexcept>

#include "VulkanLogger.h"

VulkanApp::VulkanApp()
{
  // Инициализация компонентов будет выполнена в методе run()
}

VulkanApp::~VulkanApp()
{
  // Компоненты будут очищены автоматически в деструкторах unique_ptr
}

int VulkanApp::run()
{
  try
  {
    // Инициализация компонентов
    if (!initComponents())
    {
      return -1;
    }

    // Запуск основного цикла
    mainLoop();

    return 0;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка: " + std::string(e.what()));
    return -1;
  }
}

bool VulkanApp::initComponents()
{
  try
  {
    // Инициализация базового компонента Vulkan
    m_core = std::make_unique<VulkanCore>();
    if (m_core->init() != 0)
    {
      VulkanLogger::error("Ошибка при инициализации VulkanCore");
      return false;
    }

    // Инициализация компонента управления устройством
    m_device = std::make_unique<VulkanDevice>(m_core->getInstance(), m_core->getSurface());
    if (m_device->init() != 0)
    {
      VulkanLogger::error("Ошибка при инициализации VulkanDevice");
      return false;
    }

    // Инициализация компонента управления swap chain
    m_swapChain =
        std::make_unique<VulkanSwapChain>(*m_device, m_core->getSurface(), m_core->getWindow());
    if (m_swapChain->init() != 0)
    {
      VulkanLogger::error("Ошибка при инициализации VulkanSwapChain");
      return false;
    }

    // Инициализация компонента рендеринга
    m_renderer = std::make_unique<VulkanRenderer>(*m_device, *m_swapChain);
    if (m_renderer->init() != 0)
    {
      VulkanLogger::error("Ошибка при инициализации VulkanRenderer");
      return false;
    }

    VulkanLogger::info("Все компоненты инициализированы успешно!");
    return true;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при инициализации компонентов: " + std::string(e.what()));
    return false;
  }
}

void VulkanApp::mainLoop()
{
  VulkanLogger::info("Запуск основного цикла...");

  // Цикл обработки событий и рендеринга
  int frameCount = 0;
  while (m_core->processEvents())
  {
    VulkanLogger::debug("Отрисовка кадра " + std::to_string(frameCount++));

    // Отрисовка кадра
    if (!m_renderer->drawFrame())
    {
      VulkanLogger::error("Ошибка при отрисовке кадра, завершение...");
      break;
    }

    // Добавим небольшую задержку для ограничения FPS (16ms ~= 60fps)
    SDL_Delay(16);

    // Ограничим число кадров для тестирования
    if (frameCount > 300)
    {
      VulkanLogger::info("Достигнуто максимальное число кадров, завершение...");
      break;
    }
  }

  // Ожидание завершения всех операций перед выходом
  m_device->getDevice().waitIdle();

  VulkanLogger::info("Основной цикл завершен");
}