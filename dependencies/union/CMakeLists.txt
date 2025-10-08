add_subdirectory(union-api)
add_subdirectory(gothic-api)

# union
add_library(union INTERFACE)

target_link_libraries(union
	INTERFACE
		gothic_api
		union_api_lib
)