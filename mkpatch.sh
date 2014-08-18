#!/bin/bash

PATCH_COMPLETE="psas_capstone_2014-complete.patch"
PATCH_MODIFIED="psas_capstone_2014-modified.patch"

# Need an upstream set
git remote show upstream >/dev/null 2>&1
if [ "$?" -ne "0" ]; then
    echo >&2 "ERROR git upstream not set"
    exit 1
fi

# Prompt
echo -n "Merge `git config --get remote.upstream.url` into branch `git rev-parse --abbrev-ref HEAD`? "
read -n 1 CONFIRM
echo
CONFIRM=`echo "$CONFIRM" | tr 'Y' 'y'`
test "$CONFIRM" == "y" || exit 1

# Do Merge
echo "Merging Upstream"
git fetch upstream
git merge -m "Merge remote-tracking branch 'upstream/stable_2.6.x' into stable_2.6.x" upstream/stable_2.6.x
echo

# Show Status
echo "Diffed Files -- Excluding Added"
git diff --no-prefix --name-status upstream/stable_2.6.x | grep -v '^A'

echo

# Create Patches
git diff --no-prefix upstream/stable_2.6.x > "$PATCH_COMPLETE"
echo "Complete Patch: $PATCH_COMPLETE"

git diff --no-prefix --diff-filter=M upstream/stable_2.6.x > "$PATCH_MODIFIED"
echo "Modification-Only Patch: $PATCH_MODIFIED"
