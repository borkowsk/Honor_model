# CMAKE configuration for MAPS4HONOR tool
######################################################
# Because of <direct.h> for MSWindows only.
# @Author: Wojciech Borkowski
#

cmake_minimum_required(VERSION 2.9)

project(Maps4honor)

# SymShellLight library is required for compilation. 
# Enter path here.
set(SYMSHELL "../../../public/SymShellLight")

add_subdirectory( ${SYMSHELL} ${SYMSHELL} )

include_directories( ${SYMSHELL} )

add_executable( MapGeneratorForHonor
               "MapGeneratorForHonor.cpp"
#windows only
               "src/symshwin.ico"
               )

target_link_libraries( MapGeneratorForHonor
     "-L${SYMSHELL}"
     wbsyshX11 X11 Xpm
     rt
     )

