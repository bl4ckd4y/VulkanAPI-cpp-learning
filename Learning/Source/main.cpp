#include <iostream>

#include "VulkanApp.h"
#include "VulkanLogger.h"

int main()
{
  // Инициализация логгера в начале работы приложения
  VulkanLogger::init("vulkan_app.log", true, LogLevel::DEBUG);
  VulkanLogger::info("Запуск Vulkan приложения...");

  try
  {
    VulkanLogger::info("Создание объекта приложения...");
    VulkanApp app;

    VulkanLogger::info("Запуск приложения...");
    int result = app.run();

    VulkanLogger::info("Приложение завершено с кодом: " + std::to_string(result));

    // Закрываем логгер ПОСЛЕ всех вызовов, но ПЕРЕД return
    VulkanLogger::cleanup();

    // В main можно оставить вывод для консольного интерфейса
    if (result != 0)
    {
      std::cout << "Нажмите Enter для выхода..." << std::endl;
      std::cin.get();
    }

    return result;
  }
  catch (const std::exception& e)
  {
    VulkanLogger::fatal("Критическая ошибка: " + std::string(e.what()));
    VulkanLogger::cleanup();

    // Вывод в консоль оставляем для пользовательского интерфейса
    std::cerr << "Критическая ошибка: " << e.what() << std::endl;
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();

    return EXIT_FAILURE;
  }
  catch (...)
  {
    VulkanLogger::fatal("Неизвестная критическая ошибка!");
    VulkanLogger::cleanup();

    // Вывод в консоль оставляем для пользовательского интерфейса
    std::cerr << "Неизвестная критическая ошибка!" << std::endl;
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();

    return EXIT_FAILURE;
  }
}