
set(PROGRAM_NAME 03GPUWaveGenerator)

find_package( OpenGL REQUIRED )

#add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/../../SDK/glew-2.1.0/build/cmake" "${CMAKE_CURRENT_BINARY_DIR}/glew")

add_executable (${PROGRAM_NAME}
	"03GPUWaveGenerator.cpp"
)

target_include_directories(${PROGRAM_NAME} PRIVATE "../../SDK/glew-2.1.0/include/")
target_link_libraries( ${PROGRAM_NAME} glew_s )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

target_include_directories(${PROGRAM_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../SDK/GLFW/include/")

if(MSVC)
	target_link_libraries(${PROGRAM_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../../SDK/GLFW/lib-vc2022/glfw3.lib")
else()
	target_link_libraries(${PROGRAM_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../../SDK/GLFW/lib-mingw-w64/libglfw3.a")
endif()