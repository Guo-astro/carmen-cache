#!/bin/bash

set -eu
set -o pipefail

# gyp will put "MAKEFLAGS=r -- BUILDTYPE=Release" into the makefiles
# which breaks the rocksdb build
unset MAKEFLAGS

# setup mason
./scripts/setup.sh --config local.env
source local.env

# avoid mis-reporting of CPU due to docker
# from resulting in OOM killer knocking out g++
export MASON_CONCURRENCY=2

# only build from source if it does not exist
if [[ ! -f mason_packages/.link/lib/libbz2.a ]]; then
    mason build bzip2 1.0.6
fi

# only build from source if it does not exist
if [[ ! -f mason_packages/.link/lib/librocksdb.a ]]; then
    patch -d ./.mason/scripts/rocksdb/4.13-dev/ -p1 < ./scripts/rocksdb/4.13-dev/mason-script-sh.diff
    mason build rocksdb 4.13-dev
fi

mason link bzip2 1.0.6
mason link rocksdb 4.13-dev
mason install protozero 1.6.2
mason link protozero 1.6.2
