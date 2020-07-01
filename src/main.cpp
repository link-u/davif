#include <vector>
#include <filesystem>
#include <optional>

#include <dav1d/dav1d.h>
#include <avif/util/Logger.hpp>
#include <avif/util/FileLogger.hpp>
#include <avif/util/File.hpp>
#include <avif/Constants.hpp>
#include <avif/Parser.hpp>
#include <avif/img/Image.hpp>
#include <avif/img/Conversion.hpp>
#include <avif/img/Crop.hpp>
#include <avif/img/Transform.hpp>
#include <thread>
#include <avif/Query.hpp>
#include <iostream>

#include "../external/clipp/include/clipp.h"
#include "img/PNGWriter.hpp"
#include "img/Conversion.hpp"

using MatrixType = avif::img::MatrixCoefficients;

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

void log_callback(void *cookie, const char *format, va_list ap) {
  auto* const log = static_cast<avif::util::Logger *>(cookie);
  auto const len = vsnprintf(nullptr, 0, format, ap);
  std::string buff;
  buff.resize(len);
  vsnprintf(buff.data(), len, format, ap);
  log->info("[dav1d] {}", buff);
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

avif::img::ColorProfile calcColorProfile(avif::FileBox const& fileBox, uint32_t const itemID, Dav1dPicture const& pic) {
  namespace query = avif::util::query;
  std::optional<avif::ColourInformationBox> colr = query::findProperty<avif::ColourInformationBox>(fileBox, itemID);
  if(colr.has_value()) {
    auto profile = colr.value().profile;
    if(std::holds_alternative<avif::ColourInformationBox::RestrictedICC>(profile)) {
      return avif::img::ICCProfile(std::get<avif::ColourInformationBox::RestrictedICC>(profile).payload);
    }else if(std::holds_alternative<avif::ColourInformationBox::UnrestrictedICC>(profile)) {
      return avif::img::ICCProfile(std::get<avif::ColourInformationBox::UnrestrictedICC>(profile).payload);
    }else if(std::holds_alternative<avif::ColourInformationBox::NCLX>(profile)) {
      return std::get<avif::ColourInformationBox::NCLX>(profile);
    }
  }
  return avif::ColourInformationBox::NCLX {
    .colourPrimaries = static_cast<uint16_t>(pic.seq_hdr->pri),
    .transferCharacteristics = static_cast<uint16_t>(pic.seq_hdr->trc),
    .matrixCoefficients = static_cast<uint16_t>(pic.seq_hdr->mtrx),
    .fullRangeFlag = pic.seq_hdr->color_range == 1,
  };
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
    return img;
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
  return img;
}
}

unsigned int decodeImageAt(avif::util::Logger& log, std::shared_ptr<avif::Parser::Result> const& res, uint32_t const itemID, Dav1dContext* ctx, Dav1dPicture& pic) {
  Dav1dData data{};
  auto const buffBegin = res->buffer().data();
  avif::FileBox const& fileBox = res->fileBox();
  size_t const baseOffset = fileBox.metaBox.itemLocationBox.items.at(itemID - 1).baseOffset;
  size_t const extentOffset = fileBox.metaBox.itemLocationBox.items.at(itemID - 1).extents[0].extentOffset;
  size_t const extentLength = fileBox.metaBox.itemLocationBox.items.at(itemID - 1).extents[0].extentLength;
  auto const imgBegin = std::next(buffBegin, baseOffset + extentOffset);
  auto const imgEnd = std::next(imgBegin, extentLength);
  {
    auto start = std::chrono::steady_clock::now();

    dav1d_data_wrap(&data, imgBegin, std::distance(imgBegin, imgEnd), nop_free_callback, nullptr);
    int err = dav1d_send_data(ctx, &data);

    if(err < 0) {
      log.fatal( "Failed to send data to dav1d: {}\n", err);
    }

    err = dav1d_get_picture(ctx, &pic);
    if (err < 0) {
      log.error("Failed to decode dav1d: {}\n", err);
    }
    auto finish = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count();
  }
}

static void saveImage(avif::util::Logger& log, std::string const& dstPath, avif::FileBox const& fileBox, Dav1dPicture& primary, avif::img::ColorProfile primaryProfile, std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha) {
  if(!endsWidh(dstPath, ".png")) {
    log.fatal("Please give png file for output: {}", dstPath);
  }

  std::optional<std::string> writeResult;
  std::variant<avif::img::Image<8>, avif::img::Image<16>> encoded = createImage(primary, primaryProfile, alpha);
  if(std::holds_alternative<avif::img::Image<8>>(encoded)) {
    auto& img = std::get<avif::img::Image<8>>(encoded);
    img = applyTransform(std::move(img), fileBox);
    img.colorProfile() = primaryProfile;
    writeResult = PNGWriter(log, dstPath).write(img);
  } else {
    auto& img = std::get<avif::img::Image<16>>(encoded);
    img = applyTransform(std::move(img), fileBox);
    img.colorProfile() = primaryProfile;
    writeResult = PNGWriter(log, dstPath).write(img);
  }
  if(writeResult.has_value()) {
    log.fatal("Failed to write PNG: {}", writeResult.value());
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
  log.debug(" - dav1d ver: {}", dav1d_version());

  // Init dav1d
  Dav1dSettings settings{};
  dav1d_default_settings(&settings);
  settings.logger.cookie = &log;
  settings.logger.callback = log_callback;
  settings.n_tile_threads = static_cast<int>(std::thread::hardware_concurrency());

  std::string inputFilename = {};
  std::string outputFilename = {};
  std::optional<std::string> outputAlphaFilename = {};
  std::optional<std::string> outputDepthFilename = {};
  bool showHelp = false;
  {
    using namespace clipp;
    auto convertFlags = (
        required("-i", "--input") & value("input.avif", inputFilename),
        required("-o", "--output") & value("output.png", outputFilename),
        option("--extract-alpha") & value("output-alpha.png").call([&](std::string const& path){ outputAlphaFilename = path; }),
        option("--extract-depth") & value("output-depth.png").call([&](std::string const& path){ outputDepthFilename = path; }),
        option("--threads") & integer("Num of threads to use", settings.n_tile_threads)
    );
    auto supportFlags = (
        option("-h", "--help").doc("Show help and exit.").set(showHelp, true)
    );
    auto cli = (convertFlags | supportFlags);
    if(parse(argc, argv, cli).any_error()) {
      std::cerr << make_man_page(cli, basename(std::string(argv[0])));
      return -1;
    }
    if(showHelp) {
      std::cerr << make_man_page(cli, basename(std::string(argv[0])));
      return 0;
    }
    if(inputFilename == outputFilename) {
      std::cerr << "Input and output can't be the same file!" << std::endl;
      return -1;
    }
  }

  Dav1dContext* ctx{};
  int err = dav1d_open(&ctx, &settings);
  if(err != 0) {
    log.fatal("Failed to open dav1d: {}\n", err);
  }

  // Read file.
  std::variant<std::vector<uint8_t>, std::string> avif_data = avif::util::readFile(inputFilename);
  if(std::holds_alternative<std::string>(avif_data)){
    log.fatal("Failed to open input: {}\n", std::get<1>(avif_data));
  }

  // parse ISOBMFF
  avif::Parser parser(log, std::move(std::get<0>(avif_data)));
  std::shared_ptr<avif::Parser::Result> res = parser.parse();
  if (!res->ok()) {
    log.fatal("Failed to parse {} as avif: {}\n", inputFilename, res->error());
  }

  log.info("Decoding: {} -> {}", inputFilename, outputFilename);
  // start decoding
  avif::FileBox const& fileBox = res->fileBox();
  Dav1dPicture primaryImg{};
  std::optional<Dav1dPicture> alphaImg{};
  namespace query = avif::util::query;
  uint32_t const primaryImageID = query::findPrimaryItemID(fileBox).value_or(1);
  { // primary image
    unsigned int elapsed = decodeImageAt(log, res, primaryImageID, ctx, primaryImg);
    log.info(" Decoded: {} -> {} in {} [ms]", inputFilename, outputFilename, elapsed);
  }
  auto primaryItemID = query::findPrimaryItemID(fileBox).value_or(1);
  avif::img::ColorProfile primaryProfile = calcColorProfile(fileBox, primaryItemID, primaryImg);
  avif::img::ColorProfile alphaProfile = {};

  { // alpha image
    std::optional<uint32_t> alphaID = query::findAuxItemID(fileBox, primaryImageID, avif::kAlphaAuxType());
    if(alphaID.has_value()) {
      alphaImg = Dav1dPicture{};
      unsigned int elapsed = decodeImageAt(log, res, alphaID.value(), ctx, alphaImg.value());
      log.info(" Decoded: {} -> {} in {} [ms] (Alpha image)", inputFilename, outputFilename, elapsed);
      if(alphaImg.value().p.w != primaryImg.p.w || alphaImg.value().p.h != primaryImg.p.h) {
        // TODO(ledyba-z): It is okay to alpha image and primary image are different sizes.
        //  see: https://github.com/AOMediaCodec/av1-avif/issues/68
        log.fatal("Currently, alpha image whose size ({} x {}) does not match to primary image ({} x {}) is not supported.",
                  alphaImg.value().p.w, alphaImg.value().p.h, primaryImg.p.w, primaryImg.p.h);
      }
      if(outputAlphaFilename.has_value()) {
        alphaProfile = calcColorProfile(fileBox, alphaID.value(), alphaImg.value());
        saveImage(log, outputAlphaFilename.value(), fileBox, alphaImg.value(), alphaProfile, std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>>());
        log.info(" Extracted: {} -> {} (Alpha image)", inputFilename, outputAlphaFilename.value());
      }
    } else {
      if(outputAlphaFilename.has_value()) {
        log.fatal("{} does not have alpha plane.", inputFilename);
      }
    }
  }
  { // depth image
    std::optional<uint32_t> depthID = query::findAuxItemID(fileBox, primaryImageID, avif::kDepthAuxType());
    if(depthID.has_value()) {
      Dav1dPicture depthImg = {};
      unsigned int elapsed = decodeImageAt(log, res, depthID.value(), ctx, depthImg);
      log.info(" Decoded: {} in {} [ms] (Depth image)", inputFilename, elapsed);
      if(depthImg.p.w != primaryImg.p.w || depthImg.p.h != primaryImg.p.h) {
        // TODO(ledyba-z): Can alpha image and primary image be different size?
        //  see: https://github.com/AOMediaCodec/av1-avif/issues/68
        log.fatal("Alpha size ({} x {}) does not match to primary image({} x {}).",
                  depthImg.p.w, depthImg.p.h, primaryImg.p.w, primaryImg.p.h);
      }
      if(outputDepthFilename.has_value()) {
        auto depthProfile = calcColorProfile(fileBox, depthID.value(), depthImg);
        saveImage(log, outputDepthFilename.value(), fileBox, depthImg, depthProfile, std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>>());
        log.info(" Extracted: {} -> {} (Depth image)", inputFilename, outputDepthFilename.value());
      }
    }else{
      if(outputDepthFilename.has_value()) {
        log.fatal("{} does not have depth plane.", inputFilename);
      }
    }
  }
  // Write to file.
  if(alphaImg.has_value()) {
    std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha = std::make_optional(std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>(alphaImg.value(), alphaProfile));
    saveImage(log, outputFilename, fileBox, primaryImg, primaryProfile, alpha);
  } else {
    saveImage(log, outputFilename, fileBox, primaryImg, primaryProfile, std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>>());
  }

  dav1d_picture_unref(&primaryImg);
  if(alphaImg.has_value()) {
    dav1d_picture_unref(&alphaImg.value());
  }
  dav1d_close(&ctx);
  return 0;
}
