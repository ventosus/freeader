project('freeader', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=false',
	'b_lto=false',
	'c_std=c11'])

d2tk = subproject('d2tk')

use_backend = get_option('use-backend')
if use_backend == 'nanovg'
	d2tk_dep = d2tk.get_variable('d2tk_nanovg')
elif use_backend == 'cairo'
	d2tk_dep = d2tk.get_variable('d2tk_cairo')
else
	error('no valid UI backend given')
endif

add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-DPUGL_HAVE_GL', language : 'c')

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
jbig85_dep = cc.find_library('jbig85')
lv2_dep = dependency('lv2', version : '>=1.14.0')
png_dep = dependency('libpng')
cairo_dep = dependency('cairo')

ui_deps = [lv2_dep, m_dep, jbig85_dep, d2tk_dep]

root_inc = include_directories('..')
d2tk_inc = include_directories(join_paths('subprojects', 'd2tk'))

incs = [root_inc, d2tk_inc]

c_args = ['-fvisibility=hidden',
	'-ffast-math',
	'-Wno-misleading-indentation',
	'-Wno-unused-function']

freeader_emu = executable('freeader_emu', 'freeader_emu.c',
	c_args : c_args,
	dependencies : [ui_deps],
	include_directories : incs,
	install : true)

freeader_enc = executable('freeader_enc', 'freeader_enc.c',
	c_args : c_args,
	dependencies : [m_dep, jbig85_dep, png_dep],
	install : true)

freeader_dec = executable('freeader_dec', 'freeader_dec.c',
	c_args : c_args,
	dependencies : [m_dep, jbig85_dep],
	install : true)

freeader_toc = executable('freeader_toc', 'freeader_toc.c',
	c_args : c_args,
	dependencies : [cairo_dep],
	install : true)

configure_file(
	input : join_paths('subprojects', 'd2tk', 'nanovg', 'example', 'Roboto-Bold.ttf'),
	output : 'Roboto-Bold.ttf',
	copy : true,
	install : true,
	install_dir : join_paths(get_option('datadir'), 'freeader'))

test('Encoding', freeader_enc,
	is_parallel : false,
	args : [
		'-O', '0013.pig',
		'-t', 'Flatland',
		'-a', 'Adwin A. Abott',
		'../0013.pbm'
	])

test('Decoding', freeader_dec,
	is_parallel : false,
	args : [
		'0013.pig',
		'0013.pbm',
		'1'
	])

test('Toc', freeader_toc,
	is_parallel : false)

diff = find_program('diff', native : true, required : false)

if diff.found()
	test('Compare', diff,
		is_parallel : false,
		args : [
			'../0013.pbm',
			'0013.pbm'
		])
endif

#install_man('freeader_enc.1')
#install_man('freeader_emu.1')
#install_man('freeader_toc.1')
