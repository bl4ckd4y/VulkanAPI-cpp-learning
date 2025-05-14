#include "VulkanCore.h"

#include <iostream>
#include <stdexcept>

// Конструктор
VulkanCore::VulkanCore() {}

// Деструктор
VulkanCore::~VulkanCore()
{
  // Очистка ресурсов
  cleanup();
}

// Инициализация
int VulkanCore::init()
{
  try
  {
    // Инициализация окна
    initWindow();
    // Создание экземпляра Vulkan
    createInstance();

    // Создание поверхности (surface)
    VkSurfaceKHR vkSurface_hSurface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(m_pWindow, static_cast<VkInstance>(getInstance()),
                                  &vkSurface_hSurface))
    {
      throw std::runtime_error("Не удалось создать Vulkan surface через SDL2");
    }
    m_vkSurface = vk::UniqueSurfaceKHR(vkSurface_hSurface, getInstance());

    std::cout << "VulkanCore инициализирован успешно!" << std::endl;
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Ошибка при инициализации VulkanCore: " << e.what() << std::endl;
    cleanup();
    return -1;
  }
}

// Очистка ресурсов
void VulkanCore::cleanup()
{
  // Surface и Instance уничтожаются автоматически через RAII (vk::Unique*)
  cleanupWindow();
}

// Обработка событий
bool VulkanCore::processEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_QUIT)
    {
      return false;
    }
  }
  return true;
}

// Инициализация окна SDL
void VulkanCore::initWindow()
{
  std::cout << "Инициализация SDL..." << std::endl;

  // Инициализация SDL2
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::string error = SDL_GetError();
    std::cerr << "Ошибка SDL_Init: " << error << std::endl;
    throw std::runtime_error("Не удалось инициализировать SDL2: " + error);
  }

  std::cout << "SDL инициализирован успешно" << std::endl;
  std::cout << "Создание окна SDL..." << std::endl;

  // Создание окна
  m_pWindow = SDL_CreateWindow("Vulkan Application",                 // Заголовок
                               SDL_WINDOWPOS_CENTERED,               // Позиция X
                               SDL_WINDOWPOS_CENTERED,               // Позиция Y
                               800, 600,                             // Размеры
                               SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN  // Флаги
  );

  if (!m_pWindow)
  {
    std::string error = SDL_GetError();
    std::cerr << "Ошибка SDL_CreateWindow: " << error << std::endl;
    SDL_Quit();
    throw std::runtime_error("Не удалось создать окно SDL2: " + error);
  }

  std::cout << "Окно SDL создано успешно" << std::endl;
}

// Очистка окна SDL
void VulkanCore::cleanupWindow()
{
  if (m_pWindow)
  {
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
  }
  SDL_Quit();
}

// Создание экземпляра Vulkan
void VulkanCore::createInstance()
{
  // Информация о приложении
  vk::ApplicationInfo appInfo = {};
  appInfo.pApplicationName    = "Vulkan Application";
  appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName         = "No Engine";
  appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion          = VK_API_VERSION_1_2;

  // Получение расширений для SDL2
  unsigned int extCount = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extCount, nullptr))
  {
    throw std::runtime_error("Не удалось получить количество расширений Vulkan через SDL2");
  }

  std::vector<const char*> extensions(extCount);
  if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extCount, extensions.data()))
  {
    throw std::runtime_error("Не удалось получить расширения Vulkan через SDL2");
  }

  // Создание экземпляра Vulkan
  vk::InstanceCreateInfo createInfo  = {};
  createInfo.pApplicationInfo        = &appInfo;
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  try
  {
    m_vkInstance = vk::createInstanceUnique(createInfo);
    std::cout << "Экземпляр Vulkan создан успешно" << std::endl;
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать экземпляр Vulkan: " + std::string(e.what()));
  }
}