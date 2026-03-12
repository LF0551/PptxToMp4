cmake_minimum_required(VERSION 3.26)
project(PptxToMp4)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(API_DIR "${CMAKE_SOURCE_DIR}")
set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin")
set(OUT_DIR "${BIN_DIR}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets AxContainer)
find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME}
        include/DraggableListWidget.h
        include/ImageRenamerWidget.h
        include/Mp4VideoCreator.h
        include/PdfConverter.h
        include/pch.h
        include/PresentationBatchConverter.h
        include/PptxConverter.h
        include/PptxToJpgConverter.h

        src/DraggableListWidget.cpp
        src/ImageRenamerWidget.cpp
        src/main.cpp
        src/Mp4VideoCreator.cpp
        src/PdfConverter.cpp
        src/PresentationBatchConverter.cpp
        src/PptxConverter.cpp
        src/PptxToJpgConverter.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::AxContainer ${OpenCV_LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${BIN_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
)
