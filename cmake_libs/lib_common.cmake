set(SAVE_PATH ${CMAKE_CURRENT_LIST_DIR})
if (UNIX)
    include(${CMAKE_CURRENT_LIST_DIR}/lib_unix.cmake)
endif (UNIX)

function(add_common_options TARGET)
    add_common_options_internal(${TARGET} ${SAVE_PATH})
endfunction(add_common_options)
