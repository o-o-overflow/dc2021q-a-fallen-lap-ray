#!/bin/bash

TEMPLATE_GITHUB="git@github.com:o-o-overflow/dc2021q-template.git"

# So, this is kinda dirty -- and yes, the tester script could also live outside challenge repos
# However, this works pretty well and allows gradually improving in the build-up to the game

set -eu -o pipefail
GIT="git --no-pager"

if [ ! -z "$($GIT status --untracked-files=no --porcelain)" ]; then
    echo "[:(] I'm not sure this works on a dirty repository. Please stash / reset / ..."
    echo "     Status of tracked files:"
    $GIT status --untracked-files=no --porcelain
    exit 3
fi

$GIT remote add template "$TEMPLATE_GITHUB" 2>/dev/null || true
$GIT fetch -q template
TEMPLATE_MASTER="$(git describe template/master --always)"
TEMPLATE_MASTER_BLOB="$($GIT ls-tree -l template/master tester | cut -d' ' -f3)"

# 1. Find the (oldest) template commit that matches your ./tester file
MY_BLOB="$($GIT hash-object -t blob --no-filter ./tester)"
while read TREEHASH COMMITHASH COMMITSUBJ; do
    TEMPLATE_BLOB="$($GIT ls-tree -l "$TREEHASH" tester | cut -d' ' -f3)"
    if [ "$MY_BLOB" = "$TEMPLATE_BLOB" ]; then
        COMMON_COMMIT="$COMMITHASH"
        COMMON_COMMIT_SUBJ="$COMMITSUBJ"
    fi
done < <($GIT log --pretty='%T %h %s' template/master -- tester)
if [ "${COMMON_COMMIT-xxx}" = 'xxx' ]; then
    echo "[:(] I didn't find your ./tester version among template commits, is it custom?"
    exit 1
fi
echo "[ ] Guessed your last template sync was at $COMMON_COMMIT ('$COMMON_COMMIT_SUBJ')"

if [ $COMMON_COMMIT = $TEMPLATE_MASTER ]; then
    echo "I think you're already fully up-to-date"
    $GIT remote rm template
    exit 0
fi


# 2. What changed since then?
echo "[I] Template commits since then:"
$GIT log --pretty='format:%C(auto)%h %s' $COMMON_COMMIT..$TEMPLATE_MASTER --
echo -e "\n\n[I] Diff since then EXCLUDING THE ./tester:"
$GIT diff $COMMON_COMMIT..$TEMPLATE_MASTER -- . ':(exclude)tester'


# 3. Try to apply that diff in the most friendly way possible
echo -e "\n[I] git apply --check and --summary..."
$GIT diff $COMMON_COMMIT..$TEMPLATE_MASTER > template_update.patch
$GIT apply --check template_update.patch || true
$GIT apply --summary template_update.patch || true

echo -e "\n\n[*] Trying to git  apply -v --index --3way template_update.patch"
$GIT apply -v --index --3way template_update.patch

# 3a. Small sanity check
MY_BLOB="$($GIT hash-object -t blob --no-filter ./tester)"
if [ "$MY_BLOB" != "$TEMPLATE_MASTER_BLOB" ]; then
    echo "[!] Despite applying the same commits, this ./tester != the template's?!?"
    exit 9
fi


# 4. Auto-commit, cleanup
echo -e "\n\n[*] Good! Auto-committing"
$GIT commit -m "[t] sync with the template $COMMON_COMMIT..$TEMPLATE_MASTER"
rm -f template_update.patch
$GIT remote rm template

echo -e "[:)] All done!"
