# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++11")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram.out")

add_definitions(-DPLATFORM_OSX)

# load core library
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/*.h)

# add sources and include headers
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/osx/*.h 
    ${PROJECT_SOURCE_DIR}/osx/*.cpp)

# link and build functions
function(link_libraries)
    find_library(OPENGL_FRAMEWORK OpenGL)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
    
    list(APPEND GLFW_LIBRARY -lglfw3
                             ${OPENGL_FRAMEWORK}
                             ${COCOA_FRAMEWORK}
                             ${IOKIT_FRAMEWORK}
                             ${CORE_FOUNDATION_FRAMEWORK}
                             ${CORE_VIDEO_FRAMEWORK})

    check_and_link_libraries(${EXECUTABLE_NAME} curl)
    target_link_libraries(${EXECUTABLE_NAME} ${GLFW_LIBRARY} core)
endfunction()

function(build) 
    add_executable(${EXECUTABLE_NAME} ${SOURCES})
endfunction()
