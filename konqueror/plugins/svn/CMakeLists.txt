project(fileviewsvnplugin)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkonq
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QIMAGEBLITZ_INCLUDES}
)

set(fileviewsvnplugin_SRCS fileviewsvnplugin.cpp)
kde4_add_kcfg_files(fileviewsvnplugin_SRCS fileviewsvnpluginsettings.kcfgc)
kde4_add_plugin(fileviewsvnplugin  ${fileviewsvnplugin_SRCS})
target_link_libraries(fileviewsvnplugin ${KDE4_KIO_LIBS} konq)
install(FILES fileviewsvnplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES fileviewsvnpluginsettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(TARGETS fileviewsvnplugin DESTINATION ${PLUGIN_INSTALL_DIR})
