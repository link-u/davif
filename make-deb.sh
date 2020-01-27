#! /bin/bash
set -e

# FIXME(ledyba-z): How about using venv?
pip3 install meson ninja --user
export PATH="$(python3 -c 'import site; print(site.USER_BASE);')/bin:${PATH}"

fakeroot debian/rules clean
fakeroot debian/rules binary
env --chdir=external/libpng git reset --hard
mv ../davif_*.deb ../davif-dbgsym_*.ddeb .

