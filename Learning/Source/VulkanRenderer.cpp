#include "VulkanRenderer.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "VulkanLogger.h"

VulkanRenderer::VulkanRenderer(VulkanDevice& device, VulkanSwapChain& swapChain)
    : m_device(device), m_swapChain(swapChain)
{
}

VulkanRenderer::~VulkanRenderer()
{
  // Очистка ресурсов
  cleanup();
}

int VulkanRenderer::init()
{
  try
  {
    // Последовательная инициализация компонентов рендеринга
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();  // Добавляем создание буфера вершин
    createCommandBuffers();
    createSyncObjects();

    VulkanLogger::info("VulkanRenderer инициализирован успешно!");
    return 0;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при инициализации VulkanRenderer: " + std::string(e.what()));
    return -1;
  }
}

void VulkanRenderer::cleanup()
{
  // Ожидаем завершения всех операций
  m_device.getDevice().waitIdle();

  // Объекты освобождаются автоматически через RAII (vk::Unique*)
}

void VulkanRenderer::createRenderPass()
{
  // Описание цветового вложения
  vk::AttachmentDescription colorAttachment = {};
  colorAttachment.format                    = m_swapChain.getImageFormat();
  colorAttachment.samples                   = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp                    = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp                   = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp             = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp            = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout             = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout               = vk::ImageLayout::ePresentSrcKHR;

  // Описание подключения вложения
  vk::AttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment              = 0;
  colorAttachmentRef.layout                  = vk::ImageLayout::eColorAttachmentOptimal;

  // Описание подпрохода
  vk::SubpassDescription subpass = {};
  subpass.pipelineBindPoint      = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount   = 1;
  subpass.pColorAttachments      = &colorAttachmentRef;

  // Описание зависимости подпрохода
  vk::SubpassDependency dependency = {};
  dependency.srcSubpass            = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass            = 0;
  dependency.srcStageMask          = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.srcAccessMask         = vk::AccessFlags();
  dependency.dstStageMask          = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.dstAccessMask         = vk::AccessFlagBits::eColorAttachmentWrite;

  // Создание render pass
  vk::RenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.attachmentCount          = 1;
  renderPassInfo.pAttachments             = &colorAttachment;
  renderPassInfo.subpassCount             = 1;
  renderPassInfo.pSubpasses               = &subpass;
  renderPassInfo.dependencyCount          = 1;
  renderPassInfo.pDependencies            = &dependency;

  try
  {
    m_vkRenderPass = m_device.getDevice().createRenderPassUnique(renderPassInfo);
    VulkanLogger::info("Render pass создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать render pass: " + std::string(e.what()));
  }
}

void VulkanRenderer::createGraphicsPipeline()
{
  // Загрузка байт-кода шейдеров
  auto vertShaderCode = VulkanUtils::readFile("Learning/Shaders/triangle.vert.spv");
  auto fragShaderCode = VulkanUtils::readFile("Learning/Shaders/triangle.frag.spv");

  // Создание шейдерных модулей
  auto vertShaderModule = createShaderModule(vertShaderCode);
  auto fragShaderModule = createShaderModule(fragShaderCode);

  // Описание стадий шейдеров
  vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.stage                             = vk::ShaderStageFlagBits::eVertex;
  vertShaderStageInfo.module                            = *vertShaderModule;
  vertShaderStageInfo.pName                             = "main";  // Имя точки входа в шейдер

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.stage                             = vk::ShaderStageFlagBits::eFragment;
  fragShaderStageInfo.module                            = *fragShaderModule;
  fragShaderStageInfo.pName                             = "main";  // Имя точки входа в шейдер

  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // Информация о вершинных данных
  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.vertexBindingDescriptionCount          = 1;
  vertexInputInfo.pVertexBindingDescriptions             = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  // Описание сборки примитивов
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.topology                                 = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable                   = VK_FALSE;

  // Настройка вьюпорта и ножниц
  vk::Viewport viewport = {};
  viewport.x            = 0.0f;
  viewport.y            = 0.0f;
  viewport.width        = static_cast<float>(m_swapChain.getExtent().width);
  viewport.height       = static_cast<float>(m_swapChain.getExtent().height);
  viewport.minDepth     = 0.0f;
  viewport.maxDepth     = 1.0f;

  vk::Rect2D scissor = {};
  scissor.offset     = vk::Offset2D{0, 0};
  scissor.extent     = m_swapChain.getExtent();

  vk::PipelineViewportStateCreateInfo viewportState = {};
  viewportState.viewportCount                       = 1;
  viewportState.pViewports                          = &viewport;
  viewportState.scissorCount                        = 1;
  viewportState.pScissors                           = &scissor;

  // Настройка растеризатора
  vk::PipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.depthClampEnable                         = VK_FALSE;
  rasterizer.rasterizerDiscardEnable                  = VK_FALSE;
  rasterizer.polygonMode                              = vk::PolygonMode::eFill;
  rasterizer.lineWidth                                = 1.0f;
  rasterizer.cullMode        = vk::CullModeFlagBits::eNone;  // отключаем отсеивание граней
  rasterizer.frontFace       = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable = VK_FALSE;

  // Настройка мультисемплинга (отключен)
  vk::PipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sampleShadingEnable                    = VK_FALSE;
  multisampling.rasterizationSamples                   = vk::SampleCountFlagBits::e1;

  // Настройка смешивания цветов
  vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;

  vk::PipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.logicOpEnable                         = VK_FALSE;
  colorBlending.attachmentCount                       = 1;
  colorBlending.pAttachments                          = &colorBlendAttachment;

  // Создание layout'а пайплайна
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.setLayoutCount               = 0;
  pipelineLayoutInfo.pushConstantRangeCount       = 0;

  try
  {
    m_vkPipelineLayout = m_device.getDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать pipeline layout: " + std::string(e.what()));
  }

  // Создание графического пайплайна
  vk::GraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.stageCount                     = 2;
  pipelineInfo.pStages                        = shaderStages;
  pipelineInfo.pVertexInputState              = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState            = &inputAssembly;
  pipelineInfo.pViewportState                 = &viewportState;
  pipelineInfo.pRasterizationState            = &rasterizer;
  pipelineInfo.pMultisampleState              = &multisampling;
  pipelineInfo.pDepthStencilState             = nullptr;
  pipelineInfo.pColorBlendState               = &colorBlending;
  pipelineInfo.pDynamicState                  = nullptr;
  pipelineInfo.layout                         = *m_vkPipelineLayout;
  pipelineInfo.renderPass                     = *m_vkRenderPass;
  pipelineInfo.subpass                        = 0;
  pipelineInfo.basePipelineHandle             = nullptr;

  try
  {
    auto result          = m_device.getDevice().createGraphicsPipelineUnique(nullptr, pipelineInfo);
    m_vkGraphicsPipeline = std::move(result.value);
    VulkanLogger::info("Графический конвейер создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать графический конвейер: " + std::string(e.what()));
  }
}

void VulkanRenderer::createFramebuffers()
{
  // Изменение размера вектора для хранения framebuffers
  auto imageViews = m_swapChain.getImageViews();
  m_vkSwapChainFramebuffers.resize(imageViews.size());

  // Создание framebuffer для каждого image view
  for (size_t i = 0; i < imageViews.size(); i++)
  {
    // Массив из одного image view
    vk::ImageView attachments[] = {imageViews[i]};

    // Создание framebuffer
    vk::FramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.renderPass                = *m_vkRenderPass;
    framebufferInfo.attachmentCount           = 1;
    framebufferInfo.pAttachments              = attachments;
    framebufferInfo.width                     = m_swapChain.getExtent().width;
    framebufferInfo.height                    = m_swapChain.getExtent().height;
    framebufferInfo.layers                    = 1;

    try
    {
      m_vkSwapChainFramebuffers[i] = m_device.getDevice().createFramebufferUnique(framebufferInfo);
    }
    catch (const vk::SystemError& e)
    {
      throw std::runtime_error("Не удалось создать framebuffer: " + std::string(e.what()));
    }
  }

  VulkanLogger::info("Framebuffers созданы успешно");
}

void VulkanRenderer::createCommandPool()
{
  // Получение индекса семейства очередей для графических операций
  QueueFamilyIndices queueFamilyIndices = m_device.getQueueFamilyIndices();

  // Создание пула командных буферов
  vk::CommandPoolCreateInfo poolInfo = {};
  poolInfo.queueFamilyIndex          = queueFamilyIndices.graphicsFamily.value();
  poolInfo.flags                     = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

  try
  {
    m_vkCommandPool = m_device.getDevice().createCommandPoolUnique(poolInfo);
    VulkanLogger::info("Command pool создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать command pool: " + std::string(e.what()));
  }
}

void VulkanRenderer::createCommandBuffers()
{
  // Изменение размера вектора для хранения командных буферов
  m_vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  // Создание командных буферов
  vk::CommandBufferAllocateInfo allocInfo = {};
  allocInfo.commandPool                   = *m_vkCommandPool;
  allocInfo.level                         = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount            = static_cast<uint32_t>(m_vkCommandBuffers.size());

  try
  {
    m_vkCommandBuffers = m_device.getDevice().allocateCommandBuffersUnique(allocInfo);
    VulkanLogger::info("Command buffers созданы успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать command buffers: " + std::string(e.what()));
  }
}

void VulkanRenderer::createSyncObjects()
{
  // Изменение размера векторов для хранения объектов синхронизации
  m_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  // Создание семафоров для каждого изображения swapchain
  m_vkImageInFlightSemaphores.resize(m_swapChain.getImages().size());

  // Создание семафоров и заборов
  vk::SemaphoreCreateInfo semaphoreInfo = {};
  vk::FenceCreateInfo     fenceInfo     = {};
  fenceInfo.flags                       = vk::FenceCreateFlagBits::eSignaled;

  // Создание объектов синхронизации для каждого кадра
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    try
    {
      m_vkImageAvailableSemaphores[i] = m_device.getDevice().createSemaphoreUnique(semaphoreInfo);
      m_vkRenderFinishedSemaphores[i] = m_device.getDevice().createSemaphoreUnique(semaphoreInfo);
      m_vkInFlightFences[i]           = m_device.getDevice().createFenceUnique(fenceInfo);
    }
    catch (const vk::SystemError& e)
    {
      throw std::runtime_error("Не удалось создать объекты синхронизации: " +
                               std::string(e.what()));
    }
  }

  // Создание семафоров для каждого изображения swapchain
  for (size_t i = 0; i < m_swapChain.getImages().size(); i++)
  {
    try
    {
      m_vkImageInFlightSemaphores[i] = m_device.getDevice().createSemaphoreUnique(semaphoreInfo);
    }
    catch (const vk::SystemError& e)
    {
      throw std::runtime_error("Не удалось создать семафоры для изображений: " +
                               std::string(e.what()));
    }
  }

  VulkanLogger::info("Объекты синхронизации созданы успешно");
}

void VulkanRenderer::createVertexBuffer()
{
  // Размер данных вершин в байтах
  vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

  // Создание стадийного буфера (staging buffer)
  vk::UniqueBuffer       stagingBuffer;
  vk::UniqueDeviceMemory stagingBufferMemory;

  // Создание стадийного буфера
  vk::BufferCreateInfo bufferInfo = {};
  bufferInfo.size                 = bufferSize;
  bufferInfo.usage                = vk::BufferUsageFlagBits::eTransferSrc;
  bufferInfo.sharingMode          = vk::SharingMode::eExclusive;

  try
  {
    stagingBuffer = m_device.getDevice().createBufferUnique(bufferInfo);
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать staging buffer: " + std::string(e.what()));
  }

  // Выделение памяти для стадийного буфера
  vk::MemoryRequirements memRequirements =
      m_device.getDevice().getBufferMemoryRequirements(*stagingBuffer);

  vk::MemoryAllocateInfo allocInfo = {};
  allocInfo.allocationSize         = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible |
                                                         vk::MemoryPropertyFlagBits::eHostCoherent);

  try
  {
    stagingBufferMemory = m_device.getDevice().allocateMemoryUnique(allocInfo);
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось выделить память для staging buffer: " +
                             std::string(e.what()));
  }

  // Связывание буфера с памятью
  m_device.getDevice().bindBufferMemory(*stagingBuffer, *stagingBufferMemory, 0);

  // Копирование данных вершин в стадийный буфер
  void*                 data;
  [[maybe_unused]] auto mapResult =
      m_device.getDevice().mapMemory(*stagingBufferMemory, 0, bufferSize, {}, &data);
  memcpy(data, m_vertices.data(), (size_t)bufferSize);
  m_device.getDevice().unmapMemory(*stagingBufferMemory);

  // Создание буфера вершин
  bufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;

  try
  {
    m_vkVertexBuffer = m_device.getDevice().createBufferUnique(bufferInfo);
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать буфер вершин: " + std::string(e.what()));
  }

  // Выделение памяти для буфера вершин
  memRequirements = m_device.getDevice().getBufferMemoryRequirements(*m_vkVertexBuffer);

  // Создаем новый объект вместо переиспользования allocInfo
  vk::MemoryAllocateInfo vertexAllocInfo = {};
  vertexAllocInfo.allocationSize         = memRequirements.size;
  vertexAllocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

  try
  {
    m_vkVertexBufferMemory = m_device.getDevice().allocateMemoryUnique(vertexAllocInfo);
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось выделить память для буфера вершин: " +
                             std::string(e.what()));
  }

  // Связывание буфера с памятью
  m_device.getDevice().bindBufferMemory(*m_vkVertexBuffer, *m_vkVertexBufferMemory, 0);

  // Копирование данных из стадийного буфера в буфер вершин
  vk::CommandBufferAllocateInfo cmdBufAllocInfo = {};
  cmdBufAllocInfo.level                         = vk::CommandBufferLevel::ePrimary;
  cmdBufAllocInfo.commandPool                   = *m_vkCommandPool;
  cmdBufAllocInfo.commandBufferCount            = 1;

  auto tempCommandBuffers = m_device.getDevice().allocateCommandBuffersUnique(cmdBufAllocInfo);
  vk::UniqueCommandBuffer& tempCommandBuffer = tempCommandBuffers[0];

  // Начало записи команд
  vk::CommandBufferBeginInfo beginInfo = {};
  beginInfo.flags                      = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  tempCommandBuffer->begin(beginInfo);

  // Команда копирования
  vk::BufferCopy copyRegion = {};
  copyRegion.size           = bufferSize;
  tempCommandBuffer->copyBuffer(*stagingBuffer, *m_vkVertexBuffer, copyRegion);

  // Конец записи команд
  tempCommandBuffer->end();

  // Отправка команд и ожидание выполнения
  vk::SubmitInfo submitInfo          = {};
  submitInfo.commandBufferCount      = 1;
  vk::CommandBuffer commandBuffers[] = {*tempCommandBuffer};
  submitInfo.pCommandBuffers         = commandBuffers;

  m_device.getGraphicsQueue().submit(submitInfo, nullptr);  // возвращает void в Vulkan C++ API
  m_device.getGraphicsQueue().waitIdle();

  VulkanLogger::info("Буфер вершин создан успешно");
}

bool VulkanRenderer::drawFrame()
{
  try
  {
    // Ожидание завершения предыдущего кадра
    [[maybe_unused]] auto fenceResult = m_device.getDevice().waitForFences(
        *m_vkInFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

    // Получение индекса изображения из цепочки обмена
    uint32_t imageIndex;
    try
    {
      auto acquireResult = m_device.getDevice().acquireNextImageKHR(
          m_swapChain.getSwapChain(), std::numeric_limits<uint64_t>::max(),
          *m_vkImageAvailableSemaphores[m_currentFrame], nullptr);
      imageIndex = acquireResult.value;
    }
    catch (const vk::OutOfDateKHRError&)
    {
      // Обработка устаревшего swap chain
      return true;  // Пока просто возвращаем true
    }
    catch (const vk::SystemError& e)
    {
      throw std::runtime_error("Не удалось получить следующее изображение: " +
                               std::string(e.what()));
    }

    // Сброс забора для текущего кадра
    m_device.getDevice().resetFences(*m_vkInFlightFences[m_currentFrame]);

    // Сброс и запись команд для текущего буфера
    m_vkCommandBuffers[m_currentFrame]->reset();
    recordCommandBuffer(*m_vkCommandBuffers[m_currentFrame], imageIndex);

    // Настройка отправки команд в очередь
    vk::SubmitInfo submitInfo = {};

    // Семафоры ожидания (ждем, пока изображение станет доступным)
    vk::Semaphore          waitSemaphores[] = {*m_vkImageAvailableSemaphores[m_currentFrame]};
    vk::PipelineStageFlags waitStages[]     = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount           = 1;
    submitInfo.pWaitSemaphores              = waitSemaphores;
    submitInfo.pWaitDstStageMask            = waitStages;

    // Буфер команд для отправки
    vk::CommandBuffer commandBuffers[] = {*m_vkCommandBuffers[m_currentFrame]};
    submitInfo.commandBufferCount      = 1;
    submitInfo.pCommandBuffers         = commandBuffers;

    // Семафоры сигнала (сигнализируем о завершении рендеринга)
    // Используем семафор, связанный с конкретным изображением swapchain
    vk::Semaphore signalSemaphores[] = {*m_vkImageInFlightSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount  = 1;
    submitInfo.pSignalSemaphores     = signalSemaphores;

    // Отправка команд в очередь
    m_device.getGraphicsQueue().submit(submitInfo, *m_vkInFlightFences[m_currentFrame]);

    // Настройка отображения на экране
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    vk::SwapchainKHR swapChains[] = {m_swapChain.getSwapChain()};
    presentInfo.swapchainCount    = 1;
    presentInfo.pSwapchains       = swapChains;
    presentInfo.pImageIndices     = &imageIndex;

    // Отображение кадра на экране
    try
    {
      [[maybe_unused]] auto presentResult = m_device.getPresentQueue().presentKHR(presentInfo);
    }
    catch (const vk::OutOfDateKHRError&)
    {
      // Обработка устаревшего swap chain
      return true;  // Пока просто возвращаем true
    }
    catch (const vk::SystemError& e)
    {
      throw std::runtime_error("Не удалось отобразить кадр: " + std::string(e.what()));
    }

    // Обновление времени анимации
    m_animationTime += 0.01f;
    if (m_animationTime > 1.0f)
      m_animationTime = 0.0f;

    // Обновление цвета вершин
    m_vertices[0].color[0] = 0.5f + 0.5f * sin(m_animationTime * 6.28f);
    m_vertices[1].color[1] = 0.5f + 0.5f * sin(m_animationTime * 6.28f + 2.09f);
    m_vertices[2].color[2] = 0.5f + 0.5f * sin(m_animationTime * 6.28f + 4.19f);

    // Переход к следующему кадру
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::error("Ошибка при отрисовке кадра: " + std::string(e.what()));
    return false;
  }
}

vk::UniqueShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
  // Создание шейдерного модуля
  vk::ShaderModuleCreateInfo createInfo = {};
  createInfo.codeSize                   = code.size();
  createInfo.pCode                      = reinterpret_cast<const uint32_t*>(code.data());

  try
  {
    return m_device.getDevice().createShaderModuleUnique(createInfo);
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать shader module: " + std::string(e.what()));
  }
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
  // Получение информации о памяти физического устройства
  vk::PhysicalDeviceMemoryProperties memProperties =
      m_device.getPhysicalDevice().getMemoryProperties();

  // Поиск подходящего типа памяти
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("Не удалось найти подходящий тип памяти");
}

void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
  // Начало записи команд в буфер
  vk::CommandBufferBeginInfo beginInfo = {};
  beginInfo.flags                      = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  try
  {
    commandBuffer.begin(beginInfo);

    // Цвет фона (почти темный)
    vk::ClearValue clearColor =
        vk::ClearColorValue(std::array<float, 4>{0.01f, 0.01f, 0.01f, 1.0f});

    // Начало render pass
    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass              = *m_vkRenderPass;
    renderPassInfo.framebuffer             = *m_vkSwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset       = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent       = m_swapChain.getExtent();
    renderPassInfo.clearValueCount         = 1;
    renderPassInfo.pClearValues            = &clearColor;

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    // Привязка графического пайплайна
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_vkGraphicsPipeline);

    // Привязка буфера вершин
    vk::Buffer     vertexBuffers[] = {*m_vkVertexBuffer};
    vk::DeviceSize offsets[]       = {0};
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    // Отрисовка треугольника
    commandBuffer.draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);

    // Завершение render pass
    commandBuffer.endRenderPass();

    // Завершение записи команд
    commandBuffer.end();
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось записать командный буфер: " + std::string(e.what()));
  }
}