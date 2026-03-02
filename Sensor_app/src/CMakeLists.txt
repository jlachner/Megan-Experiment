CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

PROJECT(2671_app)

## Include directories (corrected)
#set(includeDir "/home/newman_lab/Desktop/Explicit-FRI/Libraries/FTSensor/include")          # Freedom units!
set(includeDir "/home/newman_lab/Desktop/FTSensor/include")                                  # Metric units!
include_directories(${includeDir})

## Make all source files visible in the IDE
set(sourceFiles "")
file(GLOB_RECURSE sourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE headerFiles ${includeDir}/*.h)  # Include header files explicitly

## Add executable
add_executable(${PROJECT_NAME} ${sourceFiles} ${headerFiles})

##### LINKING #####

# Correct path to FTSensor library
#set(SENSOR_LIBRARY_DIR "/home/newman_lab/Desktop/Explicit-FRI/Libraries/FTSensor/lib")          # Freedom units!
set(SENSOR_LIBRARY_DIR "/home/newman_lab/Desktop/FTSensor/lib")                                  # Metric units!
set(SENSOR_LIBRARY "${SENSOR_LIBRARY_DIR}/libFTSensor.so")  # Full path to .so file

# Ensure the linker can find the library
link_directories(${SENSOR_LIBRARY_DIR})

# Link the library to the project
target_link_libraries(${PROJECT_NAME} ${SENSOR_LIBRARY})

## Set output directory
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../build/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../build/Release"
)

## Install headers
install(DIRECTORY ${includeDir} DESTINATION ${CMAKE_INSTALL_PREFIX}/include
    FILES_MATCHING PATTERN "*.h"
)

set(libOutputDir ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${libOutputDir}
    ARCHIVE DESTINATION ${libOutputDir}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/application
)
