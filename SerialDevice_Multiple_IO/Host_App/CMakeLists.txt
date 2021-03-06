cmake_minimum_required(VERSION 3.17)
project(SerialDevice_Multiple_IO)

set(CMAKE_CXX_STANDARD 14)

find_path(SerialDeviceHost_DIR "")
find_package(SerialDeviceHost REQUIRED PATHS "${SerialDeviceHost_DIR}")

if (DEFINED MSVC_VERSION)
    if (MSVC_VERSION GREATER_EQUAL 1500)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
    endif()
    if (MSVC_VERSION GREATER_EQUAL 1910)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
    endif()
endif()

set (IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui)
file (GLOB IMGUI_SRC "${IMGUI_DIR}/*.cpp")
include_directories("${IMGUI_DIR}")

if (WIN32)
    set(GL3W_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gl3w)
    include_directories("${GL3W_DIR}")
    set(_GLFW_Arch 32)
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(_GLFW_Arch 64)
    endif()
    set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glfw)
    include_directories("${GLFW_DIR}/include")
    add_executable(${PROJECT_NAME} main.cpp ${IMGUI_SRC} ${GL3W_DIR}/GL/gl3w.c  ${SerialDeviceHost_Src})
    target_link_libraries(${PROJECT_NAME} opengl32.lib "${GLFW_DIR}/lib-vc2010-${_GLFW_Arch}/glfw3.lib" ${SerialDeviceHost_Lib})
else()
    find_package(OpenGL REQUIRED)
    include_directories( ${OPENGL_INCLUDE_DIRS})

    find_package(glfw3 REQUIRED)
    include_directories(${GLFW_INCLUDE_DIRS})
    link_libraries(${GLFW_LIBRARY_DIRS})

    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})

    if (APPLE)
        find_library(COCOA_LIBRARY Cocoa)
        find_library(OpenGL_LIBRARY OpenGL)
        find_library(IOKIT_LIBRARY IOKit)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        SET(EXTRA_LIBS ${COCOA_LIBRARY} ${OpenGL_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
    endif (APPLE)

    add_executable(${PROJECT_NAME} main.cpp ${IMGUI_SRC} ${SerialDeviceHost_Src})
    target_link_libraries(${PROJECT_NAME} glfw ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${EXTRA_LIBS} ${SerialDeviceHost_Lib})

endif()
