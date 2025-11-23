# Git Hooks - Production Setup

This directory contains production-ready Git hooks and setup scripts for the HelixShell project.

## Quick Start

To install the Git hooks on your local machine:

```bash
./scripts/setup-hooks.sh
```

## What's Included

### Pre-Commit Hook

Runs **before every commit** to ensure code quality:

- ✅ **Debugging artifacts check**: Warns about `cout`, `printf`, `DEBUG`, `FIXME`, `XXX`
- ✅ **Code formatting check**: Warns about tabs (project uses spaces)
- ✅ **Build validation**: Ensures code compiles before commit
- ✅ **Merge conflict detection**: Prevents accidental commits with conflict markers

**When it runs**: Automatically on `git commit`

**Skip (emergency only)**: `git commit --no-verify`

### Pre-Push Hook

Runs **before every push** for comprehensive validation:

- ✅ **Protected branch warning**: Confirms push to master/main
- ✅ **Full build**: Clean build from scratch
- ✅ **Test suite**: Runs all tests
- ✅ **Uncommitted changes check**: Warns about unstaged work
- ✅ **Code quality metrics**: Large files, TODO/FIXME tracking

**When it runs**: Automatically on `git push`

**Skip (emergency only)**: `git push --no-verify`

## For Team Members

1. **First time setup**:
   ```bash
   git clone <repository>
   cd HelixShell
   ./scripts/setup-hooks.sh
   ```

2. **Your existing hooks will be backed up** with timestamp

3. **Hooks run automatically** - no manual intervention needed

## Troubleshooting

### Pre-commit fails on build
```bash
# Check what's wrong
make build

# Fix the build errors, then commit again
git commit
```

### Pre-push fails on tests
```bash
# Run tests locally to see failures
make test

# Fix failing tests, then push again
git push
```

### Emergency bypass (use sparingly!)
```bash
# Skip hooks only when absolutely necessary
git commit --no-verify
git push --no-verify
```

## Benefits

- 🛡️ **Catches errors early**: Before they reach CI/CD
- 🚀 **Faster feedback**: Local validation is instant
- 📊 **Better code quality**: Automated standards enforcement
- 👥 **Team consistency**: Everyone runs same checks

## Hook Behavior

- **Non-blocking warnings**: Some checks warn but don't fail (debug code, formatting)
- **Blocking errors**: Build failures and test failures block commits/pushes
- **Clean logs**: Temporary build logs are auto-cleaned

## Updating Hooks

If hooks are updated in the repository:

```bash
# Re-run setup to get latest version
./scripts/setup-hooks.sh
```

---

**Note**: These hooks are for local development. CI/CD pipeline runs additional checks independently.
