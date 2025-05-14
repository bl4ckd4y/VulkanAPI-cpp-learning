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
    // Инициализация логгера
    initLogger();

    VulkanLogger::info("Начало инициализации VulkanCore");

    // Инициализация окна
    initWindow();

    // Создание экземпляра Vulkan
    createInstance();

    // Настройка отладочного мессенджера, если включены валидационные слои
    if (enableValidationLayers)
    {
      VulkanLogger::info("Настройка отладочного мессенджера Vulkan");
      if (!VulkanLogger::setupDebugMessenger(getInstance()))
      {
        VulkanLogger::error("Не удалось настроить отладочный мессенджер");
      }
    }

    // Создание поверхности (surface)
    VulkanLogger::info("Создание поверхности Vulkan");
    VkSurfaceKHR vkSurface_hSurface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(m_pWindow, static_cast<VkInstance>(getInstance()),
                                  &vkSurface_hSurface))
    {
      VulkanLogger::error("Не удалось создать Vulkan surface через SDL2");
      throw std::runtime_error("Не удалось создать Vulkan surface через SDL2");
    }
    m_vkSurface = vk::UniqueSurfaceKHR(vkSurface_hSurface, getInstance());
    VulkanLogger::info("Поверхность Vulkan создана успешно");

    VulkanLogger::info("VulkanCore инициализирован успешно!");
    std::cout << "VulkanCore инициализирован успешно!" << std::endl;
    return 0;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при инициализации VulkanCore: " + std::string(e.what()));
    std::cerr << "Ошибка при инициализации VulkanCore: " << e.what() << std::endl;
    cleanup();
    return -1;
  }
}

// Очистка ресурсов
void VulkanCore::cleanup()
{
  VulkanLogger::info("Очистка ресурсов VulkanCore");

  // Surface и Instance уничтожаются автоматически через RAII (vk::Unique*)
  cleanupWindow();

  // Очистка логгера должна быть последней
  VulkanLogger::cleanup();
}

// Инициализация логгера
void VulkanCore::initLogger()
{
  if (!VulkanLogger::init("vulkan_app.log"))
  {
    std::cerr << "Предупреждение: Не удалось инициализировать логгер" << std::endl;
  }
}

// Обработка событий
bool VulkanCore::processEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_QUIT)
    {
      VulkanLogger::info("Получено событие выхода из приложения");
      return false;
    }
  }
  return true;
}

// Инициализация окна SDL
void VulkanCore::initWindow()
{
  VulkanLogger::info("Инициализация SDL...");
  std::cout << "Инициализация SDL..." << std::endl;

  // Инициализация SDL2
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::string error = SDL_GetError();
    VulkanLogger::error("Ошибка SDL_Init: " + error);
    std::cerr << "Ошибка SDL_Init: " << error << std::endl;
    throw std::runtime_error("Не удалось инициализировать SDL2: " + error);
  }

  VulkanLogger::info("SDL инициализирован успешно");
  std::cout << "SDL инициализирован успешно" << std::endl;

  VulkanLogger::info("Создание окна SDL...");
  std::cout << "Создание окна SDL..." << std::endl;

  // Создание окна
  m_pWindow = SDL_CreateWindow("Vulkan Application",                 // Заголовок
                               SDL_WINDOWPOS_CENTERED,               // Позиция X
                               SDL_WINDOWPOS_CENTERED,               // Позиция Y
                               1366, 768,                            // Размеры
                               SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN  // Флаги
  );

  if (!m_pWindow)
  {
    std::string error = SDL_GetError();
    VulkanLogger::error("Ошибка SDL_CreateWindow: " + error);
    std::cerr << "Ошибка SDL_CreateWindow: " << error << std::endl;
    SDL_Quit();
    throw std::runtime_error("Не удалось создать окно SDL2: " + error);
  }

  VulkanLogger::info("Окно SDL создано успешно");
  std::cout << "Окно SDL создано успешно" << std::endl;
}

// Очистка окна SDL
void VulkanCore::cleanupWindow()
{
  if (m_pWindow)
  {
    VulkanLogger::info("Уничтожение окна SDL");
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
  }

  VulkanLogger::info("Завершение работы SDL");
  SDL_Quit();
}

// Проверка поддержки валидационных слоев
bool VulkanCore::checkValidationLayerSupport()
{
  VulkanLogger::info("Проверка поддержки валидационных слоев");

  // Получаем доступные слои
  auto availableLayers = vk::enumerateInstanceLayerProperties();

  // Проверяем наличие каждого требуемого слоя
  for (const char* layerName : m_validationLayers)
  {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound)
    {
      VulkanLogger::error("Валидационный слой не поддерживается: " + std::string(layerName));
      return false;
    }
  }

  VulkanLogger::info("Все запрошенные валидационные слои поддерживаются");
  return true;
}

// Создание экземпляра Vulkan
void VulkanCore::createInstance()
{
  VulkanLogger::info("Создание экземпляра Vulkan");

  // Проверка поддержки валидационных слоев
  if (enableValidationLayers && !checkValidationLayerSupport())
  {
    VulkanLogger::error("Запрошенные валидационные слои не поддерживаются!");
    throw std::runtime_error("Запрошенные валидационные слои не поддерживаются!");
  }

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
    VulkanLogger::error("Не удалось получить количество расширений Vulkan через SDL2");
    throw std::runtime_error("Не удалось получить количество расширений Vulkan через SDL2");
  }

  std::vector<const char*> extensions(extCount);
  if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extCount, extensions.data()))
  {
    VulkanLogger::error("Не удалось получить расширения Vulkan через SDL2");
    throw std::runtime_error("Не удалось получить расширения Vulkan через SDL2");
  }

  // Добавляем расширения для отладки, если включены валидационные слои
  if (enableValidationLayers)
  {
    VulkanLogger::info("Добавление отладочных расширений");
    VulkanLogger::getRequiredExtensions(extensions);
  }

  // Логирование всех используемых расширений
  std::stringstream extLog;
  extLog << "Используемые расширения экземпляра (" << extensions.size() << "):";
  for (const auto& ext : extensions)
  {
    extLog << " " << ext;
  }
  VulkanLogger::info(extLog.str());

  // Создание экземпляра Vulkan
  vk::InstanceCreateInfo createInfo  = {};
  createInfo.pApplicationInfo        = &appInfo;
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // Добавляем валидационные слои, если включены
  if (enableValidationLayers)
  {
    VulkanLogger::info("Включение валидационных слоев");
    createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
    createInfo.ppEnabledLayerNames = m_validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  try
  {
    m_vkInstance = vk::createInstanceUnique(createInfo);
    VulkanLogger::info("Экземпляр Vulkan создан успешно");
    std::cout << "Экземпляр Vulkan создан успешно" << std::endl;
  }
  catch (const vk::SystemError& e)
  {
    VulkanLogger::error("Не удалось создать экземпляр Vulkan: " + std::string(e.what()));
    throw std::runtime_error("Не удалось создать экземпляр Vulkan: " + std::string(e.what()));
  }
}