# Unit tests are added in Phase 1 (clock model).
# Placeholder subdir so the top-level meson.build's subdir('tests') call
# succeeds during the Phase 0 scaffold build.
test_clock_model = executable('test_clock_model',
  ['test_clock_model.c', '../src/clock_model.c'],
  dependencies : [check_dep, m_dep])

test('clock_model', test_clock_model)
