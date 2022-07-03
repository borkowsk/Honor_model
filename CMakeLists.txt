# CMAKE configuration for HONOR simulation project
######################################################################
# @Author: Wojciech Borkowski
# @Coauthor: Andrzej Nowak
#
# Define WITHTOOLS if you want build helpers applications!
#
cmake_minimum_required(VERSION 2.9)
set(WITHTOOLS 0)
project(honor_model)

set(SYMSHELL "../../public/SymShellLight")
add_subdirectory( ${SYMSHELL} ${SYMSHELL} )

#if(WITHTOOLS)
#   add_subdirectory( HELPERS/ )
#endif()

include_directories( ${SYMSHELL} )

add_executable( honor
                "src/HonorAgent.cpp"
                "src/honor.cpp"
#windows only
#               "src/symshmsc.rc"
#               "src/symshwin.ico"
               )

target_link_libraries( honor
     "-L${SYMSHELL}"
     wbsyshX11 X11 Xpm
     rt
     )
