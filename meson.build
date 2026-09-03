project('flipclock', 'c',
  version : '0.1.0',
  default_options : ['warning_level=2', 'c_std=c11'])

gtk_dep = dependency('gtk4')
gst_dep = dependency('gstreamer-1.0', version : '>=1.20')
gst_audio_dep = dependency('gstreamer-audio-1.0', version : '>=1.20')
m_dep = meson.get_compiler('c').find_library('m', required : true)
check_dep = dependency('check', required : true)

flipclock_deps = [gtk_dep, gst_dep, gst_audio_dep, m_dep]

flipclock_sources = [
  'src/main.c',
  'src/clock_model.c',
  'src/clock_view.c',
  'src/flip_anim.c',
  'src/radio.c',
  'src/alarm.c',
  'src/buzzer.c',
]

executable('flipclock',
  flipclock_sources,
  dependencies : flipclock_deps,
  install : false)

subdir('tests')
