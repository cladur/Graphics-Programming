# CPM - package manager
include(CPM)

# OpenGL
find_package(OpenGL REQUIRED)

# glad
set(GLAD_DIR ${CMAKE_CURRENT_LIST_DIR}/glad)
set(glad_SOURCE_DIR ${GLAD_DIR}/include CACHE INTERNAL "")
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PRIVATE ${GLAD_DIR}/include)

# stb_image
set(STB_IMAGE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb_image)
set(stb_image_SOURCE_DIR ${STB_IMAGE_DIR} CACHE INTERNAL "")
add_library(stb_image STATIC ${STB_IMAGE_DIR}/stb_image.cpp)
target_include_directories(stb_image PRIVATE ${STB_IMAGE_DIR})

# other
CPMAddPackage("gh:assimp/assimp@5.2.5")
CPMAddPackage("gh:glfw/glfw#3.3.8")
CPMAddPackage("gh:g-truc/glm#master")
CPMAddPackage("gh:ocornut/imgui@1.88#docking")
CPMAddPackage("gh:gabime/spdlog@1.10.0")
CPMAddPackage("gh:CedricGuillemet/ImGuizmo#master")

set(imgui_SOURCE_DIR ${imgui_SOURCE_DIR} CACHE INTERNAL "")
add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp
					     ${imgui_SOURCE_DIR}/imgui_demo.cpp
					     ${imgui_SOURCE_DIR}/imgui_draw.cpp
						 ${imgui_SOURCE_DIR}/imgui_tables.cpp
					     ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

# ImGuizmo
# set(IM_GUIZMO_DIR ${CMAKE_CURRENT_LIST_DIR}/ImGuizmo)
set(ImGuizmo_SOURCE_DIR ${ImGuizmo_SOURCE_DIR} CACHE INTERNAL "")
# set(IM_GUIZMO_SOURCE_DIR ${IM_GUIZMO_DIR} CACHE INTERNAL "")
add_library(imguizmo STATIC ${ImGuizmo_SOURCE_DIR}/ImGuizmo.cpp
                            ${ImGuizmo_SOURCE_DIR}/GraphEditor.cpp
                            ${ImGuizmo_SOURCE_DIR}/ImCurveEdit.cpp
                            ${ImGuizmo_SOURCE_DIR}/ImGradient.cpp
                            ${ImGuizmo_SOURCE_DIR}/ImGuizmo.cpp
                            ${ImGuizmo_SOURCE_DIR}/ImSequencer.cpp)
add_dependencies(imguizmo imgui)
target_include_directories(imguizmo PRIVATE ${imgui_SOURCE_DIR})

set_target_properties(glad
                      stb_image
                      assimp
                      glfw
                      glm
                      imgui
                      imguizmo
                      spdlog PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()
