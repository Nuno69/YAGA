add_subdirectory(union)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE union)

# Prism (https://github.com/ethindp/prism, MPL-2.0) speaks through the running screen reader or a speech engine.
# It is built from its release source as a static library with the plugin's C runtime, so no extra DLL ships with
# YAGA. Building it with MSVC needs the "C++ ATL" component of Visual Studio.
include(FetchContent)

set(BUILD_SHARED_LIBS OFF)
set(PRISM_IPO OFF CACHE STRING "Interprocedural optimization for Prism" FORCE)

FetchContent_Declare(prism
	URL https://github.com/ethindp/prism/archive/refs/tags/v0.18.3.tar.gz
	URL_HASH SHA256=44d67e8f46fa52f912127eb69ae46298371c7078f1e33d2a5d7001685844ae1b
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(prism)

# CMake knows MSVC as a C23 compiler only from 4.0 on. Prism asks for C23, but its C sources belong to legacy backends
# that are not built, so C17 is enough.
if (MSVC AND CMAKE_VERSION VERSION_LESS 4.0)
	set_property(TARGET prism PROPERTY C_STANDARD 17)
endif()

# Prism exports NOMINMAX to its users, but the Gothic API headers need the min and max macros of Windows.h.
get_target_property(PRISM_INTERFACE_DEFINITIONS prism INTERFACE_COMPILE_DEFINITIONS)
list(REMOVE_ITEM PRISM_INTERFACE_DEFINITIONS NOMINMAX)
set_target_properties(prism PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${PRISM_INTERFACE_DEFINITIONS}")

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE prism::prism)
