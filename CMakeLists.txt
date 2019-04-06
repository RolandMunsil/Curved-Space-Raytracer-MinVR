list(APPEND CMAKE_PREFIX_PATH C:/Users/rolan/OneDrive/Code/MinVR/build/install)

cmake_minimum_required (VERSION 3.9)
set (CMAKE_CXX_STANDARD 11)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE )
endif()

list(APPEND CMAKE_MODULE_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_INSTALL_PREFIX})
list(INSERT CMAKE_PREFIX_PATH 0 ${CMAKE_INSTALL_PREFIX})

include(AutoBuild)
AutoBuild_init()


project(4d-raytracer)

message(STATUS "==== BUILDING ${PROJECT_NAME}")
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")


# Source:
set (SOURCEFILES 
  main.cpp
)
set (HEADERFILES
)
set (EXTRAFILES
  shaders/shader.frag
  shaders/shader.vert
)
set_source_files_properties(${EXTRAFILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${PROJECT_NAME} ${HEADERFILES} ${SOURCEFILES} ${EXTRAFILES})

include_directories("include")

set(EXTERNAL_DIR_NAME external)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR_NAME})
set(EXTERNAL_CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(MinVR REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC MinVR::MinVR)

# OpenGL
include(AutoBuildOpenGL)
AutoBuild_use_package_OpenGL(${PROJECT_NAME} PUBLIC)

if (NOT MSVC)
   find_package(ZLIB REQUIRED)
endif()

if (APPLE)
   find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
   target_link_libraries(${PROJECT_NAME} PUBLIC ${CORE_FOUNDATION_FRAMEWORK})
endif()

# GLEW
if (NOT APPLE)
    include(AutoBuildGLEW)
    AutoBuild_use_package_GLEW(${PROJECT_NAME} PUBLIC)
endif()


if (WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "./Debug")
	#set_target_properties(${WINDOWS_BINARIES} PROPERTIES VS_STARTUP_PROJECT ${PROJECT_NAME})
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif(WIN32)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if( MSVC )
  # in order to prevent DLL hell, each of the DLLs have to be suffixed with the major version and msvc prefix
  if( MSVC70 OR MSVC71 )
    set(MSVC_PREFIX "vc70")
  elseif( MSVC80 )
    set(MSVC_PREFIX "vc80")
  elseif( MSVC90 )
    set(MSVC_PREFIX "vc90")
  elseif( MSVC10 )
    set(MSVC_PREFIX "vc100")
  elseif( MSVC11 )
    set(MSVC_PREFIX "vc110")
  elseif( MSVC12 )
    set(MSVC_PREFIX "vc120")
  elseif( MSVC14 )
    set(MSVC_PREFIX "vc140")
  else()
    set(MSVC_PREFIX "vc150")
  endif()

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy
                         ${MinVR_DIR}/../../../bin/glew32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}> 
		  			   )
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy
                         ${MinVR_DIR}/../../../bin/glew32d.dll $<TARGET_FILE_DIR:${PROJECT_NAME}> 
		  			   )					   
endif()					   
				  

AutoBuild_check_status()