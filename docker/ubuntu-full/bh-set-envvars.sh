#!/bin/sh
set -eu

if test "${TARGET_ARCH:-}" != ""; then
  if test "${TARGET_ARCH}" = "arm64"; then
      export GCC_ARCH=aarch64
  else
      echo "Unhandled architecture: ${TARGET_ARCH}"
      exit 0
  fi
  export APT_ARCH_SUFFIX=":${TARGET_ARCH}"
  export CC=${GCC_ARCH}-linux-gnu-gcc-13
  export CXX=${GCC_ARCH}-linux-gnu-g++-13
  export WITH_HOST="--host=${GCC_ARCH}-linux-gnu"
  export CMAKE_EXTRA_ARGS=" -DCMAKE_SYSTEM_PROCESSOR=${TARGET_ARCH} "
else
  export APT_ARCH_SUFFIX=""
  export WITH_HOST=""
  GCC_ARCH="$(uname -m)"
  export GCC_ARCH
  export CMAKE_EXTRA_ARGS=""
fi

if [ -n "${WITH_CCACHE:-}" ]; then
    CCACHE_PARAM="-DCMAKE_C_COMPILER_LAUNCHER=ccache"
    export CCACHE_PARAM="$CCACHE_PARAM -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
else
    export CCACHE_PARAM=""
fi
