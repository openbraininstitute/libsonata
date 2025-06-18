#!/usr/bin/env bash
DIRS_TO_FORMAT="src include python"

# Install the desired clang-format, check the lines
# changed by this diff are formatted correctly

set -euo pipefail

VENV=build/venv-clang-format
CLANG_FORMAT_VERSION=20.1.5

if [[ ! -d $VENV ]]; then
    python3 -mvenv "$VENV"
    "$VENV/bin/pip" install clang-format=="$CLANG_FORMAT_VERSION"
fi

set +u  # ignore errors in virtualenv's activate
source "$VENV/bin/activate"
set -u

changes_from="$(git merge-base HEAD remotes/origin/master)"
echo "Changes from: $changes_from"

changes=$(git-clang-format $changes_from $DIRS_TO_FORMAT || true)
echo $changes
if [[ $(echo "$changes" | grep -n1 'changed files') ]]; then
    echo "The following files require changes to pass the current clang-format"
    echo "$changes"
    echo ""
    echo "This is the diff:"
    git diff
    exit 1
fi
