# If protobuf is found - libprotobuf target is available

set(HAVE_PROTOBUF FALSE)

if(NOT WITH_PROTOBUF)
  return()
endif()

ocv_option(BUILD_PROTOBUF "Force to build libprotobuf runtime from sources" OFF)
ocv_option(PROTOBUF_UPDATE_FILES "Force rebuilding .proto files (protoc should be available)" ON)

# BUILD_PROTOBUF=OFF: Custom manual protobuf configuration (see find_package(Protobuf) for details):
# - Protobuf_INCLUDE_DIR
# - Protobuf_LIBRARY
# - Protobuf_PROTOC_EXECUTABLE

function(generate_protobuf_files PROTO_FILES_INPUT)
  set(PROTO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Protobuf")

  foreach(PROTO_FILE ${PROTO_FILES_INPUT})
    get_filename_component(PROTO_FILE_ABSOLUTE ${PROTO_FILE} ABSOLUTE)
    get_filename_component(PROTO_FILE_BASENAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_FILE_DIR ${PROTO_FILE} DIRECTORY)

    set(PB_SRC_OUT "${PROTO_OUTPUT_DIR}/${PROTO_FILE_BASENAME}.pb.cc")
    set(PB_HDR_OUT "${PROTO_OUTPUT_DIR}/${PROTO_FILE_BASENAME}.pb.h")

    list(APPEND fw_srcs "${PB_SRC_OUT}")
    list(APPEND fw_hdrs "${PB_HDR_OUT}")

    message("adding custom command for Generating ${PROTO_FILE_BASENAME}.pb.h/cc from ${PROTO_FILE}")

    add_custom_command(
      OUTPUT ${PB_SRC_OUT} ${PB_HDR_OUT}
      COMMAND protobuf::protoc --cpp_out "${PROTO_OUTPUT_DIR}" -I ${PROTO_FILE_DIR} ${PROTO_FILE_ABSOLUTE}
      COMMENT "Generating ${PROTO_FILE_BASENAME}.pb.h/cc from ${PROTO_FILE}"
      DEPENDS ${PROTO_FILE}
      VERBATIM
    )
  endforeach()

  if(NOT EXISTS "${PROTO_OUTPUT_DIR}")
    file(MAKE_DIRECTORY "${PROTO_OUTPUT_DIR}")
  endif()

  set(fw_srcs "${fw_srcs}" PARENT_SCOPE)
  set(fw_hdrs "${fw_hdrs}" PARENT_SCOPE)
endfunction()

function(get_protobuf_version version include)
  file(STRINGS "${include}/google/protobuf/stubs/common.h" ver REGEX "#define GOOGLE_PROTOBUF_VERSION [0-9]+")
  string(REGEX MATCHALL "[0-9]+" ver ${ver})
  math(EXPR major "${ver} / 1000000")
  math(EXPR minor "${ver} / 1000 % 1000")
  math(EXPR patch "${ver} % 1000")
  set(${version} "${major}.${minor}.${patch}" PARENT_SCOPE)
endfunction()

if(BUILD_PROTOBUF)
  ocv_assert(NOT PROTOBUF_UPDATE_FILES)
  add_subdirectory("${OpenCV_SOURCE_DIR}/3rdparty/protobuf")
  set(Protobuf_LIBRARIES "libprotobuf")
  set(HAVE_PROTOBUF TRUE)
else()
  unset(Protobuf_VERSION CACHE)

  if(IOS OR ANDROID)
    # add cmake/host subdiretcory as host project to install protoc
    include(hunter_experimental_add_host_project)
    hunter_experimental_add_host_project("${OpenCV_SOURCE_DIR}/cmake/protobuf-host")

    add_executable(protobuf::protoc IMPORTED)
    set_property(TARGET protobuf::protoc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(protobuf::protoc PROPERTIES IMPORTED_LOCATION_RELEASE "${HUNTER_HOST_ROOT}/bin/protoc")

    message(STATUS "Using imported protoc from host: ${HUNTER_HOST_ROOT}/bin/protoc")
  endif(IOS OR ANDROID)

  hunter_add_package(Protobuf)
  find_package(Protobuf CONFIG REQUIRED)

  # Backwards compatibility
  # Define camel case versions of input variables
  foreach(UPPER
      PROTOBUF_FOUND
      PROTOBUF_LIBRARY
      PROTOBUF_INCLUDE_DIR
      PROTOBUF_VERSION
      )
      if (DEFINED ${UPPER})
          string(REPLACE "PROTOBUF_" "Protobuf_" Camel ${UPPER})
          if (NOT DEFINED ${Camel})
              set(${Camel} ${${UPPER}})
          endif()
      endif()
  endforeach()
  # end of compatibility block

  if(Protobuf_FOUND)
    if(TARGET protobuf::libprotobuf)
      set(Protobuf_LIBRARIES "protobuf::libprotobuf")
    else()
      add_library(libprotobuf UNKNOWN IMPORTED)
      set_target_properties(libprotobuf PROPERTIES
        IMPORTED_LOCATION "${Protobuf_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Protobuf_INCLUDE_DIR}"
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${Protobuf_INCLUDE_DIR}"
      )
      get_protobuf_version(Protobuf_VERSION "${Protobuf_INCLUDE_DIR}")
      set(Protobuf_LIBRARIES "libprotobuf")
    endif()
    set(HAVE_PROTOBUF TRUE)
  endif()
endif()

# if(HAVE_PROTOBUF AND PROTOBUF_UPDATE_FILES AND NOT COMMAND PROTOBUF_GENERATE_CPP)
#   message(FATAL_ERROR "Can't configure protobuf dependency (BUILD_PROTOBUF=${BUILD_PROTOBUF} PROTOBUF_UPDATE_FILES=${PROTOBUF_UPDATE_FILES})")

#   if(NOT COMMAND PROTOBUF_GENERATE_CPP)
#     message(FATAL_ERROR "PROTOBUF_GENERATE_CPP command is not available")
#   endif()
# endif()

if(HAVE_PROTOBUF)
  list(APPEND CUSTOM_STATUS protobuf)
  if(NOT BUILD_PROTOBUF)
    if(TARGET "${Protobuf_LIBRARIES}")
      get_target_property(__location "${Protobuf_LIBRARIES}" IMPORTED_LOCATION_RELEASE)
      if(NOT __location)
        get_target_property(__location "${Protobuf_LIBRARIES}" IMPORTED_LOCATION)
      endif()
    elseif(Protobuf_LIBRARY)
      set(__location "${Protobuf_LIBRARY}")
    else()
      set(__location "${Protobuf_LIBRARIES}")
    endif()
  endif()
  list(APPEND CUSTOM_STATUS_protobuf "    Protobuf:"
    BUILD_PROTOBUF THEN "build (${Protobuf_VERSION})"
    ELSE "${__location} (${Protobuf_VERSION})")
endif()
