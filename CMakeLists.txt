cmake_minimum_required(VERSION 3.21)
project(PixelArtEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Gui)

qt_add_executable(PixelArtEditor
    src/main.cpp
    src/core/PixelDocument.cpp
    src/core/PixelDocument.h
    src/editor/CanvasView.cpp
    src/editor/CanvasView.h
    src/editor/MainWindow.cpp
    src/editor/MainWindow.h
    src/formats/ProjectFormat.cpp
    src/formats/ProjectFormat.h
    src/formats/GifFormat.cpp
    src/formats/GifFormat.h
)

target_link_libraries(PixelArtEditor PRIVATE Qt6::Widgets Qt6::Gui)
target_include_directories(PixelArtEditor PRIVATE src)
target_compile_definitions(PixelArtEditor PRIVATE PIXEL_ART_EDITOR_VERSION="${PROJECT_VERSION}")

install(TARGETS PixelArtEditor RUNTIME DESTINATION bin BUNDLE DESTINATION .)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.2 COMPONENTS Test QUIET)
    if(TARGET Qt6::Test)
        qt_add_executable(PixelArtEditorTests
            tests/PixelDocumentTest.cpp
            src/core/PixelDocument.cpp
            src/core/PixelDocument.h
            src/editor/CanvasView.cpp
            src/editor/CanvasView.h
            src/editor/MainWindow.cpp
            src/editor/MainWindow.h
            src/formats/ProjectFormat.cpp
            src/formats/ProjectFormat.h
            src/formats/GifFormat.cpp
            src/formats/GifFormat.h
        )
        target_link_libraries(PixelArtEditorTests PRIVATE Qt6::Widgets Qt6::Gui Qt6::Test)
        target_include_directories(PixelArtEditorTests PRIVATE src)
        add_test(NAME PixelArtEditorTests COMMAND PixelArtEditorTests)
        set_tests_properties(PixelArtEditorTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()

if(WIN32)
    set_target_properties(PixelArtEditor PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
