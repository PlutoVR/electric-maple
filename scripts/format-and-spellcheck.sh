#!/bin/sh
# Copyright 2019, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
# Author: Rylie Pavlik <rylie.pavlik@collabora.com>

# Run both format-project and codespell-project, making a patch if any changes can be auto-made.
# Intended mainly for use in CI, but can be used elsewhere.

set -e
PATCH_DIR=patches
PATCH_NAME=fixes.diff
(
    cd $(dirname $0)
    
    # Exit if working tree dirty or uncommitted changes staged.
    if ! git diff-files --quiet; then
        echo "ERROR: Cannot perform check/fix, working tree dirty."
        exit 2
    fi
    if ! git diff-index --quiet HEAD -- ; then
        echo "ERROR: Cannot perform check/fix, changes staged but not committed."
        exit 2
    fi


    echo "Running codespell..."
    echo
    if ./codespell-project.sh; then
        # No errors, or only errors that could be auto-fixed. e.g. "g a r a n t e e" (remove spaces)
        CODESPELL_RESULT=true
    else
        # At least one non-auto-fixable error. e.g. "a o t h e r" (remove spaces)
        echo
        echo "Codespell found at least one issue it couldn't auto-fix, see preceding."
        echo "If the issue isn't actually a problem, edit IGNORE_WORDS_LIST in $(dirname $0)/codespell-project.sh"
        echo "Otherwise, you may run \`$(dirname $0)/codespell-project.sh -i 3\` locally to interactively fix."
        echo
        CODESPELL_RESULT=false
    fi

    
    echo "Running clang-format..."
    echo
    ./format-project.sh

    echo "Running cmake-format..."
    echo
    ./format-cmake.sh

    
    (
        cd ..
        mkdir -p $PATCH_DIR
        # Can't use tee because it hides the exit code
        if git diff --patch --exit-code > $PATCH_DIR/$PATCH_NAME; then
            echo
            echo "clang-format, cmake-format and codespell changed nothing."
        else
            echo
            echo "clang-format and/or codespell made at least one change, please apply the patch in the job artifacts and seen below!"
            echo "If codespell made a change in error, edit IGNORE_WORDS_LIST in $(dirname $0)/codespell-project.sh"
            echo
            echo "---------------------------------------"
            cat $PATCH_DIR/$PATCH_NAME
            echo "---------------------------------------"

            exit 1
        fi

        # It's possible that nobody made a change but codespell found an issue it couldn't auto-fix
        if ${CODESPELL_RESULT}; then
            echo "No manual changes required."
        else
            echo "However, manual changes to fix the codespell issue(s) required."
            exit 1
        fi
    )

)
