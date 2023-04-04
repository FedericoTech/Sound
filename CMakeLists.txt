cmake_minimum_required (VERSION 3.8)

project ("Sound" VERSION 0.1)

add_subdirectory(01CPUWaveGenerator)

add_subdirectory(02THWaveGenerator)

add_subdirectory(04CPUWaveMixer)

add_subdirectory(05THWaveMixer)

add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/glew-2.1.0/build/cmake" "${CMAKE_CURRENT_BINARY_DIR}/glew")

add_subdirectory(03GPUWaveGenerator)

add_subdirectory(06GPUWaveMixer)