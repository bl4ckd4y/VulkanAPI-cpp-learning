/**
 * @file VulkanRenderer.h
 * @brief Класс для управления рендерингом с использованием Vulkan.
 *
 * УЧЕБНЫЙ КОММЕНТАРИЙ:
 * VulkanRenderer - это центральный компонент графического конвейера, отвечающий за
 * отрисовку графики на экран. В Vulkan процесс рендеринга сложнее, чем в OpenGL,
 * но предоставляет больше контроля и лучшую производительность.
 *
 * Основные компоненты рендеринга в Vulkan:
 * 1. Render Pass - описание буферов и операций с ними во время рендеринга
 * 2. Графический конвейер - полная настройка процесса рендеринга (шейдеры, состояния и т.д.)
 * 3. Framebuffers - конкретные буферы, к которым привязываются render pass
 * 4. Командные буферы - буферы, в которые записываются команды рендеринга
 * 5. Синхронизация - семафоры и заборы для синхронизации GPU и CPU
 *
 * Отличительной особенностью Vulkan является то, что графический конвейер
 * фиксируется при создании, а не изменяется динамически, как в OpenGL.
 */

#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanUtils.h"

/**
 * УЧЕБНЫЙ КОММЕНТАРИЙ: Работа с вершинами в Vulkan
 * В отличие от OpenGL, Vulkan не имеет предопределенных атрибутов вершин.
 * Вместо этого нам нужно явно указать:
 * 1. Структуру вершины (position, color, texcoord, normal и т.д.)
 * 2. Описание привязки (binding description) - как читать вершины из буфера
 * 3. Описание атрибутов (attribute description) - как интерпретировать данные
 *
 * Важно также отметить, что в Vulkan необходимо явно указывать смещения (offset)
 * полей структуры с помощью offsetof, чтобы они правильно соответствовали
 * расположению данных в памяти.
 */
struct Vertex
{
  float position[2];  // x, y координаты
  float color[3];     // r, g, b компоненты

  static vk::VertexInputBindingDescription getBindingDescription()
  {
    vk::VertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding                           = 0;
    bindingDescription.stride                            = sizeof(Vertex);
    bindingDescription.inputRate                         = vk::VertexInputRate::eVertex;
    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
  {
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};

    // Положение
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset   = offsetof(Vertex, position);

    // Цвет
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset   = offsetof(Vertex, color);

    return attributeDescriptions;
  }
};

/**
 * @brief Класс для управления рендерингом с использованием Vulkan.
 * Отвечает за создание и управление графическим конвейером, фреймбуферами и командными буферами.
 */
class VulkanRenderer
{
public:
  /**
   * @brief Конструктор
   * @param device Ссылка на объект VulkanDevice
   * @param swapChain Ссылка на объект VulkanSwapChain
   */
  VulkanRenderer(VulkanDevice& device, VulkanSwapChain& swapChain);
  ~VulkanRenderer();

  /**
   * @brief Инициализация renderer и связанных ресурсов
   * @return Статус инициализации (0 - успешно)
   */
  int init();

  /**
   * @brief Очистка ресурсов
   */
  void cleanup();

  /**
   * @brief Отрисовка одного кадра
   * @return true, если рендеринг выполнен успешно
   */
  bool drawFrame();

private:
  // Ссылки на зависимые объекты (не владеет ими)
  VulkanDevice&    m_device;
  VulkanSwapChain& m_swapChain;

  // Render pass и графический конвейер
  vk::UniqueRenderPass     m_vkRenderPass;        // Render pass (RAII)
  vk::UniquePipelineLayout m_vkPipelineLayout;    // Layout графического конвейера (RAII)
  vk::UniquePipeline       m_vkGraphicsPipeline;  // Графический конвейер (RAII)

  // Framebuffers
  std::vector<vk::UniqueFramebuffer> m_vkSwapChainFramebuffers;  // Framebuffers (RAII)

  // Командные буферы
  vk::UniqueCommandPool                m_vkCommandPool;     // Пул командных буферов (RAII)
  std::vector<vk::UniqueCommandBuffer> m_vkCommandBuffers;  // Командные буферы (RAII)

  // Синхронизация
  std::vector<vk::UniqueSemaphore>
      m_vkImageAvailableSemaphores;  // Семафоры для доступных изображений
  std::vector<vk::UniqueSemaphore>
                               m_vkRenderFinishedSemaphores;  // Семафоры для завершения рендеринга
  std::vector<vk::UniqueFence> m_vkInFlightFences;            // Заборы для кадров в полёте

  // Семафоры для каждого изображения swapchain
  std::vector<vk::UniqueSemaphore>
      m_vkImageInFlightSemaphores;  // Семафоры для изображений в обработке

  // Вершинный буфер
  vk::UniqueBuffer       m_vkVertexBuffer;        // Буфер вершин (RAII)
  vk::UniqueDeviceMemory m_vkVertexBufferMemory;  // Память буфера вершин (RAII)

  // Параметры рендеринга
  const int MAX_FRAMES_IN_FLIGHT = 2;     // Максимальное количество кадров в обработке
  size_t    m_currentFrame       = 0;     // Текущий индекс кадра
  float     m_animationTime      = 0.0f;  // Время для анимации

  // Данные о вершинах (для простоты - встроенные в класс)
  std::vector<Vertex> m_vertices = {
      {{-0.8f, 0.8f}, {1.0f, 0.0f, 0.0f}},  // Верхний левый угол (красный)
      {{0.8f, 0.8f}, {0.0f, 1.0f, 0.0f}},   // Верхний правый угол (зеленый)
      {{0.0f, -0.8f}, {0.0f, 0.0f, 1.0f}}   // Нижняя вершина (синий)
  };

  // Методы инициализации
  void createRenderPass();        // Создание render pass
  void createGraphicsPipeline();  // Создание графического конвейера
  void createFramebuffers();      // Создание framebuffers
  void createCommandPool();       // Создание пула командных буферов
  void createCommandBuffers();    // Создание командных буферов
  void createSyncObjects();       // Создание объектов синхронизации
  void createVertexBuffer();      // Создание буфера вершин

  // Вспомогательные методы
  vk::UniqueShaderModule createShaderModule(
      const std::vector<char>& code);  // Создание шейдерного модуля
  uint32_t findMemoryType(uint32_t                typeFilter,
                          vk::MemoryPropertyFlags properties);  // Поиск типа памяти
  void     recordCommandBuffer(vk::CommandBuffer commandBuffer,
                               uint32_t          imageIndex);  // Запись команд в буфер
};