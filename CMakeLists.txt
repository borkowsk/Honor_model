# CMAKE configuration for COEVO-lution1 simulation project
######################################################################
# @Author: Wojciech Borkowski
# Define WITHTOOLS if you want build helpers applications!
set(WITHTOOLS 1)
#
cmake_minimum_required(VERSION 2.9)

project(coevolution1)

set(SYMSHELL "../SymShellLight")
add_subdirectory( ${SYMSHELL} ${SYMSHELL} )
if(WITHTOOLS)
   add_subdirectory( HELPERS/ )
endif()

include_directories( ${SYMSHELL} )

add_executable( coevo1
                coevo.cpp
                )

target_link_libraries( coevo1 
     "-L${SYMSHELL}"
     wbsyshX11 X11 Xpm
     rt
     )
