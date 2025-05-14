#include "VulkanApp.h"

#include <iostream>
#include <stdexcept>

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
    std::cerr << "Ошибка: " << e.what() << std::endl;
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
      std::cerr << "Ошибка при инициализации VulkanCore" << std::endl;
      return false;
    }

    // Инициализация компонента управления устройством
    m_device = std::make_unique<VulkanDevice>(m_core->getInstance(), m_core->getSurface());
    if (m_device->init() != 0)
    {
      std::cerr << "Ошибка при инициализации VulkanDevice" << std::endl;
      return false;
    }

    // Инициализация компонента управления swap chain
    m_swapChain =
        std::make_unique<VulkanSwapChain>(*m_device, m_core->getSurface(), m_core->getWindow());
    if (m_swapChain->init() != 0)
    {
      std::cerr << "Ошибка при инициализации VulkanSwapChain" << std::endl;
      return false;
    }

    // Инициализация компонента рендеринга
    m_renderer = std::make_unique<VulkanRenderer>(*m_device, *m_swapChain);
    if (m_renderer->init() != 0)
    {
      std::cerr << "Ошибка при инициализации VulkanRenderer" << std::endl;
      return false;
    }

    std::cout << "Все компоненты инициализированы успешно!" << std::endl;
    return true;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Ошибка при инициализации компонентов: " << e.what() << std::endl;
    return false;
  }
}

void VulkanApp::mainLoop()
{
  std::cout << "Запуск основного цикла..." << std::endl;

  // Цикл обработки событий и рендеринга
  int frameCount = 0;
  while (m_core->processEvents())
  {
    std::cout << "Отрисовка кадра " << frameCount++ << std::endl;

    // Отрисовка кадра
    if (!m_renderer->drawFrame())
    {
      std::cout << "Ошибка при отрисовке кадра, завершение..." << std::endl;
      break;
    }

    // Добавим задержку для отладки
    SDL_Delay(100);

    // Ограничим число кадров для тестирования
    if (frameCount > 300)
    {
      std::cout << "Достигнуто максимальное число кадров, завершение..." << std::endl;
      break;
    }
  }

  // Ожидание завершения всех операций перед выходом
  m_device->getDevice().waitIdle();

  std::cout << "Основной цикл завершен" << std::endl;
}