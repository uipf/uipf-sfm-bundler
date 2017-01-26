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
	CONFIGURE_COMMAND rsync -r ${uipf-sfm-bundler_SOURCE_DIR}/bundler .
	# parallelize the build to be faster
	BUILD_COMMAND cd bundler && make default -j 4
	TEST_COMMAND ""
	INSTALL_COMMAND "" # skip install step for COLMAP
)
ExternalProject_Get_Property(BUNDLER binary_dir)
set(BUNDLER_BINARY_DIR "${binary_dir}")


# find libuipf-module
# search in normal system paths as well as installation prefix for installation as unprivileged user.
find_library(
	ModuleBase uipf-module
	PATHS ${CMAKE_INSTALL_PREFIX}/lib
)
message("ModuleBase: ${ModuleBase}")
include_directories(${CMAKE_INSTALL_PREFIX}/include)


add_library(BundlerMatcherModule SHARED BundlerMatcherModule.cpp)
target_link_libraries(BundlerMatcherModule ${ModuleBase})
add_dependencies(BundlerMatcherModule BUNDLER)

add_library(BundlerSfMModule SHARED BundlerSfMModule.cpp)
target_link_libraries(BundlerSfMModule ${ModuleBase})
add_dependencies(BundlerSfMModule BUNDLER)

# installation targets
install(TARGETS
	BundlerMatcherModule
	BundlerSfMModule

	LIBRARY DESTINATION lib/uipf
	ARCHIVE DESTINATION lib/uipf
)

# install headers
#install(DIRECTORY ${uipf-sfm-sift_SOURCE_DIR}/include
#	DESTINATION include/uipf-sfm/sift
#	FILES_MATCHING PATTERN "*.hpp"
#)
install(FILES ${BUNDLER_BINARY_DIR}/bundler/bin/KeyMatchFull
	DESTINATION bin/
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	RENAME uipf-sfm-bundler-keymatchfull
)
install(FILES ${BUNDLER_BINARY_DIR}/bundler/bin/bundler
	DESTINATION bin/
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	RENAME uipf-sfm-bundler
)

