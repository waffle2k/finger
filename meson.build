project('finger', 'cpp',
        version : '0.1.0',
        default_options : ['warning_level=3',
                          'cpp_std=c++20'])

# Find Boost dependencies - prefer static libraries
boost_dep = dependency('boost', modules : ['system'], static : true)

# Find Google Test and Google Mock dependencies
gtest_dep = dependency('gtest', main : true, required : true)
gmock_dep = dependency('gmock', main : true, required : true)

executable('finger',
  'main.cpp','handler.cpp',
  dependencies : [boost_dep],
  link_args : ['-static', '-static-libgcc', '-static-libstdc++'],
  install : true)

# Test executable
test_exe = executable('test_handler',
  'test_handler.cpp', 'handler.cpp',
  dependencies : [boost_dep, gtest_dep, gmock_dep])

# Mock test executable
test_mock_exe = executable('test_handler_mock',
  'test_handler_mock.cpp', 'handler.cpp',
  dependencies : [boost_dep, gtest_dep, gmock_dep])

# Real filesystem test executable
test_real_fs_exe = executable('test_handler_real_filesystem',
  'test_handler_real_filesystem.cpp', 'handler.cpp',
  dependencies : [boost_dep, gtest_dep, gmock_dep])

# Register the tests
test('handler_tests', test_exe)
test('handler_mock_tests', test_mock_exe)
test('handler_real_filesystem_tests', test_real_fs_exe)
