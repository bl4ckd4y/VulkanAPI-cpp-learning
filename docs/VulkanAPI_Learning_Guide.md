# Структурированное руководство по изучению Vulkan API

## Введение

Данное руководство предназначено для поэтапного изучения Vulkan API с использованием учебного проекта на C++. Мы будем строить понимание Vulkan API, начиная с базовых концепций и переходя к более сложным темам. Проект организован с учетом принципов RAII (Resource Acquisition Is Initialization) и современных практик разработки на C++.

## Содержание

1. [Основы Vulkan API](#основы-vulkan-api)
2. [Архитектура учебного проекта](#архитектура-учебного-проекта)
3. [Этап 1: Инициализация Vulkan и создание окна](#этап-1-инициализация-vulkan-и-создание-окна)
4. [Этап 2: Устройства и очереди](#этап-2-устройства-и-очереди)
5. [Этап 3: Swap Chain и представление изображений](#этап-3-swap-chain-и-представление-изображений)
6. [Этап 4: Графический конвейер и шейдеры](#этап-4-графический-конвейер-и-шейдеры)
7. [Этап 5: Командные буферы и рендеринг](#этап-5-командные-буферы-и-рендеринг)
8. [Дальнейшие шаги](#дальнейшие-шаги)

## Основы Vulkan API

Vulkan — это кроссплатформенный API с низким уровнем абстракции для 3D-графики и вычислений. В отличие от OpenGL, Vulkan:

- Предоставляет больше контроля над аппаратным обеспечением
- Требует явного управления ресурсами
- Минимизирует накладные расходы драйвера
- Позволяет лучше распределять нагрузку между несколькими ядрами CPU

Ключевые концепции Vulkan:
- **Явное управление**: все ресурсы создаются, используются и уничтожаются явно
- **Валидационные слои**: отделение отладки от производительности
- **Многопоточность**: эффективное использование нескольких потоков без блокировок драйвера

## Архитектура учебного проекта

Наш проект организован согласно принципам RAII и модульного программирования:

- `VulkanCore`: базовый модуль для инициализации Vulkan и создания экземпляра
- `VulkanDevice`: управление физическим и логическим устройствами
- `VulkanSwapChain`: управление цепочкой обмена для отображения
- `VulkanRenderer`: рендеринг и графический конвейер
- `VulkanApp`: основной класс приложения, объединяющий все компоненты

## Этап 1: Инициализация Vulkan и создание окна

### 1.1 Инициализация Vulkan

Первый шаг в работе с Vulkan — создание экземпляра (`VkInstance`). Этот процесс включает:

- Предоставление информации о приложении
- Указание необходимых расширений
- Настройку валидационных слоев для отладки

Ключевой файл: [`VulkanCore.cpp`](../Learning/Source/VulkanCore.cpp)

```cpp
void VulkanCore::createInstance()
{
  // Информация о приложении
  vk::ApplicationInfo appInfo = {};
  appInfo.pApplicationName    = "Vulkan Learning App";
  appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName         = "No Engine";
  appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion          = VK_API_VERSION_1_2;

  // Создание экземпляра Vulkan с нужными расширениями
  vk::InstanceCreateInfo createInfo = {};
  createInfo.pApplicationInfo        = &appInfo;
  // ... настройка расширений и слоев ...
  
  // Создание экземпляра с использованием RAII
  m_vkInstance = vk::createInstanceUnique(createInfo);
}
```

### 1.2 Создание поверхности

Поверхность (`VkSurfaceKHR`) - это абстракция, связывающая Vulkan с оконной системой. Для ее создания используется SDL2:

```cpp
// Создание поверхности через SDL2
VkSurfaceKHR surfaceHandle;
if (!SDL_Vulkan_CreateSurface(m_pWindow, static_cast<VkInstance>(*m_vkInstance), &surfaceHandle))
{
  throw std::runtime_error("Не удалось создать поверхность Vulkan");
}
// Обертка в RAII объект
m_vkSurface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surfaceHandle), *m_vkInstance);
```

### 1.3 Валидационные слои

Валидационные слои — мощный инструмент отладки в Vulkan, проверяющий корректность вызовов API:

```cpp
// Проверка поддержки валидационных слоев
if (m_enableValidationLayers && !checkValidationLayerSupport())
{
  throw std::runtime_error("Запрошенные валидационные слои недоступны!");
}

// Включение валидационных слоев
if (m_enableValidationLayers)
{
  createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
  createInfo.ppEnabledLayerNames = m_validationLayers.data();
}
```

## Этап 2: Устройства и очереди

### 2.1 Физическое устройство

Физическое устройство (`VkPhysicalDevice`) представляет GPU. Процесс выбора устройства включает:

- Получение списка доступных GPU
- Проверку поддержки нужной функциональности
- Выбор наиболее подходящего устройства

Ключевой файл: [`VulkanDevice.cpp`](../Learning/Source/VulkanDevice.cpp)

```cpp
void VulkanDevice::pickPhysicalDevice()
{
  // Получение списка устройств
  std::vector<vk::PhysicalDevice> devices = m_vkInstance.enumeratePhysicalDevices();
  
  // Выбор подходящего устройства
  for (const auto& device : devices)
  {
    if (isDeviceSuitable(device))
    {
      m_vkPhysicalDevice = device;
      return;
    }
  }
}
```

### 2.2 Семейства очередей

Очереди в Vulkan используются для выполнения команд на GPU. Различные типы очередей поддерживают разные операции:

- Графические очереди: для рендеринга
- Вычислительные очереди: для вычислений
- Очереди передачи: для копирования данных
- Очереди презентации: для отображения на экран

```cpp
QueueFamilyIndices VulkanDevice::findQueueFamilies(vk::PhysicalDevice device)
{
  QueueFamilyIndices indices;
  
  // Получение свойств семейств очередей
  auto queueFamilies = device.getQueueFamilyProperties();
  
  // Поиск графической очереди
  for (uint32_t i = 0; i < queueFamilies.size(); i++)
  {
    if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
    {
      indices.graphicsFamily = i;
    }
    
    // Проверка поддержки презентации
    if (device.getSurfaceSupportKHR(i, m_vkSurface))
    {
      indices.presentFamily = i;
    }
    
    if (indices.isComplete())
    {
      break;
    }
  }
  
  return indices;
}
```

### 2.3 Логическое устройство

Логическое устройство (`VkDevice`) - это интерфейс для взаимодействия с физическим устройством:

```cpp
void VulkanDevice::createLogicalDevice()
{
  // Получение индексов семейств очередей
  QueueFamilyIndices indices = m_queueFamilyIndices;
  
  // Создание информации о очередях
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };
  
  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    vk::DeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }
  
  // Создание логического устройства
  vk::DeviceCreateInfo createInfo = {};
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  // ... настройка расширений и функций ...
  
  m_vkDevice = m_vkPhysicalDevice.createDeviceUnique(createInfo);
  
  // Получение очередей
  m_vkGraphicsQueue = m_vkDevice->getQueue(indices.graphicsFamily.value(), 0);
  m_vkPresentQueue = m_vkDevice->getQueue(indices.presentFamily.value(), 0);
}
```

## Этап 3: Swap Chain и представление изображений

### 3.1 Поддержка Swap Chain

Swap chain - это набор буферов для отображения на экран. Нужно проверить поддержку и параметры:

Ключевой файл: [`VulkanSwapChain.cpp`](../Learning/Source/VulkanSwapChain.cpp)

```cpp
SwapChainSupportDetails VulkanSwapChain::querySwapChainSupport(vk::PhysicalDevice device)
{
  SwapChainSupportDetails details;
  
  // Получение основных возможностей
  details.capabilities = device.getSurfaceCapabilitiesKHR(m_vkSurface);
  
  // Получение поддерживаемых форматов
  details.formats = device.getSurfaceFormatsKHR(m_vkSurface);
  
  // Получение режимов представления
  details.presentModes = device.getSurfacePresentModesKHR(m_vkSurface);
  
  return details;
}
```

### 3.2 Создание Swap Chain

При создании swap chain нужно выбрать:
- Формат поверхности (цветовой формат и пространство)
- Режим представления (vsync или нет)
- Размеры изображений
- Количество изображений

```cpp
void VulkanSwapChain::createSwapChain()
{
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_device.getPhysicalDevice());
  
  // Выбор оптимальных параметров
  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
  
  // Количество изображений в swap chain
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }
  
  // Создание swap chain
  vk::SwapchainCreateInfoKHR createInfo = {};
  createInfo.surface = m_vkSurface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  // ... другие параметры ...
  
  m_vkSwapChain = m_device.getDevice().createSwapchainKHRUnique(createInfo);
  
  // Получение созданных изображений
  m_vkSwapChainImages = m_device.getDevice().getSwapchainImagesKHR(*m_vkSwapChain);
}
```

### 3.3 Image Views

Image views предоставляют доступ к изображениям:

```cpp
void VulkanSwapChain::createImageViews()
{
  m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());
  
  for (size_t i = 0; i < m_vkSwapChainImages.size(); i++)
  {
    vk::ImageViewCreateInfo createInfo = {};
    createInfo.image = m_vkSwapChainImages[i];
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = m_vkSwapChainImageFormat;
    createInfo.components.r = vk::ComponentSwizzle::eIdentity;
    createInfo.components.g = vk::ComponentSwizzle::eIdentity;
    createInfo.components.b = vk::ComponentSwizzle::eIdentity;
    createInfo.components.a = vk::ComponentSwizzle::eIdentity;
    createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    
    m_vkSwapChainImageViews[i] = m_device.getDevice().createImageViewUnique(createInfo);
  }
}
```

## Этап 4: Графический конвейер и шейдеры

### 4.1 Render Pass

Render pass описывает буферы и их использование во время рендеринга:

Ключевой файл: [`VulkanRenderer.cpp`](../Learning/Source/VulkanRenderer.cpp)

```cpp
void VulkanRenderer::createRenderPass()
{
  // Описание attachment
  vk::AttachmentDescription colorAttachment = {};
  colorAttachment.format = m_swapChain.getImageFormat();
  colorAttachment.samples = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  
  // Ссылка на attachment
  vk::AttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
  
  // Подпроход
  vk::SubpassDescription subpass = {};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  
  // Зависимость для синхронизации
  vk::SubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.srcAccessMask = vk::AccessFlagBits();
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  
  // Создание render pass
  vk::RenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;
  
  m_vkRenderPass = m_device.getDevice().createRenderPassUnique(renderPassInfo);
}
```

### 4.2 Шейдеры

В Vulkan шейдеры компилируются в бинарный формат SPIR-V перед запуском приложения:

```cpp
// Загрузка скомпилированных шейдеров
std::vector<char> vertShaderCode = readFile("Learning/Shaders/triangle.vert.spv");
std::vector<char> fragShaderCode = readFile("Learning/Shaders/triangle.frag.spv");

// Создание шейдерных модулей
vk::UniqueShaderModule vertShaderModule = createShaderModule(vertShaderCode);
vk::UniqueShaderModule fragShaderModule = createShaderModule(fragShaderCode);

// Настройка этапов шейдеров
vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {};
vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
vertShaderStageInfo.module = *vertShaderModule;
vertShaderStageInfo.pName = "main";

vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {};
fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
fragShaderStageInfo.module = *fragShaderModule;
fragShaderStageInfo.pName = "main";

vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

### 4.3 Графический конвейер

Графический конвейер описывает весь процесс рендеринга:

```cpp
void VulkanRenderer::createGraphicsPipeline()
{
  // ... загрузка шейдеров ...
  
  // Конфигурация фиксированных стадий
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
  vk::PipelineViewportStateCreateInfo viewportState = {};
  vk::PipelineRasterizationStateCreateInfo rasterizer = {};
  vk::PipelineMultisampleStateCreateInfo multisampling = {};
  vk::PipelineColorBlendStateCreateInfo colorBlending = {};
  
  // ... настройка всех стадий ...
  
  // Создание layout конвейера
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
  m_vkPipelineLayout = m_device.getDevice().createPipelineLayoutUnique(pipelineLayoutInfo);
  
  // Создание графического конвейера
  vk::GraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = *m_vkPipelineLayout;
  pipelineInfo.renderPass = *m_vkRenderPass;
  pipelineInfo.subpass = 0;
  
  m_vkGraphicsPipeline = m_device.getDevice().createGraphicsPipelineUnique(nullptr, pipelineInfo);
}
```

## Этап 5: Командные буферы и рендеринг

### 5.1 Командные буферы

Командные буферы используются для записи и выполнения команд на GPU:

```cpp
void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
  // Начало записи команд
  vk::CommandBufferBeginInfo beginInfo = {};
  commandBuffer.begin(beginInfo);
  
  // Начало render pass
  vk::RenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.renderPass = *m_vkRenderPass;
  renderPassInfo.framebuffer = *m_vkSwapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
  renderPassInfo.renderArea.extent = m_swapChain.getExtent();
  
  vk::ClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;
  
  commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
  
  // Привязка графического конвейера
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_vkGraphicsPipeline);
  
  // Привязка вершинного буфера
  vk::Buffer vertexBuffers[] = {*m_vkVertexBuffer};
  vk::DeviceSize offsets[] = {0};
  commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
  
  // Команда отрисовки
  commandBuffer.draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
  
  // Конец render pass и записи команд
  commandBuffer.endRenderPass();
  commandBuffer.end();
}
```

### 5.2 Синхронизация

Для синхронизации между CPU и GPU используются семафоры и заборы:

```cpp
void VulkanRenderer::createSyncObjects()
{
  m_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  
  vk::SemaphoreCreateInfo semaphoreInfo = {};
  vk::FenceCreateInfo fenceInfo = {};
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
  
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    m_vkImageAvailableSemaphores[i] = m_device.getDevice().createSemaphoreUnique(semaphoreInfo);
    m_vkRenderFinishedSemaphores[i] = m_device.getDevice().createSemaphoreUnique(semaphoreInfo);
    m_vkInFlightFences[i] = m_device.getDevice().createFenceUnique(fenceInfo);
  }
}
```

### 5.3 Основной цикл рендеринга

Основной цикл рендеринга выполняет следующие шаги:
1. Ожидание завершения предыдущего кадра
2. Получение изображения из swap chain
3. Запись и отправка командного буфера
4. Возврат изображения в swap chain для отображения

```cpp
bool VulkanRenderer::drawFrame()
{
  try
  {
    // Ожидание завершения предыдущего кадра
    m_device.getDevice().waitForFences(1, &(*m_vkInFlightFences[m_currentFrame]), VK_TRUE, UINT64_MAX);
    
    // Получение индекса изображения из swap chain
    uint32_t imageIndex;
    vk::ResultValue<uint32_t> result = m_device.getDevice().acquireNextImageKHR(
        *m_swapChain.getSwapChain(),
        UINT64_MAX,
        *m_vkImageAvailableSemaphores[m_currentFrame],
        nullptr
    );
    imageIndex = result.value;
    
    // Сброс забора для нового кадра
    m_device.getDevice().resetFences(1, &(*m_vkInFlightFences[m_currentFrame]));
    
    // Запись команд
    vk::CommandBuffer& commandBuffer = *m_vkCommandBuffers[m_currentFrame];
    commandBuffer.reset();
    recordCommandBuffer(commandBuffer, imageIndex);
    
    // Отправка командного буфера на выполнение
    vk::SubmitInfo submitInfo = {};
    vk::Semaphore waitSemaphores[] = {*m_vkImageAvailableSemaphores[m_currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vk::Semaphore signalSemaphores[] = {*m_vkRenderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    m_device.getGraphicsQueue().submit(submitInfo, *m_vkInFlightFences[m_currentFrame]);
    
    // Отображение изображения на экран
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    vk::SwapchainKHR swapChains[] = {*m_swapChain.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    m_device.getPresentQueue().presentKHR(presentInfo);
    
    // Переход к следующему кадру
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
    return true;
  }
  catch (const vk::SystemError& e)
  {
    return false;
  }
}
```

## Дальнейшие шаги

После освоения базовых принципов Vulkan, можно продолжить изучение:

1. **Uniform буферы и Push константы**: для передачи данных в шейдеры
2. **Текстуры и сэмплеры**: для текстурирования объектов
3. **Буфер глубины**: для корректного рендеринга 3D-объектов
4. **Модели и освещение**: загрузка 3D-моделей и реализация освещения
5. **Вычислительные шейдеры**: для неграфических вычислений на GPU
6. **Постобработка**: эффекты постобработки изображения
7. **Оптимизация**: улучшение производительности и эффективное использование ресурсов

## Заключение

Vulkan предоставляет огромные возможности для создания высокопроизводительных графических приложений, но требует тщательного понимания его архитектуры и концепций. Данное руководство является лишь отправной точкой в освоении Vulkan API. Продолжайте изучение, экспериментируйте и создавайте собственные проекты, чтобы полностью раскрыть потенциал этой технологии.

---

*Автор: Учебный проект по изучению Vulkan API*
*Дата: 2023* 