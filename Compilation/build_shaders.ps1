# compile_shaders.ps1
# Скрипт для компиляции шейдеров GLSL в формат SPIR-V

# Путь к glslc.exe (компилятор шейдеров из Vulkan SDK)
$GLSLC = "C:/VulkanSDK/1.4.313.0/Bin/glslc.exe"

# Проверка существования компилятора
if (-not (Test-Path $GLSLC)) {
  Write-Error "Не найден компилятор шейдеров glslc по пути: $GLSLC"
  Write-Error "Убедитесь, что Vulkan SDK установлен и путь корректен"
  exit 1
}

# Проверка существования директории шейдеров
if (-not (Test-Path "Learning/Shaders")) {
  Write-Host "Создание директории 'Learning/Shaders'..."
  New-Item -Path "Learning/Shaders" -ItemType Directory | Out-Null
}

# Проверка существования исходных файлов шейдеров
if (-not (Test-Path "Learning/Shaders/triangle.vert")) {
  Write-Error "Не найден исходный файл вершинного шейдера: Learning/Shaders/triangle.vert"
  exit 1
}

if (-not (Test-Path "Learning/Shaders/triangle.frag")) {
  Write-Error "Не найден исходный файл фрагментного шейдера: Learning/Shaders/triangle.frag"
  exit 1
}

# Компиляция вершинного шейдера
Write-Host "Компиляция вершинного шейдера..."
$vertCmd = "& `"$GLSLC`" -c `"Learning/Shaders/triangle.vert`" -o `"Learning/Shaders/triangle.vert.spv`""
Write-Host "Выполняется: $vertCmd"
Invoke-Expression $vertCmd
if ($LASTEXITCODE -ne 0) {
  Write-Error "Ошибка при компиляции вершинного шейдера"
  exit $LASTEXITCODE
}

# Компиляция фрагментного шейдера
Write-Host "Компиляция фрагментного шейдера..."
$fragCmd = "& `"$GLSLC`" -c `"Learning/Shaders/triangle.frag`" -o `"Learning/Shaders/triangle.frag.spv`""
Write-Host "Выполняется: $fragCmd"
Invoke-Expression $fragCmd
if ($LASTEXITCODE -ne 0) {
  Write-Error "Ошибка при компиляции фрагментного шейдера"
  exit $LASTEXITCODE
}

# Проверка создания файлов
if (Test-Path "Learning/Shaders/triangle.vert.spv") {
  Write-Host "Вершинный шейдер скомпилирован успешно: Learning/Shaders/triangle.vert.spv"
}
else {
  Write-Error "Файл вершинного шейдера не создан!"
}

if (Test-Path "Learning/Shaders/triangle.frag.spv") {
  Write-Host "Фрагментный шейдер скомпилирован успешно: Learning/Shaders/triangle.frag.spv"
}
else {
  Write-Error "Файл фрагментного шейдера не создан!"
}

Write-Host "Компиляция шейдеров завершена!"
Write-Host "Текущий каталог: $(Get-Location)"
Write-Host "Содержимое каталога шейдеров:"
Get-ChildItem -Path "Learning/Shaders" | Format-Table Name, Length, LastWriteTime 