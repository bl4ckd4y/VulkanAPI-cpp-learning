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
  // УЧЕБНЫЙ КОММЕНТАРИЙ: Графический конвейер в Vulkan
  // Графический конвейер (Graphics Pipeline) определяет все этапы обработки графики,
  // от загрузки вершин до вывода пикселей. В отличие от OpenGL, в Vulkan вся конфигурация
  // конвейера задаётся заранее и фиксируется при создании, что увеличивает производительность.
  //
  // Основные этапы графического конвейера:
  // 1. Input Assembler - сбор вершинных данных
  // 2. Vertex Shader - обработка каждой вершины
  // 3. Tessellation - опционально, подразделение геометрии
  // 4. Geometry Shader - опционально, генерация новой геометрии
  // 5. Rasterization - преобразование примитивов в фрагменты
  // 6. Fragment Shader - обработка каждого фрагмента (пикселя)
  // 7. Color Blending - смешивание цветов с буфером кадра

  VulkanLogger::info("Создание графического конвейера...");

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Шейдеры в Vulkan
  // В отличие от OpenGL, шейдеры в Vulkan не компилируются во время выполнения.
  // Они предварительно компилируются в бинарный формат SPIR-V, который загружается в приложение.
  // SPIR-V - это промежуточное представление, которое делает компиляцию шейдеров более
  // предсказуемой.

  // Загрузка скомпилированных шейдеров
  std::vector<char> vertShaderCode = VulkanUtils::readFile("Learning/Shaders/triangle.vert.spv");
  std::vector<char> fragShaderCode = VulkanUtils::readFile("Learning/Shaders/triangle.frag.spv");

  VulkanLogger::info("Загружен вершинный шейдер, размер: " + std::to_string(vertShaderCode.size()) +
                     " байт");
  VulkanLogger::info(
      "Загружен фрагментный шейдер, размер: " + std::to_string(fragShaderCode.size()) + " байт");

  // Создаем VkShaderModule для каждого шейдера
  vk::UniqueShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  vk::UniqueShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Точки входа шейдеров
  // В шейдерах Vulkan указывается точка входа - функция, с которой начинается выполнение шейдера.
  // Обычно это функция "main", но может быть и другое имя.

  // Настройка для вершинного шейдера
  vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.stage  = vk::ShaderStageFlagBits::eVertex;  // Тип шейдера
  vertShaderStageInfo.module = *vertShaderModule;                 // Модуль шейдера
  vertShaderStageInfo.pName  = "main";                            // Точка входа

  // Настройка для фрагментного шейдера
  vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.stage  = vk::ShaderStageFlagBits::eFragment;  // Тип шейдера
  fragShaderStageInfo.module = *fragShaderModule;                   // Модуль шейдера
  fragShaderStageInfo.pName  = "main";                              // Точка входа

  // Этапы шейдеров в конвейере
  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Описание вершинных данных
  // В Vulkan необходимо явно указать, как интерпретировать данные вершин.
  // Это включает шаг вершин (stride), формат атрибутов, смещение и т.д.

  // Информация о вершинном вводе
  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.vertexBindingDescriptionCount          = 1;
  vertexInputInfo.pVertexBindingDescriptions             = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Сборка примитивов
  // Указываем, как вершины собираются в примитивы:
  // - Точки (POINT_LIST): каждая вершина - отдельная точка
  // - Линии (LINE_LIST): каждая пара вершин - отдельная линия
  // - Треугольники (TRIANGLE_LIST): каждые три вершины - отдельный треугольник
  // - Полоса треугольников (TRIANGLE_STRIP): более эффективный способ задания треугольников

  // Настройка входного сборщика
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;  // Рисуем треугольники
  inputAssembly.primitiveRestartEnable = VK_FALSE;  // Не используем перезапуск примитивов

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Область просмотра и ножницы
  // Viewport определяет преобразование из координат изображения в нормализованные координаты.
  // Scissors определяет область отсечения пикселей (только пиксели внутри области будут
  // обрабатываться).

  // Настройка вьюпорта и ножниц
  vk::Viewport viewport = {};
  viewport.x            = 0.0f;
  viewport.y            = 0.0f;
  viewport.width        = static_cast<float>(m_swapChain.getExtent().width);
  viewport.height       = static_cast<float>(m_swapChain.getExtent().height);
  viewport.minDepth     = 0.0f;
  viewport.maxDepth     = 1.0f;

  vk::Rect2D scissor = {};
  scissor.offset     = vk::Offset2D(0, 0);
  scissor.extent     = m_swapChain.getExtent();

  vk::PipelineViewportStateCreateInfo viewportState = {};
  viewportState.viewportCount                       = 1;
  viewportState.pViewports                          = &viewport;
  viewportState.scissorCount                        = 1;
  viewportState.pScissors                           = &scissor;

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Растеризация в Vulkan
  // Растеризация преобразует геометрию (треугольники) в фрагменты (пиксели).
  // Основные параметры растеризации включают:
  // - Режим заполнения (FILL, LINE, POINT)
  // - Отсечение задних граней (CULL_BACK, CULL_FRONT, CULL_NONE)
  // - Порядок вершин (CLOCKWISE, COUNTER_CLOCKWISE) для определения передней грани

  // Настройка растеризатора
  vk::PipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.depthClampEnable        = VK_FALSE;  // Не обрезать фрагменты за пределами глубины
  rasterizer.rasterizerDiscardEnable = VK_FALSE;  // Не отбрасывать геометрию
  rasterizer.polygonMode             = vk::PolygonMode::eFill;       // Режим заполнения: заполнение
  rasterizer.lineWidth               = 1.0f;                         // Толщина линии
  rasterizer.cullMode                = vk::CullModeFlagBits::eBack;  // Отсекать задние грани
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;  // Порядок вершин для передней грани
  rasterizer.depthBiasEnable = VK_FALSE;                    // Не использовать смещение глубины

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Мультисэмплинг
  // Мультисэмплинг (MSAA) - техника сглаживания краёв (anti-aliasing),
  // которая работает путем взятия нескольких выборок на пиксель.

  // Настройка мультисэмплинга
  vk::PipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sampleShadingEnable                    = VK_FALSE;  // Отключаем сэмпл-шейдинг
  multisampling.rasterizationSamples =
      vk::SampleCountFlagBits::e1;  // Количество выборок на пиксель

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Смешивание цветов
  // Color blending определяет, как новый цвет фрагмента смешивается с существующим цветом в буфере.
  // Существует два основных способа:
  // 1. Логические операции (побитовые операции между новым и старым цветом)
  // 2. Смешивание через уравнение (src * srcFactor + dst * dstFactor)

  // Настройка смешивания цветов
  vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;  // Отключаем смешивание для простоты

  vk::PipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.logicOpEnable                         = VK_FALSE;  // Отключаем логические операции
  colorBlending.logicOp = vk::LogicOp::eCopy;  // Не используется при logicOpEnable = VK_FALSE
  colorBlending.attachmentCount   = 1;
  colorBlending.pAttachments      = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Layout конвейера
  // Pipeline layout описывает, какие данные могут быть переданы в шейдеры.
  // Это включает uniform буферы, текстуры, push константы и т.д.
  // На этом этапе мы не используем никаких ресурсов, поэтому layout простой.

  // Создание layout конвейера
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.setLayoutCount               = 0;  // Не используем дескрипторные наборы
  pipelineLayoutInfo.pSetLayouts                  = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount       = 0;  // Не используем push константы
  pipelineLayoutInfo.pPushConstantRanges          = nullptr;

  try
  {
    m_vkPipelineLayout = m_device.getDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
    VulkanLogger::info("Pipeline layout создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать pipeline layout: " + std::string(e.what()));
  }

  // УЧЕБНЫЙ КОММЕНТАРИЙ: Объединение всех настроек конвейера
  // Все ранее созданные структуры объединяются в одну структуру создания графического конвейера.
  // Это демонстрирует явное определение всех этапов, которое требуется в Vulkan.

  // Создание графического конвейера
  vk::GraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.stageCount =
      2;  // Количество программируемых стадий (вершинный + фрагментный шейдеры)
  pipelineInfo.pStages             = shaderStages;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = nullptr;  // Не используем буфер глубины
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr;  // Не используем динамические состояния
  pipelineInfo.layout              = *m_vkPipelineLayout;
  pipelineInfo.renderPass          = *m_vkRenderPass;
  pipelineInfo.subpass             = 0;  // Индекс подпрохода

  // Можно указать родительский конвейер для наследования
  pipelineInfo.basePipelineHandle = nullptr;
  pipelineInfo.basePipelineIndex  = -1;

  try
  {
    // Создаем графический конвейер
    m_vkGraphicsPipeline =
        m_device.getDevice().createGraphicsPipelineUnique(nullptr, pipelineInfo).value;
    VulkanLogger::info("Графический конвейер создан успешно");
  }
  catch (const vk::SystemError& e)
  {
    throw std::runtime_error("Не удалось создать графический конвейер: " + std::string(e.what()));
  }

  // Шейдерные модули больше не нужны после создания конвейера
  // В нашем случае они будут автоматически уничтожены из-за использования unique_ptr
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