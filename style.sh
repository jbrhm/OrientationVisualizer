#!/usr/bin/env bash

readonly RED='\033[0;31m'
readonly NC='\033[0m'

CLANG_FORMAT_ARGS=("-style=file")

# Just do a dry run if the "fix" argument is not passed
if [ $# -eq 0 ] || [ "$1" != "--fix" ]; then
  CLANG_FORMAT_ARGS+=("--dry-run" "--Werror")
fi

function print_update_error() {
  echo -e "${RED}[Error] Please Install clang-format${NC}"
  exit 1
}

function find_executable() {
  local readonly executable="$1"
  local readonly path=$(which "${executable}")
  if [ ! -x "${path}" ]; then
    echo -e "${RED}[Error] Could not find ${executable}${NC}"
    print_update_error
  fi
  echo "${path}"
}

## Check that all tools are installed

readonly CLANG_FORMAT_PATH=$(find_executable clang-format)

## Run checks

echo "Style checking C++ ..."
readonly FOLDERS="./src"

echo "Checking $(ls ${FOLDERS})"

output=""

# Run clang-format on all src
for FOLDER in "${FOLDERS[@]}"; do
  find "${FOLDER}" -regex '.*\.\(cpp\|hpp\|h\)' | while read -r file; do
	# Run clang-format and capture the output
	output+=$("${CLANG_FORMAT_PATH}" "${CLANG_FORMAT_ARGS[@]}" "$file")
  done
done

# Check if the output is empty
if [ -s $output ]; then
	echo "Output is not empty."
	echo $output
	exit 1
else
	echo "Output is empty, no style errors"
fi
echo "Done"
