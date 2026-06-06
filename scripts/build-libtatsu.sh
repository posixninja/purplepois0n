#!/usr/bin/env bash
# Build libtatsu into external/libtatsu-install (local prefix, no sudo).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${ROOT}/external/libtatsu-install"
SRC="${ROOT}/external/libtatsu"
BUILD="${ROOT}/build/libtatsu"

mkdir -p "${BUILD}"
cd "${BUILD}"

if [[ ! -d "${SRC}/.git" ]]; then
  echo "Cloning libtatsu into external/libtatsu..."
  mkdir -p "${ROOT}/external"
  git clone --depth 1 https://github.com/libimobiledevice/libtatsu.git "${SRC}"
fi

if [[ ! -x "${SRC}/configure" ]]; then
  echo "Running autogen.sh..."
  (cd "${SRC}" && ./autogen.sh)
fi

rm -rf "${BUILD}"
mkdir -p "${BUILD}"
cd "${BUILD}"
"${SRC}/configure" --prefix="${PREFIX}" --disable-static
make -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"
make install

echo ""
echo "libtatsu installed to ${PREFIX}"
echo "Build purplepois0n with:"
echo "  PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig make release LIBTATSU=1"
