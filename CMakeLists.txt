#
# TAS-Paths main cmake file
# @author Tobias Weber <tweber@ill.fr>
# @date feb-2021
# @license GPLv3, see 'LICENSE' file
#
# -----------------------------------------------------------------------------
# TAS-Paths (part of the Takin software suite)
# Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
#                     Grenoble, France).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------------
#

cmake_minimum_required(VERSION 3.0)

project(glscene)
enable_language(CXX)


list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/cmake"
	"${PROJECT_SOURCE_DIR}/tlibs2/cmake"
)

option(USE_QT6 "use qt 6" FALSE)
option(USE_LAPACK "use lapack" FALSE)
option(USE_BULLET "use bullet physics" TRUE)


if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()


set(GL_MAJOR_VER 3)
set(GL_MINOR_VER 3)

if(USE_QT6)
	set(QT_VER 6)
else()
	set(QT_VER 5)
endif()


message("Build type: ${CMAKE_BUILD_TYPE}.")
message("Compiler: " ${CMAKE_CXX_COMPILER_ID}.)
message("Selected Qt version ${QT_VER}.")
message("Selected GL version ${GL_MAJOR_VER}.${GL_MINOR_VER}.")


set(WARN_OPTS -Wall -Wextra -Weffc++)
add_compile_options(${WARN_OPTS})


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# generate debug symbols
	add_compile_options(-g -ggdb)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_options("-DNDEBUG")
	add_compile_options("-Wno-#pragma-messages")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		# remove pragma messages
		# see: https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html
		add_compile_options(-fcompare-debug-second)
	endif()
endif()


# -----------------------------------------------------------------------------
# compiler settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)

add_definitions(-D_GL_MAJ_VER=${GL_MAJOR_VER} -D_GL_MIN_VER=${GL_MINOR_VER})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# system specific settings
# -----------------------------------------------------------------------------
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

set(BOOST_SUFFIX)
set(MINGW_WINSOCK)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)

	set(BOOST_SUFFIX -x64)
	set(MINGW_WINSOCK "ws2_32")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	#add_compile_options(-mmacosx-version-min=10.15)
	add_compile_options(-mmacosx-version-min=11.0)
endif()

include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/externals")

if(QT_VER EQUAL 6)
	include_directories(SYSTEM "${Qt6Core_INCLUDE_DIRS}/..")
elseif(QT_VER EQUAL 5)
	include_directories(SYSTEM "${Qt5Core_INCLUDE_DIRS}/..")
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
# -----------------------------------------------------------------------------
find_package(Threads REQUIRED)

set(Boost_NO_BOOST_CMAKE FALSE)
set(Boost_USE_MULTITHREADED TRUE)
set(Boost_FIND_QUIETLY FALSE)

find_package(Boost REQUIRED COMPONENTS filesystem${BOOST_SUFFIX})
message("Using Boost version ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}.")
include_directories(SYSTEM "${Boost_INCLUDE_DIRS}/..")


if(USE_LAPACK)
	find_package(Lapacke)
	if(Lapacke_FOUND)
		message("Lapacke enabled.")
		add_definitions(-DUSE_LAPACK)
		include_directories(SYSTEM "${Lapacke_INCLUDE_DIRS}")
	else()
		message("Lapacke disabled.")
	endif()
endif()


if(USE_BULLET)
	find_package(Bullet)
	if(Bullet_FOUND)
		message("Bullet enabled.")
		add_definitions(-DUSE_BULLET)
		include_directories(SYSTEM "${BULLET_INCLUDE_DIRS}")
	else()
		message("Bullet disabled.")
	endif()
endif()


if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED
		COMPONENTS Core Gui Svg Widgets
			OpenGLWidgets PrintSupport)
	list(APPEND QtBaseLibraries Qt6::Core Qt6::Gui Qt6::Widgets
		Qt6::Svg)
	list(APPEND QtAllLibraries Qt6::Core Qt6::Gui Qt6::Widgets
		Qt6::Svg Qt6::OpenGLWidgets Qt6::PrintSupport)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED
		COMPONENTS Core Gui Svg Widgets PrintSupport)
	list(APPEND QtBaseLibraries Qt5::Core Qt5::Gui Qt5::Widgets
		Qt5::Svg)
	list(APPEND QtAllLibraries Qt5::Core Qt5::Gui Qt5::Widgets
		Qt5::Svg Qt5::PrintSupport)
else()
	message(FATAL_ERROR "Unknown Qt version selected: ${QT_VER}")
endif()

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# more compiler settings
# -----------------------------------------------------------------------------
add_compile_options(${Boost_CXX_FLAGS})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	# disable some warnings for generated source files
	set_source_files_properties("${PROJECT_BINARY_DIR}/taspaths_autogen/mocs_compilation.cpp"
		PROPERTIES COMPILE_FLAGS "-Wno-effc++")
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# target executable settings
# -----------------------------------------------------------------------------
add_executable(glscene
	src/main.cpp
	src/MainWnd.cpp src/MainWnd.h
	src/settings_variables.cpp src/settings_variables.h

	src/common/Resources.cpp src/common/Resources.h

	src/renderer/GlRenderer.cpp src/renderer/GlRenderer.h
	#src/renderer/Camera.h

	src/dock/CamProperties.cpp src/dock/CamProperties.h
	src/dock/SelectionProperties.cpp src/dock/SelectionProperties.h

	src/dialogs/About.cpp src/dialogs/About.h
	src/dialogs/GeoBrowser.cpp src/dialogs/GeoBrowser.h
	src/dialogs/TextureBrowser.cpp src/dialogs/TextureBrowser.h
	src/dialogs/Settings.h

	src/Geometry.cpp src/Geometry.h
	src/Scene.cpp src/Scene.h
	src/types.h

	tlibs2/libs/qt/gl.cpp tlibs2/libs/qt/gl.h
	tlibs2/libs/qt/recent.cpp tlibs2/libs/qt/recent.h
	tlibs2/libs/maths.h tlibs2/libs/cam.h
)


target_link_libraries(glscene
	"${Boost_LIBRARIES}"
	"${Lapacke_LIBRARIES}"
	"${MINGW_WINSOCK}"
	Threads::Threads
	"${QtAllLibraries}"
	"${BULLET_LIBRARIES}"
)
