set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### konqmimedatatest ###############

kde4_add_unit_test(konqmimedatatest konqmimedatatest.cpp)

target_link_libraries(konqmimedatatest konq ${KDE4_KDECORE_LIBRARY} Qt4::QtGui Qt4::QtCore
                      Qt4::QtTest)

########### konqpopupmenutest ###############

kde4_add_unit_test(konqpopupmenutest konqpopupmenutest.cpp)

target_link_libraries(konqpopupmenutest konq ${KDE4_KFILE_LIBS}
                     Qt4::QtCore Qt4::QtGui Qt4::QtTest)

########### favicontest ###############

set(favicontest_SRCS favicontest.cpp)
qt4_add_dbus_interface(favicontest_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../favicons/org.kde.FavIcon.xml favicon_interface)
kde4_add_unit_test(favicontest ${favicontest_SRCS})

target_link_libraries(favicontest konq ${KDE4_KDECORE_LIBRARY} ${KDE4_KIO_LIBRARY} Qt4::QtCore
                      Qt4::QtGui Qt4::QtDBus Qt4::QtTest)

############################################
