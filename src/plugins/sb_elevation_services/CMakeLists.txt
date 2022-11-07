
########################################################
# Files

SET (sbelevationservices_SRCS
     sbelevationservicesplugin.cpp
     sbelevationservicesgui.cpp
     sbelevationservicesmaptool.cpp
)

SET (sbelevationservices_UIS
     sbelevationservicesguibase.ui
)

SET (sbelevationservices_MOC_HDRS
     sbelevationservicesplugin.h
     sbelevationservicesgui.h
     sbelevationservicesmaptool.h
)

SET (sbelevationservices_RCCS  sbelevationservicesplugin.qrc)

########################################################
# Build

QT5_WRAP_UI (sbelevationservices_UIS_H  ${sbelevationservices_UIS})

QT5_WRAP_CPP (sbelevationservices_MOC_SRCS  ${sbelevationservices_MOC_HDRS})

QT5_ADD_RESOURCES(sbelevationservices_RCC_SRCS ${sbelevationservices_RCCS})

ADD_LIBRARY (plugin_sbelevationservices MODULE ${sbelevationservices_SRCS} ${sbelevationservices_MOC_SRCS} ${sbelevationservices_RCC_SRCS} ${sbelevationservices_UIS_H})

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/core/expression
  ${CMAKE_SOURCE_DIR}/src/core/geometry
  ${CMAKE_SOURCE_DIR}/src/core/metadata
  ${CMAKE_SOURCE_DIR}/src/core/raster
  ${CMAKE_SOURCE_DIR}/src/core/layertree
  ${CMAKE_SOURCE_DIR}/src/core/labeling
  ${CMAKE_SOURCE_DIR}/src/core/symbology
  ${CMAKE_SOURCE_DIR}/src/gui
  ${CMAKE_SOURCE_DIR}/src/plugins
  ${CMAKE_SOURCE_DIR}/external
  ${CMAKE_SOURCE_DIR}/external/nlohmann

  ${CMAKE_BINARY_DIR}/src/core
  ${CMAKE_BINARY_DIR}/src/gui
  ${CMAKE_BINARY_DIR}/src/ui
  ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(plugin_sbelevationservices
  qgis_core
  qgis_gui
)


########################################################
# Install

INSTALL(TARGETS plugin_sbelevationservices
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

