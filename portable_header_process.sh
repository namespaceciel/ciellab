#!/bin/bash

directory="include/ciel"
headers=()

for file in "${directory}"/*; do
    if [[ -f "${file}" ]]; then
        headers+=("$(basename "${file}")")
    fi
done

portable_headers="portable_headers"

if [ -n "$1" ]; then
    portable_headers="$1"
fi

rm -rf ${portable_headers}
mkdir -p ${portable_headers}

included_headers=()

function process() {
    local input="$1"
    local output="$2"

    if [ ! -f ${input} ]; then
        echo "File ${input} doesn't exist."
        exit 1
    fi

    read -r first_line < ${input}

    if [[ " ${included_headers[@]} " =~ " ${first_line} " ]]; then
        return
    fi

    included_headers+=("$first_line")

    while IFS= read -r line; do
        if [[ $line =~ ^#include[[:space:]]*\<ciel\/([^/]+)\.hpp\> ]]; then

            header_name="${BASH_REMATCH[1]}"

            include_path="include/ciel/${header_name}.hpp"
                
            process ${include_path} ${output}
        else
            echo "${line}" >> ${output}
        fi
    done < "${input}"
}

for header in "${headers[@]}"; do
    echo -n "Processing ${header}... "

    touch ${portable_headers}/${header}

    included_headers=()

    process include/ciel/${header} ${portable_headers}/${header}

    echo "done."
done
