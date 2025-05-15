/**
 * @file VulkanCore.h
 * @brief Базовый класс для инициализации Vulkan API и создания экземпляра.
 *
 * УЧЕБНЫЙ КОММЕНТАРИЙ:
 * VulkanCore - фундаментальный компонент Vulkan приложения, отвечающий за:
 * 1. Инициализацию окна (через SDL2)
 * 2. Создание экземпляра Vulkan (VkInstance) - точки входа в API
 * 3. Настройку валидационных слоев для отладки
 * 4. Создание поверхности (VkSurfaceKHR) для связи Vulkan с оконной системой
 *
 * Класс является основой архитектуры приложения и применяет принцип RAII
 * (Resource Acquisition Is Initialization) через использование Unique-объектов
 * из C++ Vulkan API (vk::UniqueInstance, vk::UniqueSurfaceKHR).
 */

#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VulkanLogger.h"

/**
 * @brief Базовый класс для работы с Vulkan API.
 * Отвечает за инициализацию Vulkan, создание экземпляра и поверхности.
 */
class VulkanCore
{
public:
  VulkanCore();
  ~VulkanCore();

  /**
   * @brief Инициализирует окно SDL и компоненты Vulkan
   * @return Статус инициализации (0 - успешно)
   */
  int init();

  /**
   * @brief Очищает ресурсы Vulkan и SDL
   */
  void cleanup();

  /**
   * @brief Обрабатывает события окна
   * @return true если приложение должно продолжать работу, false - для завершения
   */
  bool processEvents();

  /**
   * @brief Получить указатель на экземпляр Vulkan
   */
  vk::Instance getInstance() const { return *m_vkInstance; }

  /**
   * @brief Получить указатель на поверхность Vulkan
   */
  vk::SurfaceKHR getSurface() const { return *m_vkSurface; }

  /**
   * @brief Получить указатель на окно SDL
   */
  SDL_Window* getWindow() const { return m_pWindow; }

private:
  // SDL окно
  SDL_Window* m_pWindow = nullptr;

  // Компоненты Vulkan
  // УЧЕБНЫЙ КОММЕНТАРИЙ: RAII в Vulkan
  // Уникальные (Unique) объекты автоматически управляют временем жизни ресурсов.
  // vk::UniqueInstance -> при выходе из области видимости вызывается vkDestroyInstance
  // vk::UniqueSurfaceKHR -> при выходе из области видимости вызывается vkDestroySurfaceKHR
  vk::UniqueInstance   m_vkInstance;  // Экземпляр Vulkan (RAII)
  vk::UniqueSurfaceKHR m_vkSurface;   // Поверхность Vulkan (RAII)

  // Методы инициализации
  void initWindow();                   // Инициализация окна SDL
  void cleanupWindow();                // Очистка окна SDL
  void createInstance();               // Создание экземпляра Vulkan
  void initLogger();                   // Инициализация логгера
  bool checkValidationLayerSupport();  // Проверка поддержки валидационных слоев

  // Константы
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Валидационные слои
  // Валидационные слои - это компоненты Vulkan SDK, которые проверяют корректность
  // вызовов API, находят утечки памяти и другие ошибки. Они должны использоваться
  // при разработке, но отключаться в релизной версии для повышения производительности.
  const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

  // Включение/выключение валидационных слоев
#ifdef NDEBUG
  static constexpr bool m_enableValidationLayers = false;
#else
  static constexpr bool m_enableValidationLayers = true;
#endif
};