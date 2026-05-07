#!/bin/bash
# Install git hooks for ble-locator project
# Run this script once after cloning the repository.

REPO_ROOT="$(git rev-parse --show-toplevel)"
HOOKS_DIR="$REPO_ROOT/hooks"
GIT_HOOKS_DIR="$REPO_ROOT/.git/hooks"

if [ ! -d "$HOOKS_DIR" ]; then
    echo "ERROR: hooks/ directory not found at $HOOKS_DIR"
    exit 1
fi

echo "Installing git hooks..."

# Install pre-commit
cp "$HOOKS_DIR/pre-commit" "$GIT_HOOKS_DIR/pre-commit"
chmod +x "$GIT_HOOKS_DIR/pre-commit"
echo "  pre-commit installed (clang-format check)"

# Install pre-push
cp "$HOOKS_DIR/pre-push" "$GIT_HOOKS_DIR/pre-push"
chmod +x "$GIT_HOOKS_DIR/pre-push"
echo "  pre-push installed (build check)"

echo "Done. Git hooks are now active."
