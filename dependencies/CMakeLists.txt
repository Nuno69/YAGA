add_subdirectory(union)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE union)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF)

# Recast & Detour (https://github.com/recastnavigation/recastnavigation, zlib licence) build the navigation mesh the
# navigator walks on from the loaded world's geometry. Static libraries; the demo, tests and examples are not built.
# 64-bit polygon references, because a Gothic world needs more tiles and polygons than 32-bit ones can address.
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_DT_POLYREF64 ON CACHE BOOL "" FORCE)
# Its CMakeLists asks for CMake 3.1, which CMake 4 no longer accepts without this.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(recastnavigation
	URL https://github.com/recastnavigation/recastnavigation/archive/refs/tags/v1.6.0.tar.gz
	URL_HASH SHA256=d48ca0121962fa0639502c0f56c4e3ae72f98e55d88727225444f500775c0074
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(recastnavigation)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE RecastNavigation::Recast RecastNavigation::Detour)
