# kSwitcher - The keyboard layout switcher

A lightweight Windows system tray application for keyboard layout switching with intelligent text correction.

## Features

- Press **Pause/Break** key to instantly correct text typed in wrong keyboard layout. Automatically switches layout and retypes the text correctly
- **Alt+Shift** combination for manual layout switching
- Settings stored in `%APPDATA%\kSwitcher\settings.yml`
- The app is only 115Kb, no dependencies needed
- The app will auto-install itself
- No ads or usage tracking, approved by Clippy

## Alternatives

- [Mahou](https://gitea.com/BladeMight/Mahou)
- [dotSwitcher](https://github.com/kurumpa/dotSwitcher)
- [PuntoSwitcher](http://punto.yandex.ru/)

## Building from source

### Requirements

1. **Install Visual Studio 2022** (any edition)
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Make sure to select "Desktop development with C++" workload

2. **Install CMake**
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

### Build
```bash
build.bat
```

## License

MIT License