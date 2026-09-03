test_clock_model = executable('test_clock_model',
  ['test_clock_model.c', '../src/clock_model.c'],
  dependencies : [check_dep, m_dep])

test('clock_model', test_clock_model)

test_flip_anim = executable('test_flip_anim',
  ['test_flip_anim.c', '../src/flip_anim.c'],
  dependencies : [check_dep])

test('flip_anim', test_flip_anim)

test_radio = executable('test_radio',
  ['test_radio.c', '../src/radio.c'],
  dependencies : [check_dep, gst_dep, m_dep])

test('radio', test_radio)

test_alarm = executable('test_alarm',
  ['test_alarm.c', '../src/alarm.c', '../src/clock_model.c'],
  dependencies : [check_dep])

test('alarm', test_alarm)
