#!/bin/bash

required_version=19

echo "Checking if clang-format is installed and exactly at version ${required_version}."

clang-format --version || (echo "clang-format is not installed." && exit 1)

current_version=$(clang-format --version | sed -E 's/[^0-9]*([0-9]+).*/\1/' | head -n1)

if [ "${current_version}" -ne "${required_version}" ]; then
    echo "clang-format version ${current_version} is not equal to ${required_version}."
    exit 1
fi

REPO_ROOT_DIR="."
CLANG_FORMAT_FILE="${REPO_ROOT_DIR}/.clang-format"

if [ ! -f "${CLANG_FORMAT_FILE}" ]; then
    echo ".clang-format not found in the repository root."
    exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 run/check"
    exit 1
fi

PATHS="${REPO_ROOT_DIR}/include/ciel ${REPO_ROOT_DIR}/test/include/ciel/test ${REPO_ROOT_DIR}/test/src ${REPO_ROOT_DIR}/benchmark/src"

operation="$1"

if [ "${operation}" == "run" ]; then
    echo "Formatting..."

    find ${PATHS} -type f \( -name "*.cpp" -or -name "*.h" -or -name "*.hpp" \) -exec clang-format -i --style=file '{}' \;

    echo "All header and source files have been formatted."

elif [ "${operation}" == "check" ]; then
    echo "Checking..."

    files=$(find ${PATHS} -name "*.cpp" -or -name "*.h" -or -name "*.hpp")

    for file in ${files}; do
        if ! clang-format -style=file --dry-run -Werror "${file}"; then
            echo "${file} requires formatting."
            exit 1
        fi
    done

    echo "All header and source files have been formatted."
    
else
    echo "Unknown operation: ${operation}"
    echo "Usage: $0 run/check"
    exit 1
fi
