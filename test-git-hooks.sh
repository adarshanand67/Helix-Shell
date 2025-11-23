#!/bin/bash

# Test script for Git hooks
# This demonstrates how the pre-push hook works

echo "🧪 Testing Git Hooks Setup"
echo "=========================="

# Check if hooks are properly installed
echo "1. Checking hook files..."
if [ -x ".git/hooks/pre-push" ]; then
    echo "   ✅ pre-push hook is installed and executable"
else
    echo "   ❌ pre-push hook is missing or not executable"
fi

if [ -x ".git/hooks/pre-commit" ]; then
    echo "   ✅ pre-commit hook is installed and executable"
else
    echo "   ❌ pre-commit hook is missing or not executable"
fi

echo ""
echo "2. Testing current project state..."
echo "   Building project..."
if [ -d "build" ]; then
    cd build
    make >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "   ✅ Build successful"
        cd tests
        ./hsh_tests >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "   ✅ All tests passing"
            echo ""
            echo "🎉 Git hooks are ready! Pushes will require tests to pass."

            echo ""
            echo "📖 Hook Behavior:"
            echo "   • pre-commit: Checks CMake configuration"
            echo "   • pre-push: Runs full build + tests before pushing"
            echo "   • If tests fail, push is blocked with helpful error message"
        else
            echo "   ❌ Tests are failing - hooks will block pushes"
        fi
    else
        echo "   ❌ Build failed"
    fi
else
    echo "   ❌ Build directory missing, run: mkdir build && cd build && cmake .. && make"
fi

echo ""
echo "3. To bypass hooks (for emergencies):"
echo "   git push --no-verify"
echo ""
echo "4. To disable hooks temporarily:"
echo "   chmod -x .git/hooks/pre-push"
echo "   chmod -x .git/hooks/pre-commit"
