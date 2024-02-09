# CMAKE configuration for HONOR simulation project
######################################################################
# @authors: Wojciech Borkowski, Andrzej Nowak
# @date: 2024-02-09 (last modifications)
#
project(honor_model)
cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_STANDARD 11) # See: https://matgomes.com/setting-cpp-standard-cmake/

# Define WITHTOOLS if you want build helpers applications!
set(WITHTOOLS false)

if(WIN32)
  # set stuff for windows
  message( ${PROJECT_NAME} ": MS Windows platform assumed.\n\tInside C/C++ code automatic _MSC_VER variable will be used!" )
  set( WINDOWS true )
  #set( MSVC 1 ) #old macro used also somewhere in C code?
  #set( WINDOWS_ONLY_SRC  "${WBRTM}/BASE/wb_assert.c" )
  #add_compile_options( /W4 ) #  /WX
else()
  # set stuff for other systems
  message( ${PROJECT_NAME} ": Linux/unix platform with X11 assumed." )
  set( X11 true )
  #set( X_ONLY_SRC  "" )
  #add_compile_options( -Wall )
endif()

# SymShellLight library is required for compilation. 
# Enter path here.
set(SYMSHELL "../SymShellLight" )

add_subdirectory( ${SYMSHELL} ${SYMSHELL} )

include_directories( ${SYMSHELL} )

if(WINDOWS) # only for WINDOWS
    add_executable( honor
                    src/HonorAgent.cpp
                    src/honor.cpp
                    src/symshmsc.rc
                    src/symshwin.ico
                   )
    target_link_libraries( honor
         "-L${SYMSHELL}"
         wbsyshLight
         )
endif(WINDOWS)

if(X11) # only for X11 LIB
    add_definitions( -DHAVE_X11 -DUSE_X11 )
    add_executable( honor
                    src/HonorAgent.cpp
                    src/honor.cpp
                   )
    target_link_libraries( honor
         "-L${SYMSHELL}"
         wbsyshLight
         X11 Xpm
         rt
         )
endif(X11)

if(WITHTOOLS)
   add_subdirectory( mapGen/ )
endif()