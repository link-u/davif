#include <vector>
#include <filesystem>
#include <optional>

#include <dav1d/dav1d.h>
#include <png.h>

#include <avif/util/File.hpp>
#include <avif/util/Logger.hpp>
#include <avif/util/FileLogger.hpp>
#include <avif/util/StreamWriter.hpp>
#include <avif/Parser.hpp>
#include <avif/img/Image.hpp>
#include <avif/img/Conversion.hpp>
#include <avif/img/Crop.hpp>
#include <avif/img/Transform.hpp>
#include <thread>

#include "../external/clipp/include/clipp.h"

namespace {

bool endsWidh(std::string const& target, std::string const& suffix) {
  if(target.size() < suffix.size()) {
    return false;
  }
  return target.substr(target.size()-suffix.size()) == suffix;
}

std::string basename(std::string const& path) {
  auto pos = path.find_last_of('/');
  if(pos == std::string::npos) {
    return path;
  }
  return path.substr(pos+1);
}

void nop_free_callback(const uint8_t *buf, void *cookie) {
}

std::variant<avif::img::Image<8>, avif::img::Image<16>> convertToRGB(Dav1dPicture& pic) {
  std::variant<avif::img::Image<8>, avif::img::Image<16>> result;
  switch(pic.p.bpc) {
    case 8: {
      avif::img::Image<8> img = avif::img::Image<8>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
      switch(pic.p.layout) {
        case DAV1D_PIXEL_LAYOUT_I400:
        case DAV1D_PIXEL_LAYOUT_I420:
          avif::img::ToRGB<8, 8>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
        case DAV1D_PIXEL_LAYOUT_I422:
          avif::img::ToRGB<8, 8>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
        case DAV1D_PIXEL_LAYOUT_I444:
          avif::img::ToRGB<8, 8>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
      }
      return img;
    }
    case 10: {
      avif::img::Image<16> img = avif::img::Image<16>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
      switch(pic.p.layout) {
        case DAV1D_PIXEL_LAYOUT_I400:
        case DAV1D_PIXEL_LAYOUT_I420:
          avif::img::ToRGB<16, 10>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
        case DAV1D_PIXEL_LAYOUT_I422:
          avif::img::ToRGB<16, 10>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
        case DAV1D_PIXEL_LAYOUT_I444:
          avif::img::ToRGB<16, 10>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
      }
      return img;
    }
    case 12: {
      avif::img::Image<16> img = avif::img::Image<16>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
      switch(pic.p.layout) {
        case DAV1D_PIXEL_LAYOUT_I400:
        case DAV1D_PIXEL_LAYOUT_I420:
          avif::img::ToRGB<16, 12>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
        case DAV1D_PIXEL_LAYOUT_I422:
          avif::img::ToRGB<16, 12>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
        case DAV1D_PIXEL_LAYOUT_I444:
          avif::img::ToRGB<16, 12>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
          break;
      }
      return img;
    }
    default:
      throw std::runtime_error(tfm::format("Unknwon bpc=%d", pic.p.bpc));
  }
}

void png_write_callback(png_structp  png_ptr, png_bytep data, png_size_t length) {
  auto buff = reinterpret_cast<avif::util::StreamWriter*>(png_get_io_ptr(png_ptr));
  buff->append(data, length);
}

template <size_t BitsPerComponent>
std::optional<std::string> writePNG(avif::util::Logger& log, std::string const& filename, avif::img::Image<BitsPerComponent>& img) {
  //FIXME(ledyba-z): add error handling
  const size_t w = img.width();
  const size_t h = img.height();
  const size_t stride = img.stride();
  png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr = png_create_info_struct(p);
  png_set_IHDR(p, info_ptr, w, h, BitsPerComponent,
               img.pixelOrder() == avif::img::PixelOrder::RGBA ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  std::vector<uint8_t *> rows;
  rows.resize(h);
  for(int y = 0; y < h; ++y) {
    rows[y] = img.data() + (stride * y);
  }
  png_set_rows(p, info_ptr, rows.data());
  avif::util::StreamWriter out;
  png_set_write_fn(p, &out, png_write_callback, nullptr);
  png_write_png(p, info_ptr, BitsPerComponent == 16 ? PNG_TRANSFORM_SWAP_ENDIAN : PNG_TRANSFORM_IDENTITY, nullptr);
  png_destroy_write_struct(&p, nullptr);
  auto result = avif::util::writeFile(filename, out.buffer());
  return std::move(result);
}

template <typename T>
std::optional<T> findBox(avif::FileBox const& fileBox, uint32_t itemID) {
  for(auto const& assoc : fileBox.metaBox.itemPropertiesBox.associations){
    for(auto const& item : assoc.items) {
      if(item.itemID != itemID) {
        continue;
      }
      for(auto const& entry : item.entries) {
        if(entry.propertyIndex == 0) {
          continue;
        }
        auto& prop = fileBox.metaBox.itemPropertiesBox.propertyContainers.properties.at(entry.propertyIndex - 1);
        if(std::holds_alternative<T>(prop)){
          return std::get<T>(prop);
        }
      }
    }
  }
  return std::optional<T>();
}

template <size_t BitsPerComponent>
avif::img::Image<BitsPerComponent> applyTransform(avif::img::Image<BitsPerComponent> img, avif::FileBox const& fileBox) {
  uint32_t itemID = 1;
  if(fileBox.metaBox.primaryItemBox.has_value()) {
    itemID = fileBox.metaBox.primaryItemBox.value().itemID;
  }
  std::optional<avif::CleanApertureBox> clap = findBox<avif::CleanApertureBox>(fileBox, itemID);
  std::optional<avif::ImageRotationBox> irot = findBox<avif::ImageRotationBox>(fileBox, itemID);
  std::optional<avif::ImageMirrorBox> imir = findBox<avif::ImageMirrorBox>(fileBox, itemID);
  if(!clap.has_value() && !irot.has_value() && !imir.has_value()) {
    return std::move(img);
  }
  // ISO/IEC 23000-22:2019(E)
  // p.16
  // These properties, if used, shall be indicated to be applied in the following order:
  //  clean aperture first,
  //  then rotation,
  //  then mirror.
  if(clap.has_value()) {
    img = avif::img::crop(img, clap.value());
  }
  if(irot.has_value()) {
    img = avif::img::rotate(img, irot.value().angle);
  }
  if(imir.has_value()) {
    img = avif::img::flip(img, imir.value().axis);
  }
  return std::move(img);
}
}

static int _main(int argc, char** argv);

int main(int argc, char** argv) {
  try {
    return _main(argc, argv);
  } catch (std::exception& err) {
    fprintf(stderr, "%s\n", err.what());
    fflush(stderr);
    return -1;
  }
}

int _main(int argc, char** argv) {
  avif::util::FileLogger log(stdout, stderr, avif::util::Logger::DEBUG);

  log.info("davif");
  log.debug("dav1d ver: %s", dav1d_version());

  // Init dav1d
  Dav1dSettings settings{};
  dav1d_default_settings(&settings);
  settings.n_tile_threads = static_cast<int>(std::thread::hardware_concurrency());

  std::string inputFilename = {};
  std::string outputFilename = {};
  {
    using namespace clipp;
    auto cli = (
        required("-i", "--input") & value("input.avif", inputFilename),
        required("-o", "--output") & value("output.png", outputFilename),
        option("--threads") & integer("Num of threads to use", settings.n_tile_threads)
    );
    if(!parse(argc, argv, cli)) {
      std::cerr << make_man_page(cli, basename(std::string(argv[0])));
      return -1;
    }
    if(inputFilename == outputFilename) {
      std::cerr << make_man_page(cli, basename(std::string(argv[0])));
      return -1;
    }
  }

  Dav1dContext* ctx{};
  int err = dav1d_open(&ctx, &settings);
  if(err != 0) {
    log.fatal("Failed to open dav1d: %d\n", err);
  }

  // Read file.
  std::variant<std::vector<uint8_t>, std::string> avif_data = avif::util::readFile(inputFilename);
  if(std::holds_alternative<std::string>(avif_data)){
    log.fatal("Failed to open input: %s\n", std::get<1>(avif_data));
  }

  // parse ISOBMFF
  avif::Parser parser(log, std::move(std::get<0>(avif_data)));
  std::shared_ptr<avif::Parser::Result> res = parser.parse();
  if(!res->ok()){
    log.fatal("Failed to parse %s as avif: %s\n", inputFilename, res->error());
  }
  avif::FileBox const& fileBox = res->fileBox();

  log.info("Decoding: %s -> %s", inputFilename, outputFilename);
  // start decoding
  Dav1dData data{};
  Dav1dPicture pic{};
  size_t itemID = 0;
  if(fileBox.metaBox.primaryItemBox.has_value()) {
    itemID = fileBox.metaBox.primaryItemBox.value().itemID - 1;
  }
  // FIXME(ledyba-z): handle multiple pixtures
  size_t const baseOffset = fileBox.metaBox.itemLocationBox.items[itemID].baseOffset;
  size_t const extentOffset = fileBox.metaBox.itemLocationBox.items[itemID].extents[0].extentOffset;
  size_t const extentLength = fileBox.metaBox.itemLocationBox.items[itemID].extents[0].extentLength;
  auto const buffBegin = res->buffer().data();
  auto const imgBegin = std::next(buffBegin, baseOffset + extentOffset);
  auto const imgEnd = std::next(imgBegin, extentLength);
  {
    auto start = std::chrono::steady_clock::now();

    dav1d_data_wrap(&data, imgBegin, std::distance(imgBegin, imgEnd), nop_free_callback, nullptr);
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
    auto finish = std::chrono::steady_clock::now();
    log.info("Decoded in %d [ms]", std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count());
  }

  // Write to file.

  if(!endsWidh(outputFilename, ".png")) {
    log.fatal("Please give png file for output");
  }
  std::optional<std::string> writeResult;
  std::variant<avif::img::Image<8>, avif::img::Image<16>> encoded = convertToRGB(pic);
  if(std::holds_alternative<avif::img::Image<8>>(encoded)) {
    auto& img = std::get<avif::img::Image<8>>(encoded);
    img = applyTransform(std::move(img), fileBox);
    writeResult = writePNG(log, outputFilename, img);
  } else {
    auto& img = std::get<avif::img::Image<16>>(encoded);
    img = applyTransform(std::move(img), fileBox);
    writeResult = writePNG(log, outputFilename, img);
  }
  if(writeResult.has_value()) {
    log.fatal("Failed to write PNG: %s", writeResult.value());
  }

  dav1d_picture_unref(&pic);
  dav1d_close(&ctx);
  return 0;
}
