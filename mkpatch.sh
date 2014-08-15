#!/bin/bash

PATCH_COMPLETE="psas_capstone_2014-complete.patch"
PATCH_MODIFIED="psas_capstone_2014-modified.patch"

echo "Merging Upstream"
git fetch upstream
git merge -m "Merge remote-tracking branch 'upstream/stable_2.6.x' into stable_2.6.x" upstream/stable_2.6.x

echo
echo "Diffed Files -- Excluding Added"
git diff --no-prefix --name-status upstream/stable_2.6.x | grep -v '^A'

echo

git diff --no-prefix upstream/stable_2.6.x > "$PATCH_COMPLETE"
echo "Complete Patch: $PATCH_COMPLETE"

git diff --no-prefix --diff-filter=M upstream/stable_2.6.x > "$PATCH_MODIFIED"
echo "Modification-Only Patch: $PATCH_MODIFIED"
