cmake_minimum_required(VERSION 3.21)
project(WinDown3000 VERSION 3000.0.4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(WINDOWN_STATIC_BUILD "Build a fully static single-exe binary" OFF)
option(WINDOWN_USE_WEBVIEW2  "Use Microsoft WebView2 instead of QWebEngine" OFF)

# Auto-detect: default to WebView2 on Windows
if(WIN32 AND NOT DEFINED WINDOWN_USE_WEBVIEW2)
    set(WINDOWN_USE_WEBVIEW2 ON)
endif()

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

if(WINDOWN_USE_WEBVIEW2)
    add_compile_definitions(WINDOWN_USE_WEBVIEW2)
else()
    find_package(Qt6 REQUIRED COMPONENTS WebEngineWidgets)
endif()

# ---------------------------------------------------------------------------
# Hoedown (Markdown parser – fetched at configure time)
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    hoedown
    GIT_REPOSITORY https://github.com/hoedown/hoedown.git
    GIT_TAG        3.0.7
)
FetchContent_MakeAvailable(hoedown)

# Build Hoedown as a static library from its source files
file(GLOB HOEDOWN_SOURCES "${hoedown_SOURCE_DIR}/src/*.c")
add_library(hoedown STATIC ${HOEDOWN_SOURCES})
target_include_directories(hoedown PUBLIC "${hoedown_SOURCE_DIR}/src")

# ---------------------------------------------------------------------------
# WebView2 SDK (Windows only)
# ---------------------------------------------------------------------------
if(WINDOWN_USE_WEBVIEW2 AND WIN32)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2792.45
        URL_HASH SHA256=0  # update with real hash for production
    )
    FetchContent_MakeAvailable(webview2)
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(WINDOWN_SOURCES
    src/main.cpp
    src/MainWindow.h            src/MainWindow.cpp
    src/Document.h              src/Document.cpp
    src/Editor.h                src/Editor.cpp
    src/FindReplaceDialog.h     src/FindReplaceDialog.cpp
    src/MarkdownHighlighter.h   src/MarkdownHighlighter.cpp
    src/MarkdownRenderer.h      src/MarkdownRenderer.cpp
    src/PreferencesDialog.h     src/PreferencesDialog.cpp
    src/PreviewWidget.h         src/PreviewWidget.cpp
    src/Preferences.h           src/Preferences.cpp
    src/ThemeParser.h           src/ThemeParser.cpp
)

# Windows resource file (icon + version info)
if(WIN32)
    list(APPEND WINDOWN_SOURCES resources/windown3000.rc)
endif()

# ---------------------------------------------------------------------------
# Resources
# ---------------------------------------------------------------------------
set(WINDOWN_RESOURCES resources/windown3000.qrc)

# ---------------------------------------------------------------------------
# Application target
# ---------------------------------------------------------------------------
if(WIN32)
    qt_add_executable(WinDown3000 WIN32 ${WINDOWN_SOURCES} ${WINDOWN_RESOURCES})
else()
    qt_add_executable(WinDown3000 ${WINDOWN_SOURCES} ${WINDOWN_RESOURCES})
endif()

target_link_libraries(WinDown3000 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    hoedown
)

if(WINDOWN_USE_WEBVIEW2 AND WIN32)
    target_include_directories(WinDown3000 PRIVATE
        "${webview2_SOURCE_DIR}/build/native/include"
    )
    target_link_directories(WinDown3000 PRIVATE
        "${webview2_SOURCE_DIR}/build/native/x64"
    )
    target_link_libraries(WinDown3000 PRIVATE WebView2LoaderStatic.lib)
else()
    target_link_libraries(WinDown3000 PRIVATE Qt6::WebEngineWidgets)
endif()

target_include_directories(WinDown3000 PRIVATE src)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS WinDown3000 RUNTIME DESTINATION bin)

# On Windows, bundle the WebView2Loader.dll if using dynamic linking
if(WIN32 AND WINDOWN_USE_WEBVIEW2 AND NOT WINDOWN_STATIC_BUILD)
    install(FILES
        "${webview2_SOURCE_DIR}/build/native/x64/WebView2Loader.dll"
        DESTINATION bin
    )
endif()

# ---------------------------------------------------------------------------
# Static build configuration
# ---------------------------------------------------------------------------
if(WINDOWN_STATIC_BUILD AND WIN32)
    set_target_properties(WinDown3000 PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
    target_link_options(WinDown3000 PRIVATE /LTCG /OPT:REF /OPT:ICF)
endif()
