set(konq_aboutpage_PART_SRCS konq_aboutpage.cc)

kde4_add_plugin(konq_aboutpage ${konq_aboutpage_PART_SRCS})

target_link_libraries(konq_aboutpage ${KDE4_KHTML_LIBS} Qt4::QtGui)

install(TARGETS konq_aboutpage DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES
    box-centre-konqueror.png
    top-left-konqueror.png
    lightning.png
    launch.html
    intro.html
    specs.html
    tips.html
    plugins.html
    plugins_rtl.html
    konq.css
    DESTINATION ${DATA_INSTALL_DIR}/konqueror/about)
install(FILES konqaboutpage.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES konq_aboutpage.desktop DESTINATION ${SERVICES_INSTALL_DIR})
