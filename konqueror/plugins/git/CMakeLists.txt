project(fileviewgitplugin)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkonq
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QIMAGEBLITZ_INCLUDES}
)

set(fileviewgitplugin_SRCS
    fileviewgitplugin.cpp
    checkoutdialog.cpp
    commitdialog.cpp
    tagdialog.cpp
    pushdialog.cpp
    pulldialog.cpp
    gitwrapper.cpp
)

kde4_add_kcfg_files(fileviewgitplugin_SRCS fileviewgitpluginsettings.kcfgc)
kde4_add_plugin(fileviewgitplugin  ${fileviewgitplugin_SRCS})
target_link_libraries(fileviewgitplugin ${KDE4_KIO_LIBS} konq)
install(FILES fileviewgitplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES fileviewgitpluginsettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(TARGETS fileviewgitplugin DESTINATION ${PLUGIN_INSTALL_DIR})
