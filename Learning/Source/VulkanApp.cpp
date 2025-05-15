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

// УЧЕБНЫЙ КОММЕНТАРИЙ: Запуск приложения Vulkan
// Метод run() - это точка входа в приложение, которая координирует инициализацию
// всех компонентов и запуск основного цикла. Процесс запуска включает:
// 1. Инициализацию всех компонентов Vulkan (core, device, swapchain, renderer)
// 2. Запуск основного цикла приложения (mainLoop)
// 3. Автоматическую очистку ресурсов через RAII при завершении метода
//
// Vulkan требует точного соблюдения порядка инициализации: сначала экземпляр и поверхность,
// затем устройство, затем swap chain и наконец renderer.
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

// УЧЕБНЫЙ КОММЕНТАРИЙ: Основной цикл приложения Vulkan
// В отличие от OpenGL, где основной цикл обычно содержит много логики приложения,
// в Vulkan цикл часто включает только:
// 1. Обработку событий окна (SDL или GLFW)
// 2. Запуск рендеринга через drawFrame
//
// Интересно отметить, что очистка ресурсов при завершении работы требует ожидания
// завершения всех операций на GPU (device.waitIdle). Это необходимо, так как в Vulkan
// операции могут выполняться асинхронно, и без явного ожидания мы можем начать
// удалять ресурсы, которые все еще используются GPU.
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