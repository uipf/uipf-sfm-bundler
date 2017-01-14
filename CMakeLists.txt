cmake_minimum_required(VERSION 3.0.0)
project(uipf-sfm-bundler)

#c++11 support
if(${CMAKE_VERSION} VERSION_LESS 3.1.0)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
else()
	set(CMAKE_CXX_STANDARD 11)
endif()

if(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

	# sanitize memory issues https://gcc.gnu.org/gcc-4.8/changes.html
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
endif()

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")

# bundler
include(ExternalProject)
# build bundler as external project
ExternalProject_Add(BUNDLER
	SOURCE_DIR ${uipf-sfm-bundler_SOURCE_DIR}/bundler
	# make sure compiler settings are same as for our compiler to avoid linking issues
#	CMAKE_ARGS
#	-DLTO_ENABLED=OFF
#	-DOPENMP_ENABLED=OFF
#	-DCUDA_ENABLED=OFF
#	-DPROFILING_ENABLED=OFF
#	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
#	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
#	-DTESTS_ENABLED=OFF
	# parallelize the build to be faster
	BUILD_COMMAND make -j 4
	TEST_COMMAND ""
	INSTALL_COMMAND "" # skip install step for COLMAP
)



# find libuipf-module
# search in normal system paths as well as installation prefix for installation as unprivileged user.
find_library(
	ModuleBase uipf-module
	PATHS ${CMAKE_INSTALL_PREFIX}/lib
)
message("ModuleBase: ${ModuleBase}")
include_directories(${ModuleBase_INCLUDE_DIRS})


add_library(LoeweSiftModule SHARED LoeweSiftModule.cpp)
target_link_libraries(LoeweSiftModule ${ModuleBase})
add_dependencies(LoeweSiftModule BUNDLER)

# installation targets
install(TARGETS
	LoeweSiftModule

	LIBRARY DESTINATION lib/uipf
	ARCHIVE DESTINATION lib/uipf
)

# install headers
install(DIRECTORY ${uipf-sfm-sift_SOURCE_DIR}/include
	DESTINATION include/uipf-sfm/sift
	FILES_MATCHING PATTERN "*.hpp"
)
