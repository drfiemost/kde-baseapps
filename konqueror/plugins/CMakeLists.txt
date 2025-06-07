project(konq-plugins)

add_subdirectory( khtmlsettingsplugin )
add_subdirectory( kimgalleryplugin )
add_subdirectory( dirfilter )
add_subdirectory( uachanger )
add_subdirectory( babelfish )
add_subdirectory( validators )
add_subdirectory( domtreeviewer )
add_subdirectory( webarchiver )

macro_optional_add_subdirectory(svn)
macro_optional_add_subdirectory(git)
macro_optional_add_subdirectory(dropbox)

add_subdirectory( kttsplugin )

if(UNIX)
      add_subdirectory( shellcmdplugin )
endif(UNIX)

add_subdirectory( imagerotation )
add_subdirectory( minitools )
add_subdirectory( autorefresh )
add_subdirectory( fsview )
add_subdirectory( searchbar )
add_subdirectory( rellinks )

add_subdirectory( adblock )

add_subdirectory( akregator )
