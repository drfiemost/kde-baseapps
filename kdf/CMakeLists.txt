
add_subdirectory(doc)
add_subdirectory(pics)

########### next target ###############
set(libkdf_common_SRCS 
	kdfwidget.cpp 
	kdfconfig.cpp 
	kdfitemdelegate.cpp
        kdfsortproxymodel.cpp
	mntconfig.cpp 
	disklist.cpp
	disks.cpp 
	optiondialog.cpp 
	stdoption.cpp )

kde4_add_ui_files(libkdf_common_SRCS
    kdfconfig.ui
    mntconfig.ui )

set(kdf_SRCS kdf.cpp ${libkdf_common_SRCS} )

#
#macro_bool_to_01(${CMAKE_SYSTEM_NAME} MATCHES "Linux" HAVE_LINUX)
#if ( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
#
#endif ( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
#
#if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
#
#endif ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
#

kde4_add_executable(kdf ${kdf_SRCS})

target_link_libraries(kdf ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kdf  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############

set(kwikdisk_SRCS kwikdisk.cpp ${libkdf_common_SRCS})

kde4_add_executable(kwikdisk ${kwikdisk_SRCS})

target_link_libraries(kwikdisk ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kwikdisk  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############

set(kcm_kdf_PART_SRCS kcmdf.cpp ${libkdf_common_SRCS})

kde4_add_plugin(kcm_kdf ${kcm_kdf_PART_SRCS})

target_link_libraries(kcm_kdf ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kcm_kdf  DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES kcmdf.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )

########### install files ###############

install( PROGRAMS kdf.desktop kwikdisk.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kdfui.rc  DESTINATION ${DATA_INSTALL_DIR}/kdf)
