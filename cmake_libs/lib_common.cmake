list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_local_changes(GIT_LOCAL_CHANGES)

include_directories(${CMAKE_CURRENT_LIST_DIR})

configure_file("${CMAKE_CURRENT_LIST_DIR}/cmake_modules/GitSHA1.cpp.in" "${CMAKE_BINARY_DIR}/GitSHA1.cpp" @ONLY)

set(COMMON_UTILS_GITSHA
    ${CMAKE_BINARY_DIR}/GitSHA1.cpp
)

set(SAVE_PATH ${CMAKE_CURRENT_LIST_DIR})
if (UNIX AND NOT APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/lib_unix.cmake)
elseif (APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/lib_mac.cmake)
elseif (WIN32)
endif (UNIX)

function(add_common_options TARGET)
    add_common_options_internal(${TARGET} ${SAVE_PATH})
endfunction(add_common_options)
