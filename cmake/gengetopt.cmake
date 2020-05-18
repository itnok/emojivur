macro(find_gengetopt)
    if(NOT GENGETOPT_EXECUTABLE)
        find_program(GENGETOPT_EXECUTABLE gengetopt)
        if(NOT GENGETOPT_EXECUTABLE)
            message(FATAL_ERROR "gengetopt not found - aborting!")
        endif(NOT GENGETOPT_EXECUTABLE)
    endif(NOT GENGETOPT_EXECUTABLE)
endmacro(find_gengetopt)

macro(add_gengetopt_files GENGETOPT_TARGET_NAME)
    find_gengetopt()

    foreach (_CURRENT_FILE ${ARGN})
        get_filename_component(_IN ${_CURRENT_FILE} ABSOLUTE)
        get_filename_component(_BASENAME ${_CURRENT_FILE} NAME_WE)

        set(_OUT ${CMAKE_CURRENT_BINARY_DIR}/${_BASENAME}.c)
        set(_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${_BASENAME}.h)

        add_custom_command(
            OUTPUT ${_OUT} ${_HEADER}
            COMMAND ${GENGETOPT_EXECUTABLE}
            ARGS
                --input=${_IN}
                --file-name=${_BASENAME}
                --include-getopt
            DEPENDS ${_IN}
        )
        add_custom_target(${GENGETOPT_TARGET_NAME}_GENGETOPT_FILES DEPENDS "${_OUT}" "${_HEADER}")

        set(${GENGETOPT_TARGET_NAME}_SRC ${${GENGETOPT_TARGET_NAME}_SRC} ${_OUT})
        get_filename_component(_GENGETOPT_INCLUDE_DIR ${_HEADER} DIRECTORY)
        set(${GENGETOPT_TARGET_NAME}_INCLUDE_DIR ${${GENGETOPT_TARGET_NAME}_INCLUDE_DIR} ${_GENGETOPT_INCLUDE_DIR})
    endforeach(_CURRENT_FILE)
endmacro(add_gengetopt_files)
