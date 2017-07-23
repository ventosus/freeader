cmake_minimum_required(VERSION 2.8)

project(freeader)

set(CMAKE_C_FLAGS "-fdata-sections -ffunction-sections ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-std=gnu11 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wno-attributes ${CMAKE_C_FLAGS}")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,defs ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete ${CMAKE_MODULE_LINKER_FLAGS}")
elseif(WIN32)
	set(CMAKE_C_FLAGS "-mstackrealign ${CMAKE_C_FLAGS}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	if(APPLE)
		set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-dead_strip ${CMAKE_MODULE_LINKER_FLAGS}")
	else()
		set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-s ${CMAKE_MODULE_LINKER_FLAGS}")
	endif()
endif()

add_definitions("-D_GNU_SOURCE=1") # asprintf

set(LIBS "")
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/..)
include_directories(${PROJECT_SOURCE_DIR}/../pugl)

# paths
set(FREEADER_BIN_DIR bin)
set(FREEADER_DATA_DIR share/freeader)
add_definitions("-DFREEADER_BIN_DIR=\"${CMAKE_INSTALL_PREFIX}/${FREEADER_BIN_DIR}/\"")
add_definitions("-DFREEADER_DATA_DIR=\"${CMAKE_INSTALL_PREFIX}/${FREEADER_DATA_DIR}/\"")

# pkg-config
find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

# LV2
pkg_search_module(LV2 REQUIRED lv2)
include_directories(${LV2_INCLUDE_DIRS})

# metadata
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("jack/metadata.h" JACK_HAS_METADATA_API)
if(${JACK_HAS_METADATA_API})
	add_definitions("-DJACK_HAS_METADATA_API")
endif()

# port_rename_callback
include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(jack jack_set_port_rename_callback "" JACK_HAS_PORT_RENAME_CALLBACK)
if(${JACK_HAS_PORT_RENAME_CALLBACK})
	add_definitions("-DJACK_HAS_PORT_RENAME_CALLBACK")
endif()

set(LIBS ${LIBS} m jbig85)
add_definitions("-DPUGL_HAVE_GL")

find_package(OpenGL)
if(${OPENGL_FOUND})
	set(UI_LIBS ${UI_LIBS} ${OPENGL_LIBRARIES})
else() # try pkg-config
	pkg_search_module(GL REQUIRED gl)
	if(${GL_FOUND})
		set(UI_LIBS ${UI_LIBS} ${GL_LDFLAGS})
	else()
		message(FATAL_ERROR "OpenGL not found")
	endif()
endif()
add_definitions("-DPUGL_HAVE_GL")

if(WIN32)
	find_library(GDI32_LIBRARY NAMES gdi32)
	if(GDI32_LIBRARY)
		set(UI_LIBS ${UI_LIBS} ${GDI32_LIBRARY})
	else()
		message(FATAL_ERROR "gdi32 library not found")
	endif()

	find_library(USER32_LIBRARY NAMES user32)
	if(USER32_LIBRARY)
		set(UI_LIBS ${UI_LIBS} ${USER32_LIBRARY})
	else()
		message(FATAL_ERROR "user32 library not found")
	endif()

	set(TAR_UI ${TAR_UI} pugl/pugl/pugl_win.cpp)

elseif(APPLE)
	find_library(COCOA_LIBRARY NAMES Cocoa)
	if(COCOA_LIBRARY)
		set(UI_LIBS ${UI_LIBS} ${COCOA_LIBRARY})
	else()
		message(FATAL_ERROR "Cocoa framework not found")
	endif()

	set(TAR_UI ${TAR_UI} pugl/pugl/pugl_osx.m)

else() # GNU/Linux
	pkg_search_module(X11 REQUIRED x11>=1.6)
	include_directories(${X11_INCLUDE_DIRS})
	set(UI_LIBS ${UI_LIBS} ${X11_LDFLAGS})

	pkg_search_module(XEXT REQUIRED xext>=1.3)
	include_directories(${XEXT_INCLUDE_DIRS})
	set(UI_LIBS ${UI_LIBS} ${XEXT_LDFLAGS})

	set(TAR_UI ${TAR_UI} ../pugl/pugl/pugl_x11.c)
endif()

# binary
add_executable(freeader_emu
	freeader_emu.c
	${TAR_UI})
target_link_libraries(freeader_emu ${LIBS} ${UI_LIBS})
install(TARGETS freeader_emu DESTINATION ${FREEADER_BIN_DIR})

# font
install(FILES ${PROJECT_SOURCE_DIR}/../nuklear/extra_font/Cousine-Regular.ttf DESTINATION ${FREEADER_DATA_DIR})
