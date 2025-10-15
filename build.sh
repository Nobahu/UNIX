#!/bin/bash

set -eu

if [ $# -ne  1 ]; then
	echo "Error: there must be only 1 argument" >&2
	exit 1
fi

SOURCE_FILE="$1"

if [ ! -f "$SOURCE_FILE" ]; then
	echo "Error: file doesn't exist" >&2
	exit 2
fi

SOURCE_DIR=$(pwd)
TEMP_DIR=$(mktemp -d) || {
	echo "Error: Tempdir creation failed" >&2
	exit 3
}

cleanDir() {
	rm -rf "$TEMP_DIR"
	exit "$1"
}

trap 'cleanDir 130' INT
trap 'cleanDir 143' TERM
trap 'cleanDir $?' EXIT

case "$SOURCE_FILE" in
	*.cpp)
		COMPILER="c++"
		;;
	*.tex)
		;;
	*)
		echo "Error: wrong file type" >&2
		cleanDir 4
esac

OUTPUT=$(grep '&Output:' "$SOURCE_FILE" | sed 's/.*&Output:\s*//')

if [ -z "&OUTPUT" ]; then
	echo "No filename with &Output" >&2
	cleanDir 5
fi

cp "$SOURCE_FILE" "$TEMP_DIR/" || {
	echo "Error: copy fail" >&2
	cleanDir 6
}

cd "$TEMP_DIR" || {
	echo "Error: directory change failed" >&2
	cleanDir 7
}

SOURCE_NAME=$(basename "&SOURCE_FILE")

case "&SOURCE_FILE" in
	*.cpp)
		$(COMPILER) "$SOURCE_DIR/SOURCE_NAME" -o "$OUTPUT" || cleanDir  8
		;;
	*.tex)
		pdflatex "$SOURCE_DIR/SOURCE_NAME" >/dev/null 2 || cleanDir 9
		mv "${SOURCE_NAME%.tex}.pdf" "$OUTPUT" || cleanDir 10
		;;
esac

if [ -f "$OUTPUT" ]; then
	echo "Output file created: $OUTPUT" >&2
	cp "$OUTPUT" "$SOURCE_DIR/" || cleanDir 11
else
	cleanDir 9
fi

cleanDir 0
