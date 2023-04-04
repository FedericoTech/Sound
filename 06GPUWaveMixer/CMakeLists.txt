
set(PROGRAM_NAME 06GPUWaveMixer)

find_package( OpenGL REQUIRED )

add_executable (${PROGRAM_NAME}
	"06GPUWaveMixer.cpp"
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