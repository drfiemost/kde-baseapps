project(fileviewbazaarplugin)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkonq
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set(fileviewbazaarplugin_SRCS fileviewbazaarplugin.cpp)
kde4_add_plugin(fileviewbazaarplugin  ${fileviewbazaarplugin_SRCS})
target_link_libraries(fileviewbazaarplugin ${KDE4_KIO_LIBS} konq)
install(FILES fileviewbazaarplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS fileviewbazaarplugin DESTINATION ${PLUGIN_INSTALL_DIR})
