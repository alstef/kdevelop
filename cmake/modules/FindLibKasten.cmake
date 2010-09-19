# - Find Kasten library
#
# This module defines
#  LIBKASTEN_FOUND - whether the libkasten libraries were found
#  LIBKASTEN_LIBRARIES - the libkasten libraries
#  LIBKASTEN_INCLUDE_DIR - the include path of the libkasten libraries


if( LIBKASTEN_INCLUDE_DIR AND LIBKASTEN_LIBRARIES )
    # Already in cache, be silent
    set( Kasten_FIND_QUIETLY TRUE )
endif( LIBKASTEN_INCLUDE_DIR AND LIBKASTEN_LIBRARIES )


find_library( LIBKASTENCORE_LIBRARY
  NAMES
  kastencore
  HINTS
  ${LIB_INSTALL_DIR}
  ${KDE4_LIB_DIR}
)

find_library( LIBKASTENGUI_LIBRARY
  NAMES
  kastengui
  HINTS
  ${LIB_INSTALL_DIR}
  ${KDE4_LIB_DIR}
)

find_library( LIBKASTENCONTROLLERS_LIBRARY
  NAMES
  kastencontrollers
  HINTS
  ${LIB_INSTALL_DIR}
  ${KDE4_LIB_DIR}
)

set( LIBKASTEN_LIBRARIES
  ${LIBKASTENCORE_LIBRARY}
  ${LIBKASTENGUI_LIBRARY}
  ${LIBKASTENCONTROLLERS_LIBRARY}
)


find_path( LIBKASTEN_INCLUDE_DIR
  NAMES
  abstractmodel.h
  PATH_SUFFIXES
  kasten
  HINTS
  ${INCLUDE_INSTALL_DIR}
  ${KDE4_INCLUDE_DIR}
)

if( LIBKASTEN_INCLUDE_DIR AND LIBKASTEN_LIBRARIES )
   set( LIBKASTEN_FOUND  TRUE )
else( LIBKASTEN_INCLUDE_DIR AND LIBKASTEN_LIBRARIES )
   set( LIBKASTEN_FOUND  FALSE )
endif( LIBKASTEN_INCLUDE_DIR AND LIBKASTEN_LIBRARIES )


if( LIBKASTEN_FOUND )
   if( NOT Kasten_FIND_QUIETLY )
      message( STATUS "Found Kasten libraries: ${LIBKASTEN_LIBRARIES}" )
   endif( NOT Kasten_FIND_QUIETLY )
else( LIBKASTEN_FOUND )
   if( LibKasten_FIND_REQUIRED )
      message( FATAL_ERROR "Could not find Kasten libraries" )
   endif( LibKasten_FIND_REQUIRED )
endif( LIBKASTEN_FOUND )

mark_as_advanced( LIBKASTEN_INCLUDE_DIR LIBKASTEN_LIBRARIES )