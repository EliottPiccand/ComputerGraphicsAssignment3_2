cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME ComputerGraphics)
project(${PROJECT_NAME} LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_NAME main)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_compile_options(${EXECUTABLE_NAME} PRIVATE
  -Weverything
  -Wno-c++98-compat
  -Wno-c++98-compat-pedantic
  -Wno-padded
  -Wno-unique-object-duplication
  -Wno-unused-macros
  -Wno-switch-default
  -Wno-global-constructors
  -Wno-exit-time-destructors
  -Werror
  -O3
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/Lib
)

file(GLOB LIBS ${CMAKE_CURRENT_SOURCE_DIR}/Lib/*.lib)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBS})

find_package(OpenGL REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE OpenGL::GL OpenGL::GLU)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
  $<$<CONFIG:Debug>:OE_DEBUG>
  $<$<CONFIG:Release>:OE_RELEASE>
  $<$<CONFIG:Profiling>:OE_PROFILING>
)

if(CMAKE_BUILD_TYPE STREQUAL "Profiling")
    set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy" FORCE)
    add_subdirectory(Lib/tracy)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE Tracy::TracyClient)
endif()

find_package(Python REQUIRED COMPONENTS Interpreter)

# --- Shader Structs Check ---
add_custom_target(check-shaders-structs-layout ALL
  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/check_shaders_structs_layout.py
  COMMENT "Checking shaders structs layout"
)

add_dependencies(${EXECUTABLE_NAME} check-shaders-structs-layout)

# --- Assets ---
set(ASSETS_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/Assets)
set(ASSETS_DST_DIR ${CMAKE_BINARY_DIR}/Assets)
set(ASSETS_BYPRODUCT ${CMAKE_CURRENT_BINARY_DIR}/.copy_assets_timestamp)

add_custom_target(copy-assets ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/copy_assets.py ${ASSETS_SRC_DIR} ${ASSETS_DST_DIR} ${ASSETS_BYPRODUCT}
    COMMENT "Copying Assets"
    BYPRODUCTS ${ASSETS_BYPRODUCT}
)

add_dependencies(${EXECUTABLE_NAME} copy-assets)
