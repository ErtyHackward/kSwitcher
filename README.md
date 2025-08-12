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

---

# kSwitcher - Переключатель раскладки клавиатуры

Легковесное приложение для системного трея Windows для переключения раскладки клавиатуры с интеллектуальной коррекцией текста.

## Возможности

- Нажмите клавишу **Pause/Break** для мгновенной коррекции текста, набранного в неправильной раскладке. Автоматически переключает раскладку и перенабирает текст правильно
- Комбинация **Alt+Shift** для ручного переключения раскладки
- Настройки сохраняются в `%APPDATA%\kSwitcher\settings.yml`
- Приложение занимает всего 115Кб, дополнительные зависимости не нужны
- Приложение автоматически установит себя
- Без рекламы и отслеживания использования, одобрено Клиппи

## Альтернативы

- [Mahou](https://gitea.com/BladeMight/Mahou)
- [dotSwitcher](https://github.com/kurumpa/dotSwitcher)
- [PuntoSwitcher](http://punto.yandex.ru/)

## Сборка из исходного кода

### Требования

1. **Установите Visual Studio 2022** (любая редакция)
   - Скачать с: https://visualstudio.microsoft.com/downloads/
   - Обязательно выберите рабочую нагрузку "Разработка классических приложений на C++"

2. **Установите CMake**
   - Скачать с: https://cmake.org/download/
   - Добавьте в PATH во время установки

### Сборка
```bash
build.bat
```

## Лицензия

Лицензия MIT