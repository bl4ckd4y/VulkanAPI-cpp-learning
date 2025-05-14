#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

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
  vk::UniqueInstance   m_vkInstance;  // Экземпляр Vulkan (RAII)
  vk::UniqueSurfaceKHR m_vkSurface;   // Поверхность Vulkan (RAII)

  // Методы инициализации
  void initWindow();      // Инициализация окна SDL
  void cleanupWindow();   // Очистка окна SDL
  void createInstance();  // Создание экземпляра Vulkan
};