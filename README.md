# davif

avif decoder, using dav1d directly.

## usage

```bash
davif -i <input.avif> -o <output.{bmp, png}>
```

Example avif files are available in [AOMediaCodec/av1-avif](https://github.com/AOMediaCodec/av1-avif/tree/master/testFiles).

## how to build

```bash
% git clone --recurse-submodules --recursive git@link-u.github.com:link-u/davif.git
% cd davif
%mkdir build && cd build
% cmake ..
% make
% ./davif
SYNOPSIS
        davif -i <input.avif> -o <output.{bmp, png}>

```

## limitations

 - Not supported:
   - Alpha channel(Auxially images)
   - Color profiles
