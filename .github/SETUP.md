# GitHub Actions CI/CD Setup

This repository includes a comprehensive GitHub Actions workflow for C++20 compilation, testing, and code coverage reporting.

## Features

### 🔧 Multi-Platform Build & Test
- **Ubuntu Latest**: Primary development target with GCC
- **macOS Latest**: Cross-platform compatibility with Clang
- **Windows Latest**: Broader compatibility with GCC via vcpkg

### 🧪 Comprehensive Testing
- Runs all Google Test unit tests
- Validates security features (directory traversal protection)
- Fails build on any test failures
- Uploads test logs as artifacts

### 📊 Code Coverage Reporting
- **Coverage Tool**: gcov + lcov for detailed coverage analysis
- **Integration**: Automatic upload to Codecov.io
- **Reports**: HTML coverage reports as downloadable artifacts
- **Thresholds**: Configurable coverage targets (default: 80%)
- **PR Comments**: Automatic coverage change reporting

## Workflow Structure

```
CI Workflow
├── Build & Test Matrix (Ubuntu, macOS, Windows)
│   ├── Install Dependencies (Boost, Google Test)
│   ├── Meson Setup & Configure
│   ├── Compile with C++20
│   ├── Run Unit Tests
│   └── Upload Test Artifacts
└── Coverage Analysis (Ubuntu only)
    ├── Build with Coverage Flags
    ├── Run Tests with Coverage Collection
    ├── Generate lcov Reports
    ├── Upload to Codecov
    └── Generate HTML Reports
```

## Setup Instructions

### 1. Repository Setup
1. Push this repository to GitHub
2. Update the badge URLs in `README.md`:
   ```markdown
   [![CI](https://github.com/YOUR_USERNAME/YOUR_REPO_NAME/workflows/CI/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO_NAME/actions)
   [![codecov](https://codecov.io/gh/YOUR_USERNAME/YOUR_REPO_NAME/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/YOUR_REPO_NAME)
   ```

### 2. Codecov Integration
1. Visit [codecov.io](https://codecov.io) and sign in with GitHub
2. Add your repository to Codecov
3. No additional setup required - the workflow handles token-free uploads

### 3. Branch Protection (Optional)
Configure branch protection rules in GitHub:
- Require status checks to pass before merging
- Require branches to be up to date before merging
- Include the "Build and Test" and "Code Coverage" checks

## Configuration Files

### `.github/workflows/ci.yml`
Main CI/CD workflow with:
- Multi-platform build matrix
- Dependency management for each OS
- Test execution and artifact collection
- Coverage analysis and reporting

### `.codecov.yml`
Codecov configuration with:
- Coverage targets (80% project, 80% patch)
- File exclusions (test files, build directories)
- PR comment formatting
- Coverage flags for different components

### `.gitignore` Updates
Added coverage-related file exclusions:
- `*.gcda`, `*.gcno`, `*.gcov` - Coverage data files
- `coverage/` - Coverage report directories
- `coverage*.info` - lcov report files

## Workflow Triggers

The CI workflow runs on:
- **Push** to `main` and `develop` branches
- **Pull Requests** targeting `main` and `develop` branches

## Artifacts Generated

### Test Results
- Test logs from all platforms
- Available for 90 days after workflow completion

### Coverage Reports
- HTML coverage reports (viewable in browser)
- lcov data files for further analysis
- Codecov integration for web-based viewing

## Coverage Analysis

The coverage analysis focuses on:
- `handler.cpp` - Core business logic
- Security validation functions
- File I/O operations
- Excludes test files and system headers

### Coverage Thresholds
- **Project Coverage**: 80% minimum
- **Patch Coverage**: 80% minimum for new code
- **Threshold**: 5% tolerance for coverage changes

## Troubleshooting

### Common Issues

1. **Dependency Installation Failures**
   - Check if package names are correct for the target OS
   - Verify vcpkg installation on Windows

2. **Coverage Upload Failures**
   - Coverage uploads are set to non-blocking (`fail_ci_if_error: false`)
   - Check Codecov repository configuration

3. **Test Failures**
   - Review test logs in the workflow artifacts
   - Ensure all tests pass locally before pushing

### Local Testing

Test the workflow components locally:

```bash
# Standard build and test
meson setup builddir
meson compile -C builddir
meson test -C builddir --verbose

# Coverage build and test
meson setup builddir-coverage -Db_coverage=true
meson compile -C builddir-coverage
meson test -C builddir-coverage
```

## Customization

### Adding New Platforms
Extend the build matrix in `.github/workflows/ci.yml`:
```yaml
matrix:
  os: [ubuntu-latest, macos-latest, windows-latest, ubuntu-20.04]
```

### Changing Coverage Targets
Modify `.codecov.yml`:
```yaml
coverage:
  status:
    project:
      default:
        target: 90%  # Increase to 90%
```

### Adding Static Analysis
Add steps to the workflow for tools like:
- cppcheck
- clang-tidy
- AddressSanitizer/UBSan (already partially enabled)

## Security Considerations

The workflow includes security best practices:
- Pinned action versions (`@v4`, `@v3`)
- No secrets required for basic functionality
- Minimal permissions for workflow execution
- Static linking to reduce runtime dependencies
