# CMAKE configuration for HONOR simulation project
######################################################################
# @Author: Wojciech Borkowski
# @Coauthor: Andrzej Nowak
#

cmake_minimum_required(VERSION 2.9)

project(honor_model)

# SymShellLight library is required for compilation. 
# Enter path here.
set(SYMSHELL "../../public/SymShellLight")

# Define WITHTOOLS 1 , if you want build helpers applications!
set(WITHTOOLS 0)

add_subdirectory( ${SYMSHELL} ${SYMSHELL} )

#if(WITHTOOLS)
#   add_subdirectory( mapGen/ )
#endif()

include_directories( ${SYMSHELL} )



IF (WIN32)

add_executable( honor
                "src/HonorAgent.cpp"
                "src/honor.cpp"
#windows only
               "src/symshmsc.rc"
               "src/symshwin.ico"
               )
               
target_link_libraries( honor
     "-L${SYMSHELL}"
     wbsyshLight
     )         
           
ELSE()

add_executable( honor
                "src/HonorAgent.cpp"
                "src/honor.cpp"
               )
               
target_link_libraries( honor
     "-L${SYMSHELL}"
     wbsyshLight
     X11 Xpm
     rt
     )
     
ENDIF()
