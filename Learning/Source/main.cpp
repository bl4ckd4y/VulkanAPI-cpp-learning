#include <iostream>

#include "VulkanApp.h"

int main()
{
  std::cout << "Запуск Vulkan приложения..." << std::endl;

  try
  {
    std::cout << "Создание объекта приложения..." << std::endl;
    VulkanApp app;

    std::cout << "Запуск приложения..." << std::endl;
    int result = app.run();

    std::cout << "Приложение завершено с кодом: " << result << std::endl;
    return result;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Критическая ошибка: " << e.what() << std::endl;

    // Ожидание ввода для предотвращения закрытия консоли
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();

    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Неизвестная критическая ошибка!" << std::endl;

    // Ожидание ввода для предотвращения закрытия консоли
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();

    return EXIT_FAILURE;
  }
}