project('finger', 'cpp',
        version : '0.1.0',
        default_options : ['warning_level=3',
                          'cpp_std=c++20'])

# Find Boost dependencies - prefer static libraries
boost_dep = dependency('boost', modules : ['system'], static : true)

# Find Google Test dependency
gtest_dep = dependency('gtest', main : true, required : true)

executable('finger',
  'main.cpp','handler.cpp',
  dependencies : [boost_dep],
  link_args : ['-static', '-static-libgcc', '-static-libstdc++'],
  install : true)

# Test executable
test_exe = executable('test_handler',
  'test_handler.cpp', 'handler.cpp',
  dependencies : [boost_dep, gtest_dep])

# Register the test
test('handler_tests', test_exe)
