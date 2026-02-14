cmake_minimum_required(VERSION 3.11)

project(transport_catalogue CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_MODULE 
	src/core/transport_catalogue.h
	src/core/transport_catalogue.cpp
	src/core/geo.h
	src/core/geo.cpp
	src/core/domain.h
)

set(IO_MODULE 
	src/io/json.h
	src/io/json.cpp
	src/io/json_builder.h
	src/io/json_builder.cpp
	src/io/json_reader.h
	src/io/json_reader.cpp
)

set(MAP_MODULE 
	src/map/map_renderer.h 
	src/map/map_renderer.cpp 
	src/map/svg.h 
	src/map/svg.cpp
)

set(HANDLER_MODULE
	src/request_handler/request_handler.h 
	src/request_handler/request_handler.cpp
)

set(ROUTER_MODULE
	src/router/transport_router.h 
	src/router/transport_router.cpp 
	src/router/router.h 
	src/router/ranges.h 
	src/router/graph.h
)

add_executable(transport_catalogue
	src/main.cpp
	${CORE_MODULE}
	${IO_MODULE}
	${MAP_MODULE}
	${HANDLER_MODULE}
	${ROUTER_MODULE}
)

target_include_directories(transport_catalogue PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/core
	${CMAKE_CURRENT_SOURCE_DIR}/src/io
	${CMAKE_CURRENT_SOURCE_DIR}/src/map
	${CMAKE_CURRENT_SOURCE_DIR}/src/request_handler
	${CMAKE_CURRENT_SOURCE_DIR}/src/router
)
