cmake_minimum_required(VERSION 3.21)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(target_add_versioninfo)

project(YAGA
	DESCRIPTION "Adds accessibility for blind players"
	VERSION 0.3.0.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC
    "src/*.cpp"
	"resources/*.h"
	"resources/*.rc"
)

add_library(${CMAKE_PROJECT_NAME} SHARED)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SRC})

target_include_directories(${CMAKE_PROJECT_NAME}
    INTERFACE
		"include/"

	PRIVATE
		BEFORE "userapi/"
		"src/"
		"resources/"
)

target_add_versioninfo(${CMAKE_PROJECT_NAME}
	VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}"
	COMPANY_NAME "Gothic Community"
	PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
)

add_subdirectory(dependencies)

# VDF build target

set(VDF_DIR "${PROJECT_SOURCE_DIR}/vdf/")
set(VDF_AUTORUN_DIR "${VDF_DIR}System/Autorun/")
set(VDF_SOUND_DIR "${VDF_DIR}_Work/Data/Sound/SFX/")

set(VDF_BUILD_VM_FILE "${VDF_DIR}build.vm")
string(REPLACE "/" "\\" VDF_BUILD_VM_FILE "${VDF_BUILD_VM_FILE}")

option(YAGA_BUILD_SOUNDS "Build sound assets from Csound sources" ON)

if (YAGA_BUILD_SOUNDS)
	find_program(CSOUND NAMES csound csound64)
	if (NOT CSOUND)
		message(FATAL_ERROR "Csound not found. Install Csound or configure with -DYAGA_BUILD_SOUNDS=OFF.")
	endif()

	file(GLOB CSOUND_SOURCE_FILES CONFIGURE_DEPENDS
		"${PROJECT_SOURCE_DIR}/audio/csound/cues/*.csd"
	)

	set(SOUND_OUTPUT_FILES)
	foreach(CSOUND_SOURCE_FILE ${CSOUND_SOURCE_FILES})
		get_filename_component(SOUND_NAME "${CSOUND_SOURCE_FILE}" NAME_WE)
		set(SOUND_OUTPUT_FILE "${VDF_SOUND_DIR}${SOUND_NAME}.WAV")

		add_custom_command(
			OUTPUT "${SOUND_OUTPUT_FILE}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${VDF_SOUND_DIR}"
			COMMAND ${CSOUND} -+ignore_csopts=1 -d -W -o "${SOUND_OUTPUT_FILE}" "${CSOUND_SOURCE_FILE}"
			DEPENDS "${CSOUND_SOURCE_FILE}"
			COMMENT "[Sound] Rendering ${SOUND_NAME}.WAV"
			VERBATIM
		)

		list(APPEND SOUND_OUTPUT_FILES "${SOUND_OUTPUT_FILE}")
	endforeach()

	add_custom_target(sounds
		DEPENDS ${SOUND_OUTPUT_FILES}
	)
	add_dependencies(${CMAKE_PROJECT_NAME} sounds)
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${VDF_AUTORUN_DIR}
		COMMAND ${CMAKE_COMMAND} -E echo "[Post Build] $<TARGET_FILE_NAME:${CMAKE_PROJECT_NAME}> copied to: ${VDF_AUTORUN_DIR}")

add_custom_target(vdf
		COMMAND ${CMAKE_COMMAND} -E echo "[VDF] Generating archive..."
		COMMAND "${VDF_DIR}/GothicVDFS.exe" /B "${VDF_BUILD_VM_FILE}"
		COMMAND ${CMAKE_COMMAND} -E echo "[VDF] archive build successfully!"
		WORKING_DIRECTORY "${VDF_DIR}"
)

# clang-format support
find_program(CLANG_FORMAT "clang-format")
if (CLANG_FORMAT)
	file(GLOB_RECURSE ALL_SOURCE_FILES
		"${CMAKE_SOURCE_DIR}/src/*.cpp"
		"${CMAKE_SOURCE_DIR}/src/*.h"
		"${CMAKE_SOURCE_DIR}/src/*.hpp"
	)
	add_custom_target(format
		COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_SOURCE_FILES}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
	message(STATUS "Format target enabled. Run 'cmake --build . --target format' to format code.")
else()
	message(WARNING "clang-format not found. Format target will not be available.")
endif()
