
########################################################
# Files

SET (SBMG_SRCS
  qgssbmapguiderestprovider.cpp
)

SET (SBMG_MOC_HDRS
	qgssbmapguiderestprovider.h
)

########################################################
# Build

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/core/geometry
  ${CMAKE_SOURCE_DIR}/src/core/metadata
  ${CMAKE_SOURCE_DIR}/src/core/expression
  ${CMAKE_SOURCE_DIR}/external
  ${CMAKE_SOURCE_DIR}/external/nlohmann

  ${CMAKE_BINARY_DIR}/src/core
  ${CMAKE_BINARY_DIR}/src/gui
)
INCLUDE_DIRECTORIES (SYSTEM
  ${EXPAT_INCLUDE_DIR}
)

QT5_WRAP_CPP(SBMG_MOC_SRCS ${SBMG_MOC_HDRS})

ADD_LIBRARY(sbmapguiderestprovider MODULE ${SBMG_SRCS} ${SBMG_MOC_SRCS})

TARGET_LINK_LIBRARIES(sbmapguiderestprovider
  ${EXPAT_LIBRARY}
  qgis_core
)


########################################################
# Install

INSTALL (TARGETS sbmapguiderestprovider
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
