#include <dav1d/dav1d.h>
#include <vector>
#include <filesystem>
#include <optional>
#include "../external/libavif-container/src/avif/Parser.hpp"
#include "../external/libavif-container/src/util/Logger.hpp"
#include "../external/libyuv/include/libyuv.h"
#include "util/File.h"
#include "util/StreamWriter.hpp"

void nop_free_callback(const uint8_t *buf, void *cookie) {
}

std::vector<uint8_t> createBitmap(Dav1dPicture const& pic) {
  int w = pic.p.w;
  int h = pic.p.h;
  int const header_size = 54;
  // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader#calculating-surface-stride
  int const stride = w*4;
  std::vector<uint8_t> img;
  img.resize(stride * h);

  switch(pic.p.layout){
    case DAV1D_PIXEL_LAYOUT_I400: { ///< monochrome
      libyuv::I400ToARGB(
          reinterpret_cast<const uint8_t *>(pic.data[0]),
          pic.stride[0],
          img.data(),
          stride,
          w, h);
      break;
    }
    case DAV1D_PIXEL_LAYOUT_I420: { ///< 4:2:0 planar
      libyuv::I420ToARGB(
          reinterpret_cast<const uint8_t *>(pic.data[0]),
          pic.stride[0],
          reinterpret_cast<const uint8_t *>(pic.data[1]),
          pic.stride[1],
          reinterpret_cast<const uint8_t *>(pic.data[2]),
          pic.stride[1],
          img.data(),
          stride,
          w, h);
      break;
    }
    case DAV1D_PIXEL_LAYOUT_I422: { ///< 4:2:2 planar
      libyuv::I422ToARGB(
          reinterpret_cast<const uint8_t *>(pic.data[0]),
          pic.stride[0],
          reinterpret_cast<const uint8_t *>(pic.data[1]),
          pic.stride[1],
          reinterpret_cast<const uint8_t *>(pic.data[2]),
          pic.stride[1],
          img.data(),
          stride,
          w, h);
      break;
    }
    case DAV1D_PIXEL_LAYOUT_I444: { ///< 4:4:4 planar
      libyuv::I444ToARGB(
          reinterpret_cast<const uint8_t *>(pic.data[0]),
          pic.stride[0],
          reinterpret_cast<const uint8_t *>(pic.data[1]),
          pic.stride[1],
          reinterpret_cast<const uint8_t *>(pic.data[2]),
          pic.stride[1],
          img.data(),
          stride,
          w, h);
      break;
    }
  }


  util::StreamWriter bmp;
  // BMP header
  bmp.putU16L(0x4d42);
  bmp.putU32L(img.size() + header_size);
  bmp.putU16L(0);
  bmp.putU16L(0);
  bmp.putU32L(header_size);

  // DIB header
  bmp.putU32L(40);
  bmp.putU32L(w);
  bmp.putU32L(-h >> 0u);
  bmp.putU16L(1);
  bmp.putU16L(32); //ARGB
  bmp.putU32L(0);
  bmp.putU32L(img.size());
  bmp.putU32L(2835);
  bmp.putU32L(2835);
  bmp.putU32L(0);
  bmp.putU32L(0);
  bmp.append(img);
  return bmp.buffer();
}

int main(int argc, char** argv) {
  util::Logger log(stdout, stderr, util::Logger::DEBUG);
  if(argc <= 2) {
    log.error("usage: avif-decoder <filename>.avif <filename>.bmp");
    return -1;
  }
  std::string inputFilename = std::string(argv[1]);
  std::string outputFilename = std::string(argv[2]);
  if(inputFilename == outputFilename) {
    log.error("usage: avif-decoder <filename>.avif <filename>.bmp");
    return -1;
  }

  log.debug("dav1d: %s", dav1d_version());

  // init dav1d
  Dav1dSettings settings{};
  dav1d_default_settings(&settings);
  Dav1dContext* ctx{};
  int err = dav1d_open(&ctx, &settings);
  if(err != 0) {
    log.error("Failed to open dav1d: %d\n", err);
    return -1;
  }

  // read file.
  std::variant<std::vector<uint8_t>, std::string> avif_data = util::readFile(inputFilename);
  if(std::holds_alternative<std::string>(avif_data)){
    log.error("%s\n", std::get<1>(avif_data));
    return -1;
  }

  // parse ISOBMFF
  avif::Parser parser(log, std::move(std::get<0>(avif_data)));
  std::variant<std::shared_ptr<avif::FileBox>, std::string> res = parser.parse();
  if(std::holds_alternative<std::string>(res)){
    log.error("Failed to parse %s as avif: %s\n", inputFilename, std::get<1>(res));
    return -1;
  }
  std::shared_ptr<avif::FileBox> fileBox = std::get<0>(res);

  // start decoding
  Dav1dData data{};
  Dav1dPicture pic{};
  // FIXME: multiple pixtures
  size_t const baseOffset = fileBox->metaBox.itemLocationBox.items[0].baseOffset;
  size_t const extentOffset = fileBox->metaBox.itemLocationBox.items[0].extents[0].extentOffset;
  size_t const extentLength = fileBox->metaBox.itemLocationBox.items[0].extents[0].extentLength;
  auto avifBegin = parser.buffer().cbegin();
  auto imgBegin = std::next(avifBegin, baseOffset + extentOffset);
  auto imgEnd = std::next(imgBegin, extentLength);
  // FIXME: avoid copy
  std::vector<uint8_t> obu_data = std::vector<uint8_t>(imgBegin, imgEnd);
  dav1d_data_wrap(&data, obu_data.data(), obu_data.size(), nop_free_callback, nullptr);
  err = dav1d_send_data(ctx, &data);

  if(err < 0) {
    log.error( "Failed to send data to dav1d: %d\n", err);
    return -1;
  }

  err = dav1d_get_picture(ctx, &pic);
  if (err < 0) {
    log.error("Failed to decode dav1d: %d\n", err);
    return -1;
  }

  std::vector<uint8_t> bitmap = createBitmap(pic);
  util::writeFile(outputFilename, bitmap);

  dav1d_picture_unref(&pic);
  dav1d_close(&ctx);
  return 0;
}
