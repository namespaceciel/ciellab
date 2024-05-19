#!/bin/bash

required_version=16

echo "Checking if clang-format is installed and not older than $required_version."

clang-format --version

current_version=$(clang-format --version | grep -oP '(\d+)' | head -n1 | cut -d'.' -f1)

if [ "$current_version" -lt "$required_version" ]; then
    echo "clang-format version $current_version is less than $required_version."
    exit 1
fi

REPO_ROOT_DIR="."
CLANG_FORMAT_FILE="$REPO_ROOT_DIR/.clang-format"

if [ ! -f "$CLANG_FORMAT_FILE" ]; then
    echo ".clang-format not found in the repository root."
    exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 run/check"
    exit 1
fi

operation="$1"

if [ "$operation" == "run" ]; then
    echo "Formatting..."

    find "$REPO_ROOT_DIR/include/ciel" "$REPO_ROOT_DIR/test/src" "$REPO_ROOT_DIR/benchmark/src" -type f \( -name "*.cpp" -or -name "*.h" -or -name "*.hpp" \) -exec clang-format -i --style=file '{}' \;

    echo "All header and source files have been formatted."

elif [ "$operation" == "check" ]; then
    echo "Checking..."

    files=$(find "$REPO_ROOT_DIR/include/ciel" "$REPO_ROOT_DIR/test/src" "$REPO_ROOT_DIR/benchmark/src" -name "*.cpp" -or -name "*.h" -or -name "*.hpp")

    for file in $files; do
        if ! clang-format -style=file --dry-run -Werror "$file"; then
            echo "$file requires formatting."
            exit 1
        fi
    done

    echo "All header and source files have been formatted."
    
else
    echo "Unknown operation: $operation"
    echo "Usage: $0 run/check"
    exit 1
fi
