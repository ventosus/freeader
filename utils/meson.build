project('freeader', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=true',
	'c_std=c11'])

add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-DPUGL_HAVE_GL', language : 'c')

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
jbig85_dep = cc.find_library('jbig85')
lv2_dep = dependency('lv2', version : '>=1.14.0')
png_dep = dependency('libpng')
cairo_dep = dependency('cairo')

ui_deps = [lv2_dep, m_dep, jbig85_dep]

root_inc = include_directories('..')
pugl_inc = include_directories(join_paths('..', 'pugl'))

incs = [root_inc, pugl_inc]

c_args = ['-fvisibility=hidden',
	'-ffast-math',
	'-Wno-misleading-indentation',
	'-Wno-unused-function']

ui_srcs = ['freeader_emu.c']

if host_machine.system() == 'windows'
	add_languages('cpp')
	ui_deps += cc.find_library('opengl32')
	ui_deps += cc.find_library('gdi32')
	ui_deps += cc.find_library('user32')
	ui_srcs += '../pugl/pugl/pugl_win.cpp'
elif host_machine.system() == 'darwin'
	#add_languages('objc')
	#ui_deps += cc.find_library('Cocoa')
	#ui_deps += cc.find_library('gl')
	#ui_deps += dependency('appleframeworks', modules : 'cocoa')
	#ui_srcs += '../pugl/pugl/pugl_osx.m'
else
	ui_deps += dependency('gl')
	ui_deps += dependency('x11', version : '>=1.6.0')
	ui_deps += dependency('xext', version : '>=1.3.0')
	ui_srcs += '../pugl/pugl/pugl_x11.c'
endif

executable('freeader_emu', ui_srcs,
	c_args : c_args,
	dependencies : [ui_deps],
	include_directories : incs,
	install : true)

executable('freeader_enc', 'freeader_enc.c',
	c_args : c_args,
	dependencies : [m_dep, jbig85_dep, png_dep],
	install : true)

executable('freeader_toc', 'freeader_toc.c',
	c_args : c_args,
	dependencies : [cairo_dep],
	install : true)

#install_man('freeader_enc.1')
#install_man('freeader_emu.1')
#install_man('freeader_toc.1')
