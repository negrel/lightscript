#!/usr/bin/env bash

set -euo pipefail
#set -x

CFLAGS="-std=c99 -Wall -Wextra -Werror -pedantic -Wmissing-prototypes -Wstrict-prototypes -I $PWD/inc/ -I $PWD/src/"
TEST_CFLAGS="$CFLAGS -g -DDEBUG $(pkg-config --cflags --libs check)"
: ${CC:="clang"}
BUILD_DIR="build"

tr " " "\n" <<< "$CFLAGS" > compile_flags.txt

build() {
	mkdir -p "$BUILD_DIR"
	$CC $CFLAGS ./src/ls_vm.c -o "$BUILD_DIR/ls_vm.o"
}

tests() {
	mkdir -p "$BUILD_DIR"

	# Lexer tests.
	$CC $TEST_CFLAGS ./tests/lex.c ./src/lex.h ./src/lex.c -o "$BUILD_DIR/value_string_test"
	valgrind --quiet --leak-check=full --errors-for-leak-kinds=definite $_
}

clean() {
	rm -rf "$BUILD_DIR"
}

main() {
	if [ "$#" -gt "0" ]; then
		case "$1" in
			"build")
				build
				;;
			"clean")
				clean
				;;
			"tests"|"test")
				tests
				;;
			*)
				echo "Unknown command $1" >&2
				exit 1;
		esac
	else
		build
	fi
}

main $@
