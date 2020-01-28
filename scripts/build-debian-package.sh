#! /bin/bash
set -e

BASE_DIR=$(cd $(dirname $0); cd ..; pwd)
cd ${BASE_DIR}

# FIXME(ledyba-z): How about using venv?
pip3 install meson ninja --user
export PATH="$(python3 -c 'import site; print(site.USER_BASE);')/bin:${PATH}"

fakeroot debian/rules clean
fakeroot debian/rules binary
# workaround. external/libpng will be dirty after making debian packages.
env --chdir=external/libpng git reset --hard
mv ../davif_*.deb ../davif-dbgsym_*.ddeb .