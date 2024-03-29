#! /bin/bash -eux

function readlink_f() {
  local src='import os,sys;print(os.path.realpath(sys.argv[1]))'
  python3 -c "${src}" "$1" || python -c "${src}" "$1"
}

ROOT_DIR="$(cd "$(readlink_f "$(dirname "$0")")" && cd .. && pwd)"
cd "${ROOT_DIR}" || exit 1

set -eux
set -o pipefail

DEPS_DIR="$(readlink_f "${ROOT_DIR}/_deps")"

rm -Rf "${DEPS_DIR}"
mkdir -p "${DEPS_DIR}"

# dav1d
bash -eux <<EOF
cd external/dav1d
rm -Rf build
meson setup \
  "--prefix=${DEPS_DIR}" \
  "--libdir" "lib" \
  "--auto-features" "disabled" \
  "--backend" "ninja" \
  "--buildtype" "release" \
  "--default-library" "static" \
  "-Denable_asm=true" \
  "-Denable_tests=false" \
  "build" \
  "."
(cd build && meson compile -C . && meson install)
EOF
