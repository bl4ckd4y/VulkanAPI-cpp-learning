#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace VulkanUtils
{
  /**
   * @brief Проверяет результат выполнения Vulkan-функции
   * @param result Результат выполнения функции
   * @param message Сообщение об ошибке
   * @return true если операция выполнена успешно
   */
  bool checkVkResult(VkResult result, const std::string& message);

  /**
   * @brief Загружает файл в вектор байтов
   * @param filename Имя файла
   * @return Вектор с содержимым файла
   */
  std::vector<char> readFile(const std::string& filename);
}  // namespace VulkanUtils