#! /bin/bash -eux

set -eux

BASE_DIR=$(cd $(dirname $(readlink -f $0)) && cd .. && pwd)
cd ${BASE_DIR}

apt install -y ./artifact/*.deb
apt show davif
which davif

davif --help

ldd $(which davif)
