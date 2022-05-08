if (NOT WIN32)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

########### centralwidget ###############

kde4_add_executable(centralwidget TEST centralwidget.cpp)
target_link_libraries(centralwidget konq ${KDE4_KDECORE_LIBS} Qt4::QtCore Qt4::QtGui
                      Qt4::QtTest)

########### konqviewmgrtest ###############

kde4_add_unit_test(konqviewmgrtest TEST konqviewmgrtest.cpp)
target_link_libraries(konqviewmgrtest kdeinit_konqueror Qt4::QtCore Qt4::QtGui
                      ${KDE4_KHTML_LIBS} Qt4::QtTest)

########### historymanagertest ###############

kde4_add_unit_test(historymanagertest historymanagertest.cpp)
target_link_libraries(historymanagertest konq konquerorprivate ${KDE4_KDEUI_LIBS} Qt4::QtCore Qt4::QtTest)

########### undomanagertest ###############

kde4_add_unit_test(undomanagertest undomanagertest.cpp)
target_link_libraries(undomanagertest kdeinit_konqueror Qt4::QtCore Qt4::QtTest)

########### konqhtmltest ###############

kde4_add_unit_test(konqhtmltest konqhtmltest.cpp)
target_link_libraries(konqhtmltest kdeinit_konqueror ${KDE4_KHTML_LIBS} Qt4::QtCore Qt4::QtTest)

########### konqviewtest ###############

kde4_add_unit_test(konqviewtest konqviewtest.cpp)
target_link_libraries(konqviewtest kdeinit_konqueror Qt4::QtCore Qt4::QtTest)

endif (NOT WIN32)
