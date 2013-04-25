#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

#Configure odversion.h
configure_file ( ${OpendTect_DIR}/include/Basic/odversion.h.in ${OpendTect_DIR}/include/Basic/odversion.h )

file(GLOB CMAKE_FILES CMakeModules/*.cmake )
file(GLOB TEMPLATE_FILES CMakeModules/templates/*.in )
set( CMAKE_FILES ${CMAKE_FILES} ${TEMPLATE_FILES} )
OD_ADD_SOURCE_FILES( ${CMAKE_FILES} )

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN ".svn" EXCLUDE )

#install doc stuff
install( DIRECTORY doc/Programmer/pluginexample
	 DESTINATION doc/Programmer
	 PATTERN ".svn" EXCLUDE )

install( DIRECTORY doc/Programmer/batchprogexample
	 DESTINATION doc/Programmer
	 PATTERN ".svn" EXCLUDE )

install( DIRECTORY doc/Credits/base
	 DESTINATION doc/Credits
	 PATTERN ".svn" EXCLUDE )

file( GLOB FLEXNETFILES doc/*.html )
foreach( FLEXNETFILE ${FLEXNETFILES} )
    install( FILES ${FLEXNETFILE} DESTINATION doc )
endforeach()

install( DIRECTORY doc/Scripts
	 DESTINATION doc
	 PATTERN ".svn" EXCLUDE )

#Install data
install ( DIRECTORY "data" DESTINATION .
	  PATTERN "install_files" EXCLUDE
	  PATTERN ".svn" EXCLUDE )

#install alo files
install ( DIRECTORY plugins/${OD_PLFSUBDIR}
	  DESTINATION plugins )

install( DIRECTORY ${CMAKE_SOURCE_DIR}/relinfo
	 DESTINATION .
	 PATTERN ".svn" EXCLUDE )

install( FILES CMakeLists.txt DESTINATION . )

if( WIN32 )
    install( DIRECTORY bin/win32/rsm
	     DESTINATION .
	     PATTERN ".svn" EXCLUDE )
endif()

file( GLOB TEXTFILES ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/*.txt )
if( UNIX OR APPLE )
    file( GLOB PROGRAMS ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/* )
    list( REMOVE_ITEM PROGRAMS ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/.svn )
    foreach( TEXTFILE ${TEXTFILES} )
        list( REMOVE_ITEM PROGRAMS ${TEXTFILE} )
    endforeach()

    install ( PROGRAMS ${PROGRAMS} DESTINATION . )
endif()
install ( FILES ${TEXTFILES} DESTINATION . )

if( APPLE )
    install( PROGRAMS data/install_files/macscripts/chfwscript
	     DESTINATION ${OD_EXEC_OUTPUT_RELPATH} )

    install( DIRECTORY data/install_files/macscripts/Contents
	     DESTINATION .
	     PATTERN ".svn" EXCLUDE )
endif( APPLE )

set( QJPEG ${QT_QJPEG_PLUGIN_RELEASE} )
set( LMHOSTID lmhostid )
if( WIN32 )
    install( PROGRAMS "C:/Program\ Files \(x86\)/Microsoft\ SDKs/Windows/v7.0A/Bin/signtool.exe" 
	     DESTINATION ${OD_EXEC_OUTPUT_RELPATH}/${CMAKE_BUILD_TYPE} )
    set( QJPEG ${QTDIR}/plugins/imageformats/qjpeg4.dll )
    if( ${OD_PLFSUBDIR} STREQUAL "win32" )
        set( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x86/Microsoft.VC100.CRT" )
    elseif( ${OD_PLFSUBDIR} STREQUAL "win64" )
        set( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x64/Microsoft.VC100.CRT" )
    endif()
    set( LMHOSTID "lmhostid.exe" )
endif()

install( PROGRAMS ${QJPEG} DESTINATION imageformats )
if ( WIN32 )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_OUTPUT_RELPATH}/${CMAKE_BUILD_TYPE} )
else()
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_OUTPUT_RELPATH} )
endif()
if( EXISTS ${MSVCPATH} )
    file( GLOB MSVCDLLS ${MSVCPATH}/*.dll )
    foreach( DLL ${MSVCDLLS} )
	install( FILES ${DLL} DESTINATION bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/thirdpartylibs )
    endforeach()
endif()

FILE( GLOB SCRIPTS ${CMAKE_SOURCE_DIR}/bin/od_* )
install( PROGRAMS ${SCRIPTS} DESTINATION bin )
install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/mksethdir DESTINATION bin )
install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/mac_term DESTINATION bin )
install( FILES ${CMAKE_SOURCE_DIR}/bin/macterm.in DESTINATION bin )

add_custom_target( sources ${CMAKE_COMMAND}
		   -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
		   -P ${CMAKE_SOURCE_DIR}/CMakeModules/ODInstallSources.cmake
		   COMMENT "Installing sources" )

add_custom_target( docpackages ${CMAKE_COMMAND}
        -DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR}
        -DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR}
        -DOpendTect_VERSION_DETAIL=${OpendTect_VERSION_DETAIL}
        -DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DOD_PLFSUBDIR=${OD_PLFSUBDIR}
        -DPSD=${PROJECT_SOURCE_DIR}
        -P ${PROJECT_SOURCE_DIR}/CMakeModules/packagescripts/ODMakeDocPackages.cmake
         COMMENT "Preparing doc packages" )
include ( ODSubversion )
