set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BUILD_DIR}/.. ${KDE4_INCLUDES} )

# needed on windows to correctly use the files from dolphinprivate4
add_definitions(-DLIBDOLPHINPRIVATE_EXPORT=)

# KItemSetTest
set(kitemsettest_SRCS
    kitemsettest.cpp
    ../kitemviews/kitemset.cpp
)
kde4_add_unit_test(kitemsettest ${kitemsettest_SRCS})
target_link_libraries(kitemsettest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KItemRangeTest
set(kitemrangetest_SRCS
    kitemrangetest.cpp
)
kde4_add_unit_test(kitemrangetest ${kitemrangetest_SRCS})
target_link_libraries(kitemrangetest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KItemListSelectionManagerTest
set(kitemlistselectionmanagertest_SRCS
    kitemlistselectionmanagertest.cpp
    ../kitemviews/kitemlistselectionmanager.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemset.cpp
)
kde4_add_unit_test(kitemlistselectionmanagertest ${kitemlistselectionmanagertest_SRCS})
target_link_libraries(kitemlistselectionmanagertest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KItemListControllerTest
set(kitemlistcontrollertest_SRCS
    kitemlistcontrollertest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kfileitemlistview.cpp
    ../kitemviews/kfileitemlistwidget.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemlistview.cpp
    ../kitemviews/kitemlistcontainer.cpp
    ../kitemviews/kitemlistwidget.cpp
    ../kitemviews/kitemlistviewaccessible.cpp
    ../kitemviews/kitemset.cpp
    ../kitemviews/kstandarditemlistview.cpp
    ../kitemviews/kstandarditemlistwidget.cpp
)
kde4_add_unit_test(kitemlistcontrollertest ${kitemlistcontrollertest_SRCS})
target_link_libraries(kitemlistcontrollertest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KFileItemListViewTest
set(kfileitemlistviewtest_SRCS
    kfileitemlistviewtest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kfileitemlistview.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemlistview.cpp
    ../kitemviews/kitemlistviewaccessible.cpp
    ../kitemviews/kitemlistcontainer.cpp
    ../kitemviews/kitemlistwidget.cpp
    ../kitemviews/kitemset.cpp
    ../kitemviews/kstandarditemlistview.cpp
    ../kitemviews/kstandarditemlistwidget.cpp
)
kde4_add_unit_test(kfileitemlistviewtest ${kfileitemlistviewtest_SRCS})
target_link_libraries(kfileitemlistviewtest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KFileItemModelTest
set(kfileitemmodeltest_SRCS
    kfileitemmodeltest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemset.cpp
)
kde4_add_unit_test(kfileitemmodeltest ${kfileitemmodeltest_SRCS})
target_link_libraries(kfileitemmodeltest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KFileItemModelBenchmark
set(kfileitemmodelbenchmark_SRCS
    kfileitemmodelbenchmark.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
)
kde4_add_executable(kfileitemmodelbenchmark ${kfileitemmodelbenchmark_SRCS})
target_link_libraries(kfileitemmodelbenchmark dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# KItemListKeyboardSearchManagerTest
set(kitemlistkeyboardsearchmanagertest_SRCS
    kitemlistkeyboardsearchmanagertest.cpp
    ../kitemviews/private/kitemlistkeyboardsearchmanager.cpp
)
kde4_add_unit_test(kitemlistkeyboardsearchmanagertest ${kitemlistkeyboardsearchmanagertest_SRCS})
target_link_libraries(kitemlistkeyboardsearchmanagertest ${KDE4_KIO_LIBS} Qt4::QtTest)

# KStandardItemModelTest
set(kstandarditemmodeltest_SRCS
    kstandarditemmodeltest.cpp
    ../kitemviews/kstandarditem.cpp
    ../kitemviews/kstandarditemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
)
kde4_add_unit_test(kstandarditemmodeltest ${kstandarditemmodeltest_SRCS})
target_link_libraries(kstandarditemmodeltest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)

# ViewPropertiesTest
set(viewpropertiestest_SRCS
    viewpropertiestest.cpp
    testdir.cpp
    ../views/viewproperties.cpp
)
kde4_add_kcfg_files(viewpropertiestest_SRCS
  ../settings/dolphin_generalsettings.kcfgc
  ../settings/dolphin_directoryviewpropertysettings.kcfgc
)
kde4_add_unit_test(viewpropertiestest ${viewpropertiestest_SRCS})
target_link_libraries(viewpropertiestest dolphinprivate4 ${KDE4_KIO_LIBS} Qt4::QtTest)
