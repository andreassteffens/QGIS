
########################################################
# Files

set (sb_SRCS
  qgssb.cpp
  qgssbgetserverinfo.cpp
  qgssbconvertproject.cpp
  qgssbgetlayermetadata.cpp
  qgssbgetencryptedpath.cpp
  qgssbgetdecryptedpath.cpp
  qgssbpurgecache.cpp
  qgssbsetpreloadprojects.cpp
  qgssbsetunloadprojects.cpp
  qgssbgetserverprocessid.cpp
  ${CMAKE_SOURCE_DIR}/external/SimpleCrypt/SimpleCrypt.cpp
  ${CMAKE_SOURCE_DIR}/external/fpng/fpng.cpp
)

########################################################
# Build

set(_library_suffix_MODULE "")
set(_library_suffix_STATIC "_static")

foreach(_library_type MODULE STATIC)
  set(_library_name "sb${_library_suffix_${_library_type}}")

  add_library(${_library_name} ${_library_type} ${sb_SRCS})

  # require c++17
  target_compile_features(${_library_name} PRIVATE cxx_std_17)

  include_directories(${_library_name} SYSTEM PUBLIC
    ../..
    ..
    .
    ${GDAL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core/auth
    ${CMAKE_SOURCE_DIR}/src/providers/wms
    ${CMAKE_SOURCE_DIR}/external/nlohmann
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/annotations
    ${CMAKE_SOURCE_DIR}/src/core/expression
    ${CMAKE_SOURCE_DIR}/src/core/dxf
    ${CMAKE_SOURCE_DIR}/src/core/expression
    ${CMAKE_SOURCE_DIR}/src/core/effects
    ${CMAKE_SOURCE_DIR}/src/core/geometry
    ${CMAKE_SOURCE_DIR}/src/core/metadata
    ${CMAKE_SOURCE_DIR}/src/core/raster
    ${CMAKE_SOURCE_DIR}/src/core/symbology
    ${CMAKE_SOURCE_DIR}/src/core/layertree
    ${CMAKE_SOURCE_DIR}/src/core/layout
    ${CMAKE_SOURCE_DIR}/src/core/labeling
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/editorwidgets
    ${CMAKE_SOURCE_DIR}/src/gui/editorwidgets/core
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_BINARY_DIR}/src/core
    ${CMAKE_BINARY_DIR}/src/gui
    ${CMAKE_BINARY_DIR}/src/python
    ${CMAKE_BINARY_DIR}/src/analysis
    ${CMAKE_BINARY_DIR}/src/server
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/external/SimpleCrypt
  )

  target_link_libraries(${_library_name}
    qgis_core
    qgis_server
  )
endforeach()

# only install module, static library is for testing only
install(TARGETS sb
    RUNTIME DESTINATION ${QGIS_SERVER_MODULE_DIR}
    LIBRARY DESTINATION ${QGIS_SERVER_MODULE_DIR}
)

