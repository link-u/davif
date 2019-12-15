#include <vector>
#include <filesystem>
#include <optional>

#include <dav1d/dav1d.h>
#include <libyuv.h>
#include <png.h>

#include <avif/Parser.hpp>
#include <util/Logger.hpp>

#include "util/File.h"
#include "util/StreamWriter.hpp"

bool endsWidh(std::string const& target, std::string const& suffix) {
  if(target.size() < suffix.size()) {
    return false;
  }
  return target.substr(target.size()-suffix.size()) == suffix;
}

void nop_free_callback(const uint8_t *buf, void *cookie) {
}

std::vector<uint8_t> convertToARGB(Dav1dPicture const& pic) {
  int const w = pic.p.w;
  int const h = pic.p.h;
  int const stride = w*4;
  std::vector<uint8_t> img;
  img.resize(stride * h);

  switch(pic.p.layout) {
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
  return std::move(img);
}

std::vector<uint8_t> convertToABGR(Dav1dPicture const& pic) {
  int const w = pic.p.w;
  int const h = pic.p.h;
  int const stride = w*4;
  std::vector<uint8_t> orig = convertToARGB(pic);
  std::vector<uint8_t> rotated;
  rotated.resize(orig.size());
  libyuv::ARGBToABGR(orig.data(), stride, rotated.data(), stride, w, h);
  return std::move(rotated);
}

std::optional<std::string> writeBitmap(util::Logger& log, std::string const& filename, std::vector<uint8_t> const& img, size_t const w, size_t const h) {
  static int const headerSize = 54;
  util::StreamWriter bmp;
  // BMP header
  bmp.putU16L(0x4d42);
  bmp.putU32L(img.size() + headerSize);
  bmp.putU16L(0);
  bmp.putU16L(0);
  bmp.putU32L(headerSize);

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
  auto result = util::writeFile(filename, bmp.buffer());
  return std::move(result);
}

static void png_write_callback(png_structp  png_ptr, png_bytep data, png_size_t length) {
  auto buff = reinterpret_cast<util::StreamWriter*>(png_get_io_ptr(png_ptr));
  buff->append(data, length);
}

std::optional<std::string> writePNG(util::Logger& log, std::string const& filename, std::vector<uint8_t>& img, int w, int h) {
  //FIXME(ledyba-z): add error handling
  png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr = png_create_info_struct(p);
  png_set_IHDR(p, info_ptr, w, h, 8,
               PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  std::vector<uint8_t *> rows;
  rows.resize(h);
  int const stride = w * 4;
  for(int y = 0; y < h; ++y) {
    rows[y] = img.data() + (stride * y);
  }
  png_set_rows(p, info_ptr, rows.data());
  util::StreamWriter out;
  png_set_write_fn(p, &out, png_write_callback, nullptr);
  png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
  png_destroy_write_struct(&p, nullptr);
  auto result = util::writeFile(filename, out.buffer());
  return std::move(result);
}


int main(int argc, char** argv) {
  util::Logger log(stdout, stderr, util::Logger::DEBUG);
  if(argc <= 2) {
    log.error("usage: avif-decoder <filename>.avif <filename>.{bmp, png}");
    return -1;
  }
  std::string inputFilename = std::string(argv[1]);
  std::string outputFilename = std::string(argv[2]);
  if(inputFilename == outputFilename) {
    log.error("usage: avif-decoder <filename>.avif <filename>.{bmp, png}");
    return -1;
  }

  log.debug("dav1d: %s", dav1d_version());

  // Init dav1d
  Dav1dSettings settings{};
  dav1d_default_settings(&settings);
  Dav1dContext* ctx{};
  int err = dav1d_open(&ctx, &settings);
  if(err != 0) {
    log.error("Failed to open dav1d: %d\n", err);
    return -1;
  }

  // Read file.
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
  // FIXME(ledyba-z): handle multiple pixtures
  size_t const baseOffset = fileBox->metaBox.itemLocationBox.items[0].baseOffset;
  size_t const extentOffset = fileBox->metaBox.itemLocationBox.items[0].extents[0].extentOffset;
  size_t const extentLength = fileBox->metaBox.itemLocationBox.items[0].extents[0].extentLength;
  auto avifBegin = parser.buffer().cbegin();
  auto imgBegin = std::next(avifBegin, baseOffset + extentOffset);
  auto imgEnd = std::next(imgBegin, extentLength);
  // FIXME(ledyba-z): avoid copy
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

  // Write to file.

  if(endsWidh(outputFilename, ".bmp")) {
    std::vector<uint8_t> buff = convertToARGB(pic);
    auto result = writeBitmap(log, outputFilename, buff, pic.p.w, pic.p.h);
    if(result.has_value()){
      log.error("Failed to write Bitmap: %s", result.value());
      return -1;
    }
  } else if(endsWidh(outputFilename, ".png")) {
    std::vector<uint8_t> buff = convertToABGR(pic);
    auto result = writePNG(log, outputFilename, buff, pic.p.w, pic.p.h);
    if(result.has_value()){
      log.error("Failed to write PNG: %s", result.value());
      return -1;
    }
  } else {
    log.error("Unknown file extension: %s", outputFilename);
    return -1;
  }

  dav1d_picture_unref(&pic);
  dav1d_close(&ctx);
  return 0;
}
