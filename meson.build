project('finger', 'cpp',
        version : '0.1.0',
        default_options : ['warning_level=3',
                          'cpp_std=c++20'])

# Find Boost dependencies - prefer static libraries
boost_dep = dependency('boost', modules : ['system'], static : true)

# pthreads — required by Boost ASIO; clang on FreeBSD does not link it implicitly
threads_dep = dependency('threads')

# Find Google Test and Google Mock dependencies
gtest_dep = dependency('gtest', main : true, required : true)
gmock_dep = dependency('gmock', main : true, required : true)

executable('finger',
  'main.cpp','handler.cpp','ban.cpp',
  dependencies : [boost_dep, threads_dep],
  install : true)

# Test executable
test_exe = executable('test_handler',
  'test_handler.cpp', 'handler.cpp',
  dependencies : [boost_dep, threads_dep, gtest_dep, gmock_dep])

# Mock test executable
test_mock_exe = executable('test_handler_mock',
  'test_handler_mock.cpp', 'handler.cpp',
  dependencies : [boost_dep, threads_dep, gtest_dep, gmock_dep])

# Real filesystem test executable
test_real_fs_exe = executable('test_handler_real_filesystem',
  'test_handler_real_filesystem.cpp', 'handler.cpp',
  dependencies : [boost_dep, threads_dep, gtest_dep, gmock_dep])

# Ban tracker test executable
test_ban_exe = executable('test_ban',
  'test_ban.cpp', 'ban.cpp',
  dependencies : [boost_dep, threads_dep, gtest_dep, gmock_dep])

# Register the tests
test('handler_tests', test_exe)
test('handler_mock_tests', test_mock_exe)
test('handler_real_filesystem_tests', test_real_fs_exe)
test('ban_tests', test_ban_exe)
