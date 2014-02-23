######################################################################
# CMake file for LDForge.
# The original QMake file was created on Sat Sep 22 17:29:49 2012
######################################################################

project (ldforge)
add_subdirectory (updaterevision)
cmake_minimum_required (VERSION 2.6)
find_package (Qt4 REQUIRED)
find_package (OpenGL REQUIRED)

get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} src/Git.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS updaterevision)

include_directories (${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set (LDForgeSources
	src/AddObjectDialog.cc
	src/Colors.cc
	src/ColorSelector.cc
	src/Configuration.cc
	src/ConfigurationDialog.cc
	src/CrashCatcher.cc
	src/Dialogs.cc
	src/Documentation.cc
	src/Document.cc
	src/EditHistory.cc
	src/ExternalPrograms.cc
	src/GLRenderer.cc
	src/LDConfig.cc
	src/LDObject.cc
	src/Main.cc
	src/MainWindow.cc
	src/MessageLog.cc
	src/Misc.cc
	src/PartDownloader.cc
	src/Primitives.cc
	src/Types.cc
	src/Version.cc
	src/Widgets.cc
	src/actions/MainActions.cc
	src/actions/EditActions.cc
)

set (LDForgeHeaders
	src/PropertyMacro.h
	src/CrashCatcher.h
	src/Colors.h
	src/misc/DocumentPointer.h
	src/misc/InvokationDeferer.h
	src/misc/RingFinder.h
	src/Document.h
	src/AddObjectDialog.h
	src/LDConfig.h
	src/PartDownloader.h
	src/LDObject.h
	src/Primitives.h
	src/Misc.h
	src/MessageLog.h
	src/Dialogs.h
	src/Widgets.h
	src/Documentation.h
	src/Main.h
	src/Types.h
	src/ColorSelector.h
	src/ConfigurationDialog.h
	src/GLRenderer.h
	src/Configuration.h
	src/MainWindow.h
	src/EditHistory.h
)

set (LDForgeForms
	ui/about.ui
	ui/addhistoryline.ui
	ui/bombbox.ui
	ui/colorsel.ui
	ui/config.ui
	ui/coverer.ui
	ui/downloadfrom.ui
	ui/edger2.ui
	ui/editraw.ui
	ui/extprogpath.ui
	ui/flip.ui
	ui/intersector.ui
	ui/isecalc.ui
	ui/ldforge.ui
	ui/ldrawpath.ui
	ui/makeprim.ui
	ui/newpart.ui
	ui/openprogress.ui
	ui/overlay.ui
	ui/rectifier.ui
	ui/replcoords.ui
	ui/rotpoint.ui
	ui/ytruder.ui
)

set (LDForgeResources ldforge.qrc)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lGLU")
include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_BINARY_DIR}/src")
include_directories ("${PROJECT_BINARY_DIR}/src/misc")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

qt4_wrap_cpp (LDForgeMOC ${LDForgeHeaders})
qt4_wrap_ui (LDForgeFormsHeaders ${LDForgeForms})
qt4_add_resources (LDForgeResourcesRCC ${LDForgeResources})

add_executable (ldforge
	${LDForgeSources}
	${LDForgeResourcesRCC}
	${LDForgeFormsHeaders}
	${LDForgeMOC}
)

target_link_libraries (ldforge
	${QT_QTCORE_LIBRARY}
	${QT_QTGUI_LIBRARY}
	${QT_QTNETWORK_LIBRARY}
	${QT_QTOPENGL_LIBRARY}
	${OPENGL_LIBRARIES}
)

install (TARGETS ldforge RUNTIME DESTINATION bin)
