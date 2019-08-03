project('d2tk', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=false',
	'b_lto=false',
	'c_std=gnu11'])

static_link = false #meson.is_cross_build()
build_debug = get_option('build-debug')

prefix = get_option('prefix')
datadir = get_option('datadir')
bindir = get_option('bindir')

pdatadir = join_paths(prefix, datadir, 'd2tk', '')

add_project_arguments('-DD2TK_DATA_DIR="'+pdatadir+'"', language : 'c')

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
thread_dep = dependency('threads')
freetype_dep = dependency('freetype2', version : '>=18.0.0',
	static : static_link, required: false)
pixman_dep = dependency('pixman-1', version : '>=0.34.0',
	static : static_link, required: false)
cairo_dep = dependency('cairo', version : '>=1.14.0',
	static : static_link, required: false)
evdev_dep = dependency('libevdev', version : '>=1.5.0',
	static : static_link, required: false)
input_dep = dependency('libinput', version : '>=1.6.0',
	static : static_link, required: false)
udev_dep = dependency('libudev', version : '>=232',
	static : static_link, required: false)

deps = [m_dep, evdev_dep]
links = []

pugl_inc = include_directories('pugl')
nanovg_inc = include_directories('nanovg/src')
glew_inc = include_directories('glew-2.1.0')
inc_dir = [pugl_inc, nanovg_inc, glew_inc]

rawvers = run_command('cat', 'VERSION').stdout().strip()
version = rawvers.split('.')

conf_data = configuration_data()
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-DPUGL_HAVE_GL', language : 'c')

if build_debug
	add_project_arguments('-DD2TK_DEBUG', language : 'c')
endif

lib_srcs = [
	join_paths('src', 'mum.c'),
	join_paths('src', 'core.c'),
	join_paths('src', 'base.c')
]

bin_srcs = [
	join_paths('example', 'example.c')
]

pugl_srcs = [
	join_paths('src', 'frontend_pugl.c')
]

pugl_bin_srcs = [
	join_paths('example', 'd2tk_pugl.c')
]

nanovg_srcs = [
	join_paths('nanovg', 'src', 'nanovg.c'),
	join_paths('src', 'backend_nanovg.c'),
	join_paths('glew-2.1.0', 'glew.c')
]

cairo_srcs = [
	join_paths('src', 'backend_cairo.c')
]

fbdev_srcs = [
	join_paths('src', 'frontend_fbdev.c')
]

fbdev_bin_srcs = [
	join_paths('example', 'd2tk_fbdev.c')
]

test_core_srcs = [
	join_paths('test', 'core.c'),
	join_paths('test', 'mock.c')
]

test_base_srcs = [
	join_paths('test', 'base.c'),
	join_paths('test', 'mock.c')
]

c_args = ['-fvisibility=hidden',
	'-ffast-math']

if host_machine.system() == 'windows'
	add_languages('cpp')
	add_project_arguments('-DGLEW_STATIC', language : 'c')
	deps += cc.find_library('opengl32')
	deps += cc.find_library('gdi32')
	deps += cc.find_library('ws2_32')
	pugl_srcs += 'pugl/pugl/pugl_win.cpp'
elif host_machine.system() == 'darwin'
	add_languages('objc')
	links += ['-framework', 'OpenGL']
	links += ['-framework', 'Cocoa']
	pugl_srcs += 'pugl/pugl/pugl_osx.m'
else
	deps += dependency('gl')
	deps += dependency('x11', version : '>=1.6.0')
	deps += dependency('xext', version : '>=1.3.0')
	pugl_srcs += 'pugl/pugl/pugl_x11.c'
endif

if freetype_dep.found() and pixman_dep.found() and cairo_dep.found() and (host_machine.system() == 'linux')
	d2tk_cairo = declare_dependency(
		compile_args : '-DPUGL_HAVE_CAIRO',
		include_directories : inc_dir,
		dependencies : [deps, freetype_dep, pixman_dep, cairo_dep],
		link_args : links,
		sources : [lib_srcs, cairo_srcs, pugl_srcs])

	executable('d2tk.cairo', [bin_srcs, pugl_bin_srcs],
		c_args : c_args,
		include_directories : inc_dir,
		dependencies: d2tk_cairo,
		install : false)

	if input_dep.found() and udev_dep.found()
		d2tk_fbdev = declare_dependency(
			include_directories : inc_dir,
			dependencies : [deps, freetype_dep, pixman_dep, cairo_dep, input_dep, udev_dep],
			link_args : links,
			sources : [lib_srcs, cairo_srcs, fbdev_srcs])

		executable('d2tk.fbdev', [bin_srcs, fbdev_bin_srcs],
			c_args : c_args,
			include_directories : inc_dir,
			dependencies: d2tk_fbdev,
			install : false)
	endif
endif

d2tk_nanovg = declare_dependency(
	include_directories : inc_dir,
	dependencies : deps,
	link_args : links,
	sources : [lib_srcs, nanovg_srcs, pugl_srcs])

executable('d2tk.nanovg', [bin_srcs, pugl_bin_srcs],
	c_args : c_args,
	include_directories : inc_dir,
	dependencies: d2tk_nanovg,
	install : false)

configure_file(
	input : join_paths('nanovg', 'example', 'Roboto-Bold.ttf'),
	output : 'Roboto-Bold.ttf',
	copy : true,
	install : false)

configure_file(
	input : join_paths('example', 'libre-arrow-circle-right.png'),
	output : 'libre-arrow-circle-right.png',
	copy : true,
	install : false)

configure_file(
	input : join_paths('example', 'libre-gui-folder.png'),
	output : 'libre-gui-folder.png',
	copy : true,
	install : false)

configure_file(
	input : join_paths('example', 'libre-gui-file.png'),
	output : 'libre-gui-file.png',
	copy : true,
	install : false)

test_core = executable('test.core', [test_core_srcs, lib_srcs],
	c_args : c_args,
	dependencies : deps,
	include_directories : inc_dir,
	install : false)

test_base = executable('test.base', [test_base_srcs, lib_srcs],
	c_args : c_args,
	dependencies : deps,
	include_directories : inc_dir,
	install : false)

test('Test core', test_core)
test('Test base', test_base)
