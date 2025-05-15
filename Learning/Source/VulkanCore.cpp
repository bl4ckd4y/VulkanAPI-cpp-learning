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
    // Удаляем инициализацию логгера, так как это делается в main.cpp
    // initLogger();

    VulkanLogger::info("Начало инициализации VulkanCore");

    // Инициализация окна
    initWindow();

    // Создание экземпляра Vulkan
    createInstance();

    // Настройка отладочного мессенджера, если включены валидационные слои
    if (m_enableValidationLayers)
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
    return 0;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при инициализации VulkanCore: " + std::string(e.what()));
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

  // Удаляем закрытие логгера, так как это делается в main.cpp
  // VulkanLogger::cleanup();
}

// Инициализация логгера - больше не используется, но оставляем для обратной совместимости
void VulkanCore::initLogger()
{
  // Функция оставлена пустой, так как инициализация происходит в main.cpp
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

  // Инициализация SDL2
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::string error = SDL_GetError();
    VulkanLogger::error("Ошибка SDL_Init: " + error);
    throw std::runtime_error("Не удалось инициализировать SDL2: " + error);
  }

  VulkanLogger::info("SDL инициализирован успешно");

  VulkanLogger::info("Создание окна SDL...");

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
    SDL_Quit();
    throw std::runtime_error("Не удалось создать окно SDL2: " + error);
  }

  VulkanLogger::info("Окно SDL создано успешно");
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
  if (m_enableValidationLayers && !checkValidationLayerSupport())
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
  if (m_enableValidationLayers)
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
  if (m_enableValidationLayers)
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
  }
  catch (const vk::SystemError& e)
  {
    VulkanLogger::error("Не удалось создать экземпляр Vulkan: " + std::string(e.what()));
    throw std::runtime_error("Не удалось создать экземпляр Vulkan: " + std::string(e.what()));
  }
}