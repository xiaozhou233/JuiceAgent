
# Resolve project directory
get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(VERSION_FILE "${PROJECT_DIR}/Version.txt")
set(BIN_DIR "${PROJECT_DIR}/build/bin")
set(OUTPUT_DIR "${PROJECT_DIR}/dist")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

# Read version file
if(NOT EXISTS "${VERSION_FILE}")
    message(FATAL_ERROR "Version.txt not found: ${VERSION_FILE}")
endif()

file(STRINGS "${VERSION_FILE}" VERSION_LINES)

foreach(LINE IN LISTS VERSION_LINES)
    if(LINE MATCHES "^[ \t]*Major[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(VERSION_MAJOR "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^[ \t]*Minor[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(VERSION_MINOR "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^[ \t]*Patch[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(VERSION_PATCH "${CMAKE_MATCH_1}")
    endif()
endforeach()

if(NOT DEFINED VERSION_MAJOR OR
   NOT DEFINED VERSION_MINOR OR
   NOT DEFINED VERSION_PATCH)
    message(FATAL_ERROR "Invalid Version.txt: Major, Minor and Patch are required")
endif()


# Define package files
set(PACKAGE_FILES
    injector.exe
    libinject.dll
    libloader.dll
    libagent.dll
)

foreach(FILE_NAME IN LISTS PACKAGE_FILES)
    if(NOT EXISTS "${BIN_DIR}/${FILE_NAME}")
        message(FATAL_ERROR "Required file not found: ${BIN_DIR}/${FILE_NAME}")
    endif()
endforeach()

# Create package
set(OUTPUT_FILE
    "${OUTPUT_DIR}/JuiceAgent_v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}_x64.zip"
)

file(REMOVE "${OUTPUT_FILE}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar
        cf "${OUTPUT_FILE}"
        --format=zip
        ${PACKAGE_FILES}
    WORKING_DIRECTORY "${BIN_DIR}"
    RESULT_VARIABLE TAR_RESULT
)

if(NOT TAR_RESULT STREQUAL "0")
    message(FATAL_ERROR "Failed to create ZIP package: ${TAR_RESULT}")
endif()

message(STATUS "Package created: ${OUTPUT_FILE}")
