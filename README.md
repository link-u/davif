# davif

avif decoder, using dav1d directly.

## usage

```bash
davif -i <input.avif> -o <output.{bmp, png}>
```

Example avif files are available in [AOMediaCodec/av1-avif](https://github.com/AOMediaCodec/av1-avif/tree/master/testFiles).

## how to build

```bash
# cloning this repository with dependencies.
git clone --recurse-submodules --recursive git@link-u.github.com:link-u/davif.git
cd davif

# build davif
mkdir build && cd build
cmake ..
make

# show usage
./davif
SYNOPSIS
        davif -i <input.avif> -o <output.{bmp, png}>

# decode an avif image.
./davif -i input.avif -o output.png
```

## limitations

 - Not supported:
   - Alpha channel(Auxially images)
   - Color profiles
   - HDR images.

# Related repositories

 - [link-u/libavif-container](https://github.com/link-u/libavif-container) - a library to parse avif container.
