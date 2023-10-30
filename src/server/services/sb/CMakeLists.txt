
########################################################
# Files

SET (sb_SRCS
  qgssb.cpp
  qgssbgetserverinfo.cpp
  qgssbconvertproject.cpp
  qgssbgetlayermetadata.cpp
  qgssbgetencryptedpath.cpp
  qgssbpurgecache.cpp
  qgssbsetpreloadprojects.cpp
  qgssbsetunloadprojects.cpp
  qgssbgetserverprocessid.cpp
  ${CMAKE_SOURCE_DIR}/external/SimpleCrypt/SimpleCrypt.cpp
)

########################################################
# Build

ADD_LIBRARY (sb MODULE ${sb_SRCS})

INCLUDE_DIRECTORIES(SYSTEM
  ${GDAL_INCLUDE_DIR}
)

INCLUDE_DIRECTORIES(
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
  ${CMAKE_SOURCE_DIR}/external
  ../..
  ..
  .
  
  ${CMAKE_BINARY_DIR}/src/core
  ${CMAKE_BINARY_DIR}/src/gui
  ${CMAKE_BINARY_DIR}/src/python
  ${CMAKE_BINARY_DIR}/src/analysis
  ${CMAKE_BINARY_DIR}/src/server
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/external/SimpleCrypt
)

TARGET_LINK_LIBRARIES(sb
  qgis_core
  qgis_server
)


########################################################
# Install

INSTALL(TARGETS sb
    RUNTIME DESTINATION ${QGIS_SERVER_MODULE_DIR}
    LIBRARY DESTINATION ${QGIS_SERVER_MODULE_DIR}
)

