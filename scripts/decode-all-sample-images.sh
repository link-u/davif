#! /bin/bash
BASE_DIR=$(cd $(dirname $0); cd ..; pwd)

DAVIF=${BASE_DIR}/cmake-build-debug/davif

cd ${BASE_DIR}
IMAGES=$(env --chdir ./sample-images find . -type f -name \*.avif -print)
mkdir -p decoded
for img in ${IMAGES}; do
   ${DAVIF} -i sample-images/$img -o decoded/$img.png
done