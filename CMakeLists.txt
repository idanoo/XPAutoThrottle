cmake_minimum_required(VERSION 3.16)
project(XP_AutoThrottle)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set X-Plane SDK path (relative path)
set(XPLM_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SDK/4.2.0")

# Check if macOS
if(APPLE)
    # Set minimum macOS version support
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    
    # Set architectures (support Intel and Apple Silicon)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    
    # X-Plane plugins must disable some default settings
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".xpl")
    
    # Compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat -Wformat-security")
    
    # X-Plane specific compiler definitions
    add_definitions(-DAPL=1 -DIBM=0 -DLIN=0)
    add_definitions(-DXPLM200=1 -DXPLM300=1 -DXPLM400=1 -DXPLM410=1)  # Support X-Plane SDK versions
    
    # Header file include paths
    include_directories(
        "${XPLM_SDK_PATH}/CHeaders/XPLM"
        "${XPLM_SDK_PATH}/CHeaders/Widgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    
    # Source files
    set(SOURCES
        src/plugin.cpp
    )
    
    # Create dynamic library (X-Plane plugin)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    
    # Set output name to mac.xpl
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "mac"
        SUFFIX ".xpl"
        PREFIX ""
    )
    
    # Set framework search path in linker flags
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-F${XPLM_SDK_PATH}/Libraries/Mac -Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/mac_exports.txt"
    )
    
    # Link X-Plane frameworks
    target_link_libraries(${PROJECT_NAME}
        "-framework" "XPLM"
        "-framework" "XPWidgets"
        "-framework" "Carbon"
    )

elseif(WIN32)
    # Windows configuration
    message(STATUS "Configuring for Windows...")
    
    # X-Plane plugin settings
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".xpl")
    
    # Compiler flags - check compiler type
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Visual Studio compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /EHsc")
    else()
        # MinGW/GCC compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    endif()
    
    # X-Plane specific compiler definitions
    add_definitions(-DAPL=0 -DIBM=1 -DLIN=0)
    add_definitions(-DXPLM200=1 -DXPLM300=1 -DXPLM400=1 -DXPLM410=1)  # Support X-Plane SDK versions

    # Header file include paths
    include_directories(
        "${XPLM_SDK_PATH}/CHeaders/XPLM"
        "${XPLM_SDK_PATH}/CHeaders/Widgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    # Source files
    set(SOURCES
        src/plugin.cpp
    )
    
    # Create dynamic library (X-Plane plugin)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    
    # Set output name to win.xpl
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "win"
        SUFFIX ".xpl"
        PREFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}"
    )
    
    # Link X-Plane libraries
    target_link_libraries(${PROJECT_NAME}
        "${XPLM_SDK_PATH}/Libraries/Win/XPLM_64.lib"
        "${XPLM_SDK_PATH}/Libraries/Win/XPWidgets_64.lib"
    )
    
    # Use Windows symbol export file
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Visual Studio linker flags
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/win_exports.def"
        )
    else()
        # MinGW/GCC - add .def file directly as a source file to the linker
        # This is the correct way to use .def files with MinGW
        target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/win_exports.def")
    endif()

elseif(UNIX AND NOT APPLE)
    # Linux configuration
    message(STATUS "Configuring for Linux...")
    
    # X-Plane plugin settings
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".xpl")
    
    # Compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat -Wformat-security")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pipe")
    
    # X-Plane specific compiler definitions
    add_definitions(-DAPL=0 -DIBM=0 -DLIN=1)
    add_definitions(-DXPLM200=1 -DXPLM300=1 -DXPLM400=1 -DXPLM410=1)  # Support X-Plane SDK versions
    
    # Header file include paths
    include_directories(
        "${XPLM_SDK_PATH}/CHeaders/XPLM"
        "${XPLM_SDK_PATH}/CHeaders/Widgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    
    # Source files
    set(SOURCES
        src/plugin.cpp
    )
    
    # Create dynamic library (X-Plane plugin)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    
    # Set output name to lin.xpl
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "lin"
        SUFFIX ".xpl"
        PREFIX ""
    )
    
    # Link X-Plane libraries
    # On Linux, link directly to the .so files (they don't have lib prefix)
    target_link_libraries(${PROJECT_NAME}
        "${XPLM_SDK_PATH}/Libraries/Lin/XPLM_64.so"
        "${XPLM_SDK_PATH}/Libraries/Lin/XPWidgets_64.so"
        dl  # Dynamic link library
    )
    
    # Use Linux symbol export file
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux_exports.txt"
    )
endif()

# Output compilation information
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "SDK Path: ${XPLM_SDK_PATH}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
if(APPLE)
    message(STATUS "Target Architectures: ${CMAKE_OSX_ARCHITECTURES}")
    message(STATUS "macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()
