#include "VulkanUtils.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace VulkanUtils
{
  bool checkVkResult(VkResult result, const std::string& message)
  {
    if (result != VK_SUCCESS)
    {
      std::cerr << "Vulkan ошибка (" << result << "): " << message << std::endl;
      return false;
    }
    return true;
  }

  std::vector<char> readFile(const std::string& filename)
  {
    // Открытие файла с конца (бинарный режим)
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
      throw std::runtime_error("Не удалось открыть файл: " + filename);
    }

    // Определение размера файла
    size_t            fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    // Возврат в начало и чтение файла
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }
}  // namespace VulkanUtils