#!/bin/sh

set -eu

if [ $# -ne 1 ]; then
    echo "Error: there must be only 1 argument" >&2
    exit 1
fi

SOURCE_FILE="$1"

SOURCE_DIR=$(pwd)
TEMP_DIR=$(mktemp -d)

cleanDir() {
    rm -rf "$TEMP_DIR"
    exit "$1"
}

trap 'cleanDir 130' INT
trap 'cleanDir 143' TERM
trap 'cleanDir $?' EXIT

OUTPUT=$(grep '&Output:' "$SOURCE_FILE" | sed 's/.*&Output:\s*//')
SOURCE_NAME=$(basename "$SOURCE_FILE")

cp "$SOURCE_FILE" "$TEMP_DIR/"
cd "$TEMP_DIR"

case "$SOURCE_FILE" in
    *.cpp)
	COMPILER="g++"
        echo "Compiling with $COMPILER..." >&2
        $COMPILER "$SOURCE_NAME" -o "$OUTPUT" || cleanDir 8
        ;;
    *.tex)
        echo "Compiling LaTeX..." >&2
        pdflatex "$SOURCE_NAME" >/dev/null 2>&1 || cleanDir 9
        mv "${SOURCE_NAME%.tex}.pdf" "$OUTPUT" || cleanDir 10
        ;;
esac

cp "$OUTPUT" "$SOURCE_DIR/"

cleanDir 0
