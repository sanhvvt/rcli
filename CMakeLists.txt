
cmake_minimum_required(VERSION 3.5)

project(rcli C CXX)
option(rcli_ENABLE_SANITIZE "use sanitize" OFF)
option(rcli_BUILD_SHARED "build shared" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if(rcli_ENABLE_SANITIZE)
    message(STATUS "CMAKE_CXX_FLAGS: sanitize")	
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif()

# find path: xxx_ROOT_DIR
# output var: xxx_INCLUDES, xxx_LIBRARIES
macro(find_my_library arg_1 arg_2 arg_3)
	foreach(x ${arg_2})
		unset(y CACHE)
		find_library(y NAMES ${x} HINTS ${${arg_1}_ROOT_DIR}/lib NO_SYSTEM_ENVIRONMENT_PATH)
		list(APPEND ${arg_1}_LIBRARIES ${y})
	endforeach(x)
	if (NOT ${arg_1}_LIBRARIES)
		message(FATAL_ERROR "Cannot find ${arg_1}_LIBRARIES: ${${arg_1}_ROOT_DIR}/lib, ${arg_2}")
	endif()
	message("Find ${arg_1}_LIBRARIES: ${${arg_1}_LIBRARIES}")

	find_path(${arg_1}_INCLUDES NAMES ${arg_3} HINTS ${${arg_1}_ROOT_DIR}/include NO_SYSTEM_ENVIRONMENT_PATH)
	if (NOT ${arg_1}_INCLUDES)
		message(FATAL_ERROR "Cannot find ${arg_1}_INCLUDES: ${${arg_1}_ROOT_DIR}/include, ${arg_3}")
	endif()
	message("Find ${arg_1}_INCLUDES: ${${arg_1}_INCLUDES}")
endmacro()

# find_my_library(hiredis hiredis hiredis/hiredis.h)

include(ExternalProject)
set(libhiredis_SOURCE_DIR ${PROJECT_SOURCE_DIR}/hiredis-1.2.0)
set(libhiredis_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libhiredis)
set(libhiredis_INSTALL_DIR ${libhiredis_BINARY_DIR}/install)
set(libhiredis_INCLUDES ${libhiredis_INSTALL_DIR}/include)
set(libhiredis_LIBRARIES ${libhiredis_INSTALL_DIR}/lib/libhiredis.a)
ExternalProject_Add( libhiredis
	PREFIX libhiredis
	SOURCE_DIR ${libhiredis_SOURCE_DIR}
	BINARY_DIR ${libhiredis_BINARY_DIR}
	INSTALL_DIR ${libhiredis_INSTALL_DIR}
	BUILD_BYPRODUCTS ${libhiredis_LIBRARIES}
	CMAKE_CACHE_ARGS
		-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
		-DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo
		-DCMAKE_INSTALL_PREFIX:STRING=${libhiredis_INSTALL_DIR}
		-DCMAKE_INSTALL_LIBDIR:STRING=lib
		-DBUILD_SHARED_LIBS:BOOL=OFF
	)

include_directories(${libhiredis_INCLUDES})
file(GLOB rcli_src src/*.cpp)
file(GLOB rcli_hdr src/*.h)

if(rcli_BUILD_SHARED)
	add_library(rcli SHARED ${rcli_src} ${rcli_hdr})
	add_dependencies(rcli libhiredis)
	target_link_libraries(rcli PRIVATE ${libhiredis_LIBRARIES})
else()
	add_library(rcli STATIC ${rcli_src} ${rcli_hdr})
	add_dependencies(rcli libhiredis)
	target_link_libraries(rcli PRIVATE ${libhiredis_LIBRARIES})
endif()

install(FILES rcli.h DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
install(TARGETS rcli DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
if (WIN32)
	set_target_properties(rcli PROPERTIES OUTPUT_NAME librcli)
endif(WIN32)

include_directories(src)
file(GLOB test_src test/*.cpp)
file(GLOB test_hdr test/*.h)

add_executable(test_rcli ${test_src} ${test_hdr})
add_dependencies(test_rcli rcli)
target_link_libraries(test_rcli rcli)