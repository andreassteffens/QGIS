
########################################################
# Files

SET (sbaddressservices_SRCS
     sbaddressservicesplugin.cpp
     sbaddressservicesgui.cpp
     sbaddressservicesmaptool.cpp
     ${CMAKE_SOURCE_DIR}/external/SimpleCrypt/SimpleCrypt.cpp
)

SET (sbaddressservices_UIS
     sbaddressservicesguibase.ui
)

SET (sbaddressservices_HDRS
     ${CMAKE_SOURCE_DIR}/external/SimpleCrypt/SimpleCrypt.h
)


SET (sbaddressservices_MOC_HDRS
     sbaddressservicesplugin.h
     sbaddressservicesgui.h
     sbaddressservicesmaptool.h
)

SET (sbaddressservices_RCCS  sbaddressservicesplugin.qrc)

########################################################
# Build

QT5_WRAP_UI (sbaddressservices_UIS_H  ${sbaddressservices_UIS})

QT5_WRAP_CPP (sbaddressservices_MOC_SRCS  ${sbaddressservices_MOC_HDRS})

QT5_ADD_RESOURCES(sbaddressservices_RCC_SRCS ${sbaddressservices_RCCS})

ADD_LIBRARY (plugin_sbaddressservices MODULE ${sbaddressservices_SRCS} ${sbaddressservices_HDRS} ${sbaddressservices_MOC_SRCS} ${sbaddressservices_RCC_SRCS} ${sbaddressservices_UIS_H})

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
  ${CMAKE_SOURCE_DIR}/external/SimpleCrypt
)

TARGET_LINK_LIBRARIES(plugin_sbaddressservices
  qgis_core
  qgis_gui
)


########################################################
# Install

INSTALL(TARGETS plugin_sbaddressservices
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

