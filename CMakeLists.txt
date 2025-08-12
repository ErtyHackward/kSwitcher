cmake_minimum_required(VERSION 3.16)
project(kSwitcher VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static linking
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(BUILD_SHARED_LIBS OFF)

# No external dependencies needed

# Source files
set(SOURCES
    src/main.cpp
    src/Settings.cpp
    src/NativeTrayIcon.cpp
    src/KeyboardInterceptor.cpp
    src/TrayApplication.cpp
    src/Installation.cpp
    src/kSwitcher.rc
)

# Headers
set(HEADERS
    src/Settings.h
    src/NativeTrayIcon.h
    src/KeyboardInterceptor.h
    src/TrayApplication.h
    src/Installation.h
    src/resource.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    user32 
    shell32 
    gdi32
    uxtheme
    dwmapi
    advapi32
    shcore
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Compiler definitions for size optimization
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    UNICODE 
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    VC_EXTRALEAN
    STRICT
)

# Compiler flags for size optimization
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/Os>     # Optimize for size
        $<$<CONFIG:Release>:/GL>     # Whole program optimization
        $<$<CONFIG:Release>:/Gy>     # Function-level linking
        $<$<CONFIG:Release>:/GF>     # String pooling
        $<$<CONFIG:Release>:/Gw>     # Global data optimization
    )
    
    # Linker flags for size optimization  
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF /MERGE:.rdata=.text"
    )
    
    # Additional size optimizations for Release
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
    )
endif()

# Set subsystem to Windows (GUI application) and size optimizations
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

# Force static linking for all targets
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
