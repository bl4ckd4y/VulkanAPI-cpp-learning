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
    // УЧЕБНЫЙ КОММЕНТАРИЙ: Инициализация Vulkan API
    // Процесс инициализации Vulkan включает несколько этапов:
    // 1. Создание окна (SDL2) для отображения графики
    // 2. Создание экземпляра Vulkan (vkInstance) - точка входа в Vulkan API
    // 3. Настройка валидационных слоёв для отладки (если включены)
    // 4. Создание поверхности (surface) для связи Vulkan с оконной системой

    VulkanLogger::info("Инициализация VulkanCore...");

    // Инициализируем окно SDL
    initWindow();

    // Создаем экземпляр Vulkan
    createInstance();

    // Получаем поверхность для отображения
    // УЧЕБНЫЙ КОММЕНТАРИЙ: Поверхность (VkSurfaceKHR) - это абстракция,
    // которая связывает Vulkan с конкретной оконной системой (Windows, X11, Wayland и т.д.).
    // Для создания поверхности используем SDL_Vulkan_CreateSurface, который
    // скрывает различия между разными платформами.
    VkSurfaceKHR surfaceHandle;
    if (!SDL_Vulkan_CreateSurface(m_pWindow, static_cast<VkInstance>(*m_vkInstance),
                                  &surfaceHandle))
    {
      throw std::runtime_error("Не удалось создать поверхность Vulkan: " +
                               std::string(SDL_GetError()));
    }
    // УЧЕБНЫЙ КОММЕНТАРИЙ: RAII в Vulkan
    // Используем vk::UniqueSurfaceKHR для автоматического управления ресурсом.
    // Когда объект выходит из области видимости, деструктор автоматически вызывает
    // vkDestroySurfaceKHR
    m_vkSurface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surfaceHandle), *m_vkInstance);

    VulkanLogger::info("VulkanCore инициализирован успешно");
    return 0;  // Успешная инициализация
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
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Создание экземпляра Vulkan (VkInstance)
  // Экземпляр Vulkan - это точка входа в Vulkan API. При создании экземпляра мы:
  // 1. Предоставляем информацию о приложении (название, версия)
  // 2. Указываем необходимые расширения (в т.ч. для работы с окном)
  // 3. Настраиваем валидационные слои для отладки (если включены)
  // 4. Инициализируем Vulkan API с нужными параметрами

  // Проверяем поддержку валидационных слоев, если они включены
  if (m_enableValidationLayers && !checkValidationLayerSupport())
  {
    throw std::runtime_error("Запрошенные валидационные слои недоступны!");
  }

  // Информация о приложении для драйвера
  vk::ApplicationInfo appInfo = {};
  appInfo.pApplicationName    = "Vulkan Learning App";
  appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName         = "No Engine";
  appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion          = VK_API_VERSION_1_2;

  // Информация о создании экземпляра
  vk::InstanceCreateInfo createInfo = {};
  createInfo.pApplicationInfo       = &appInfo;

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Расширения Vulkan
  // Vulkan требует явного указания всех необходимых расширений.
  // Для работы с окном требуются специфичные расширения (surface, platform-specific и т.д.)
  // SDL2 упрощает работу с расширениями, предоставляя функцию SDL_Vulkan_GetInstanceExtensions

  // Получаем необходимые расширения от SDL
  uint32_t extensionCount = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extensionCount, nullptr))
  {
    throw std::runtime_error("Не удалось получить количество расширений Vulkan: " +
                             std::string(SDL_GetError()));
  }

  std::vector<const char*> extensions(extensionCount);
  if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extensionCount, extensions.data()))
  {
    throw std::runtime_error("Не удалось получить расширения Vulkan: " +
                             std::string(SDL_GetError()));
  }

  VulkanLogger::info("Требуемые расширения Vulkan:");
  for (const auto& extension : extensions)
  {
    VulkanLogger::info("  - " + std::string(extension));
  }

  // Если включены валидационные слои, добавляем расширение для отладки
  if (m_enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  // Устанавливаем расширения
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Валидационные слои Vulkan
  // Валидационные слои - это компоненты Vulkan, которые проверяют корректность вызовов API,
  // отлавливают ошибки и предоставляют диагностику. Их нужно использовать при разработке,
  // но отключать в релизной версии для повышения производительности.

  // Устанавливаем валидационные слои, если они включены
  if (m_enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
    createInfo.ppEnabledLayerNames = m_validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  // Создаем экземпляр Vulkan
  // УЧЕБНЫЙ КОММЕНТАРИЙ: RAII в Vulkan C++ API (vulkan.hpp)
  // Используем vk::createInstanceUnique() вместо vkCreateInstance для автоматического
  // управления временем жизни объекта. Когда m_vkInstance выходит из области видимости,
  // уничтожение VkInstance происходит автоматически.
  m_vkInstance = vk::createInstanceUnique(createInfo);

  // Список всех доступных расширений
  auto availableExtensions = vk::enumerateInstanceExtensionProperties();
  VulkanLogger::info("Доступные расширения Vulkan:");
  for (const auto& extension : availableExtensions)
  {
    VulkanLogger::info("  - " + std::string(extension.extensionName.data()) + ": " +
                       std::to_string(extension.specVersion));
  }
}