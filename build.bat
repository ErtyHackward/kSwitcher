@echo off
echo Building kSwitcher - Keyboard Switcher...

if not exist build mkdir build
cd build

echo Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo CMake failed. Make sure you have:
    echo - Visual Studio 2022 with C++ support
    echo - No external dependencies required
    pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed.
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable location: build\Release\kSwitcher.exe

echo Compressing executable with UPX...
if exist ..\upx-5.0.2-win64\upx.exe (
    ..\upx-5.0.2-win64\upx.exe --best Release\kSwitcher.exe
    echo Compression completed!
    goto end_upx
)

echo UPX not found - downloading UPX 5.0.2...
cd ..

REM Download UPX if zip doesn't exist
if not exist upx.zip (
    echo Downloading UPX 5.0.2...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/upx/upx/releases/download/v5.0.2/upx-5.0.2-win64.zip' -OutFile 'upx.zip'"
    if %ERRORLEVEL% neq 0 (
        echo Failed to download UPX. Skipping compression.
        cd build
        goto end_upx
    )
)

REM Extract UPX if directory doesn't exist
if not exist upx-5.0.2-win64 (
    echo Extracting UPX...
    powershell -Command "Expand-Archive -Path 'upx.zip' -DestinationPath '.' -Force"
    if %ERRORLEVEL% neq 0 (
        echo Failed to extract UPX. Skipping compression.
        cd build
        goto end_upx
    )
)

cd build

REM Try compression with downloaded UPX
if exist ..\upx-5.0.2-win64\upx.exe (
    echo Compressing with downloaded UPX...
    ..\upx-5.0.2-win64\upx.exe --best Release\kSwitcher.exe
    echo Compression completed!
) else (
    echo UPX extraction failed. Skipping compression.
)

:end_upx

pause