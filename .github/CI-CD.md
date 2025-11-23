# CI/CD Pipeline Documentation

## Overview

HelixShell implements a comprehensive CI/CD pipeline that runs automatically on every commit and pull request to ensure code quality, correctness, and reliability.

## Pipeline Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    CI/CD Pipeline Trigger                   │
│              (Push to master/main or Pull Request)          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
        ┌──────────────────────────────────────────┐
        │         Parallel Job Execution           │
        └──────────────────────────────────────────┘
                            │
        ┌───────────────────┴───────────────────┐
        │                                       │
        ▼                                       ▼
┌───────────────┐                      ┌───────────────┐
│ Build & Test  │                      │ Build & Test  │
│    (macOS)    │                      │    (Linux)    │
└───────────────┘                      └───────────────┘
        │                                       │
        └───────────────────┬───────────────────┘
                            │
                            ▼
        ┌──────────────────────────────────────────┐
        │         Quality Analysis Jobs            │
        └──────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│   Coverage   │   │    Static    │   │    Memory    │
│  (100% Goal) │   │   Analysis   │   │ Leak Check   │
└──────────────┘   └──────────────┘   └──────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
                            ▼
        ┌──────────────────────────────────────────┐
        │         Additional Quality Checks        │
        └──────────────────────────────────────────┘
                            │
        ┌───────────────────┴───────────────────┐
        │                                       │
        ▼                                       ▼
┌───────────────┐                      ┌───────────────┐
│ Code Quality  │                      │ Integration   │
│    Checks     │                      │     Tests     │
└───────────────┘                      └───────────────┘
        │                                       │
        └───────────────────┬───────────────────┘
                            │
                            ▼
                  ┌───────────────────┐
                  │  ✅ All Checks    │
                  │     Passed        │
                  └───────────────────┘
```

## Pipeline Jobs

### 1. Build & Test (macOS)
**Purpose**: Verify the project builds and tests pass on macOS

**Steps**:
- Checkout code
- Cache Homebrew packages for faster builds
- Install dependencies (CppUnit, pkg-config)
- Build the project using Make
- Run the complete test suite
- Verify all build artifacts are created correctly

**Artifacts**: Build executables (hsh, hsh_tests)

### 2. Build & Test (Linux)
**Purpose**: Ensure cross-platform compatibility with Linux

**Steps**:
- Checkout code
- Cache apt packages
- Install dependencies (build-essential, CppUnit)
- Build the project
- Run all tests
- Verify Linux compatibility

**Why**: Ensures the shell works across different Unix-like systems

### 3. Code Coverage (Target: 100%)
**Purpose**: Measure test coverage and ensure comprehensive testing

**Tools**: gcov, lcov

**Process**:
1. Build with coverage instrumentation (`--coverage` flag)
2. Run all tests to generate coverage data
3. Generate coverage report using lcov
4. Calculate coverage percentage
5. Upload HTML coverage report as artifact
6. Warn if coverage is below 100% target

**Artifacts**:
- Coverage report (HTML)
- Coverage statistics

**Access Reports**: Download from Actions artifacts tab

### 4. Static Analysis
**Purpose**: Detect potential bugs and code quality issues

**Tools**:
- cppcheck (comprehensive static analyzer)
- clang-tidy (LLVM-based linter)

**Checks**:
- Memory management issues
- Resource leaks
- Undefined behavior
- Performance issues
- Unsafe function usage (strcpy, sprintf, gets)
- Raw pointer usage (recommend smart pointers)

**Artifacts**: Static analysis report

### 5. Memory Leak Detection
**Purpose**: Ensure no memory leaks exist

**Tool**: Valgrind (Linux)

**Configuration**:
```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --error-exitcode=1
```

**Result**: Pipeline fails if any memory leaks detected

**Artifacts**: Valgrind report

### 6. Code Quality Checks
**Purpose**: Enforce coding standards and best practices

**Checks**:
- ✅ **Formatting**: No tabs (spaces only), no trailing whitespace
- ✅ **Debug Code**: No leftover cout/printf statements
- ✅ **Code Markers**: Warn on FIXME/XXX comments
- ✅ **Documentation**: README, headers with guards
- ✅ **Project Structure**: Required directories and files

**Standards**:
- Indentation: Spaces (no tabs)
- Header guards: Required on all .h files
- Documentation: README.md and inline comments

### 7. Integration Tests
**Purpose**: Test the shell in realistic scenarios

**Tests**:
- Simple command execution
- Commands with arguments
- Shell exit behavior
- Real-world usage patterns

**Timeout**: 5 seconds per test (prevents hanging)

### 8. Final Status Check
**Purpose**: Aggregate results from all jobs

**Dependencies**: Waits for all other jobs to complete

**Output**: Summary of all pipeline checks

## Performance Optimizations

### Caching Strategy
- **macOS**: Homebrew packages cached
- **Linux**: APT packages cached
- **Benefits**: Faster builds, reduced CI time

### Concurrency
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```
- Cancels outdated pipeline runs
- Saves CI resources
- Faster feedback on latest code

### Parallel Execution
- Jobs run in parallel when possible
- Dependent jobs (integration tests) wait for prerequisites
- Optimal resource utilization

## Trigger Conditions

### Automatic Triggers
```yaml
on:
  push:
    branches: [ master, main ]
  pull_request:
    branches: [ master, main ]
```

**Runs on**:
- Every push to master/main branches
- Every pull request targeting master/main
- Manual workflow dispatch (from Actions tab)

## Viewing Results

### GitHub Actions UI
1. Go to repository → **Actions** tab
2. Click on the latest workflow run
3. View job status and logs
4. Download artifacts (coverage reports, analysis results)

### Pull Request Integration
- Status checks appear on PR page
- Coverage summary posted as comment
- Prevents merging if checks fail

### Badges
Add to your README to show pipeline status:
```markdown
[![CI/CD Pipeline](https://github.com/adarshanand67/Helix-Shell/actions/workflows/ci.yml/badge.svg)](https://github.com/adarshanand67/Helix-Shell/actions/workflows/ci.yml)
```

## Failure Handling

### When Pipeline Fails

**1. Build Failure**
- Check build logs in the failing job
- Look for compilation errors
- Run `make build` locally to reproduce

**2. Test Failure**
- Review test output in logs
- Run `make test` locally
- Check for environment differences

**3. Coverage Below Target**
- Currently set to **warn only** (doesn't fail)
- Review coverage report artifact
- Add tests for uncovered code paths

**4. Memory Leaks**
- Pipeline **fails** if leaks detected
- Download valgrind report artifact
- Fix leaks and re-run

**5. Static Analysis Issues**
- Currently set to **warn only**
- Review cppcheck report
- Address high-priority issues

## Local Testing

Before pushing, run the same checks locally:

```bash
# Build and test
make clean && make build && make test

# Coverage (requires lcov installed)
CXXFLAGS="--coverage" LDFLAGS="--coverage" make build
make test
lcov --directory . --capture --output-file coverage.info
genhtml coverage.info --output-directory coverage/

# Static analysis (requires cppcheck)
cppcheck --enable=all src/ include/

# Memory check (requires valgrind, Linux only)
valgrind --leak-check=full ./build/hsh_tests
```

## Best Practices

### ✅ Do's
- Write tests for all new code
- Run `make test` before committing
- Keep coverage at or near 100%
- Fix memory leaks immediately
- Use smart pointers over raw pointers
- Document complex code

### ❌ Don'ts
- Don't commit without running tests locally
- Don't ignore CI warnings
- Don't use unsafe C functions (strcpy, etc.)
- Don't leave debug code (cout/printf)
- Don't skip code review

## Maintenance

### Adding New Checks
1. Edit `.github/workflows/ci.yml`
2. Add new job or step
3. Test in a feature branch first
4. Document in this file

### Updating Dependencies
- Update package versions in workflow
- Test locally first
- Update setup.sh to match

### Modifying Coverage Target
Currently: 100% (warning only)

To enforce:
```yaml
# In coverage job, change:
# exit 1  # Uncomment this line
```

## Costs & Resources

### GitHub Actions Minutes
- **Free tier**: 2,000 minutes/month (public repos unlimited)
- **Current usage**: ~10-15 minutes per pipeline run
- **Estimate**: ~100-150 runs/month

### Artifact Storage
- Coverage reports: ~5MB each
- Retention: 30 days
- Auto-cleanup after retention period

## Troubleshooting

### Pipeline Stuck/Hanging
- Check for infinite loops in tests
- Look for missing timeout configurations
- Cancel and retry

### Flaky Tests
- Add retries for network-dependent tests
- Use fixed seeds for random tests
- Ensure proper test isolation

### Slow Pipeline
- Check cache hit rates
- Parallelize more jobs
- Reduce artifact sizes

## Future Enhancements

### Planned
- [ ] Codecov integration for better coverage visualization
- [ ] Automated performance benchmarking
- [ ] Security scanning (dependency vulnerabilities)
- [ ] Automated releases on tag push
- [ ] Docker container builds
- [ ] Multi-architecture support (ARM64, x86_64)

### Under Consideration
- [ ] Nightly builds for extended tests
- [ ] Deployment to test environments
- [ ] Automated changelog generation
- [ ] Code complexity metrics

## Support

### Issues
If the pipeline fails unexpectedly:
1. Check recent commits for breaking changes
2. Review workflow logs
3. Compare with last successful run
4. Open an issue with logs attached

### Documentation
- [GitHub Actions Docs](https://docs.github.com/en/actions)
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
- [lcov Documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- [cppcheck Manual](http://cppcheck.net/manual.pdf)

---

**Last Updated**: 2025-01-23
**Pipeline Version**: 2.0
**Maintainer**: HelixShell Team
