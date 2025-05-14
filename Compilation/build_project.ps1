# build_project.ps1

# 1. Удаляем старую папку сборки
Write-Host "Очистка предыдущей сборки..."
if (Test-Path -Path ".\build" -PathType Container) {
  Remove-Item -Recurse -Force ".\build"
  Write-Host "Папка 'build' удалена."
}
else {
  Write-Host "Папка 'build' не найдена, очистка не требуется."
}

# 2. Конфигурируем проект с помощью CMake
Write-Host "Конфигурирование проекта CMake..."
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake" -A x64
if ($LASTEXITCODE -ne 0) {
  Write-Error "Ошибка на этапе конфигурации CMake. Сборка прервана."
  exit $LASTEXITCODE
}
Write-Host "Конфигурация CMake завершена успешно."

# 3. Собираем проект
Write-Host "Сборка проекта..."
cmake --build build
if ($LASTEXITCODE -ne 0) {
  Write-Error "Ошибка на этапе сборки проекта. Проверьте вывод."
  exit $LASTEXITCODE
}
Write-Host "Сборка проекта завершена успешно!"

Read-Host -Prompt "Нажмите Enter для выхода..." 