list(APPEND CMAKE_MODULE_PATH "./")
if (UNIX)
    include(lib_unix)
endif (UNIX)

function(add_common_options TARGET)
    add_common_options_internal(${TARGET})
endfunction(add_common_options)
