# to compile the plugin out of
# kopete source tree
if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
  find_package(KDE4 REQUIRED)
  find_package(Qt4 REQUIRED)
  find_package(OpenSSL REQUIRED)
  INCLUDE_DIRECTORIES(${QT4_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${QT_QTCORE_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${QT_QTGUI_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${QT_QTNETWORK_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${KDE4_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${KDE4_INCLUDES})

  include(KDE4Defaults)
  include(MacroLibrary)

  find_path(KOPETE_INCLUDE_DIR
    NAME kopeteversion.h
    PATH_SUFFIXES kopete
    HINTS
    ${INCLUDE_INSTALL_DIR}
    ${KDE4_INCLUDE_DIR}
  )

  if(KOPETE_INCLUDE_DIR)
    MESSAGE("Found Kopete: includes - ${KOPETE_INCLUDE_DIR}")
  else(KOPETE_INCLUDE_DIR)
    MESSAGE(FATAL_ERROR "Kopete headers not found")
  endif(KOPETE_INCLUDE_DIR)

  mark_as_advanced(KOPETE_INCLUDE_DIR)
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

# add_subdirectory( icons )

include_directories(${KOPETE_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})

set(kopete_fetion_PART_SRCS
    fetionaccount.cpp
    fetionaddcontactpage.cpp
    fetioncontact.cpp
    fetioneditaccountwidget.cpp
    fetionprotocol.cpp
    fetionsession.cpp
    fetionsipevent.cpp
    fetionsipnotifier.cpp
    fetionsiputils.cpp
    fetionvcodedialog.cpp
)

kde4_add_plugin(kopete_fetion ${kopete_fetion_PART_SRCS})

target_link_libraries(kopete_fetion ${KDE4_KIO_LIBS} kopete ${QT_QTNETWORK_LIBRARY} crypto)

install(TARGETS kopete_fetion DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES kopete_fetion.desktop DESTINATION ${SERVICES_INSTALL_DIR})
