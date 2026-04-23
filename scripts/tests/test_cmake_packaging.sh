#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
TMP_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/galay-utils-cmake.XXXXXX")

cleanup() {
    rm -rf "$TMP_ROOT"
}

trap cleanup EXIT INT TERM HUP

fail() {
    printf '%s\n' "FAIL: $*" >&2
    exit 1
}

PACKAGE_VERSION=$(sed -n 's/^project(galay-utils VERSION \([0-9][0-9.]*\) LANGUAGES CXX)$/\1/p' "$REPO_ROOT/CMakeLists.txt")
[ -n "$PACKAGE_VERSION" ] || fail "failed to parse galay-utils project version"

BUILD_DIR="$TMP_ROOT/build"
INSTALL_PREFIX="$TMP_ROOT/prefix"
CONSUMER_DIR="$TMP_ROOT/consumer"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_MODULE_TESTS=OFF >/dev/null
cmake --install "$BUILD_DIR" >/dev/null

[ -f "$INSTALL_PREFIX/lib/cmake/galay-utils/galay-utils-config-version.cmake" ] || fail "missing installed config-version file"

mkdir -p "$CONSUMER_DIR"
cat > "$CONSUMER_DIR/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.16)
project(galay_utils_consumer LANGUAGES CXX)

find_package(galay-utils ${PACKAGE_VERSION} REQUIRED CONFIG)

if(NOT TARGET galay::galay-utils)
    message(FATAL_ERROR "galay::galay-utils target is missing")
endif()
EOF

cmake -S "$CONSUMER_DIR" -B "$TMP_ROOT/consumer-build" \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" >/dev/null

printf '%s\n' "ok"
