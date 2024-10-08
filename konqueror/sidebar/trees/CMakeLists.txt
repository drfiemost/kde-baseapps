include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/legacy
)

set(libkonq_sidebar_tree_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartree.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreeitem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreetoplevelitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/k3listview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/k3listviewsearchline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/k3urldrag.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/q3dragobject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/q3header.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/q3listview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy/q3scrollview.cpp
)

remove_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

add_subdirectory( init )
add_subdirectory( dirtree_module )
add_subdirectory( bookmark_module )

########### next target ###############

set(konqsidebar_tree_PART_SRCS konqsidebar_oldtreemodule.cpp ${libkonq_sidebar_tree_SRCS})

kde4_add_plugin(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})

target_link_libraries(konqsidebar_tree ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS} konq konqsidebarplugin )

install(TARGETS konqsidebar_tree DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install desktop files ###############

install( FILES konqsidebar_tree.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/plugins )

