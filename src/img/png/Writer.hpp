//
// Created by psi on 2020/02/16.
//

#pragma once


#include <string>
#include "avif/util/Logger.hpp"
#include "avif/img/Image.hpp"
#include "png.h"
#include "avif/util/File.hpp"
#include "avif/util/StreamWriter.hpp"

namespace img::png {

class Writer final {
private:
  avif::util::Logger& log_;
  std::string filename_;
public:
  Writer() = delete;
  Writer(Writer const&) = delete;
  Writer(Writer&&) = delete;
  Writer& operator=(Writer const&) = delete;
  Writer& operator=(Writer&&) = delete;
  ~Writer() noexcept = default;
public:
  explicit Writer(avif::util::Logger& log, std::string filename) noexcept
  :log_(log)
  ,filename_(std::move(filename)){
  }
  static char const* version();
  template <size_t BitsPerComponent>
  std::optional<std::string> write(avif::img::Image<BitsPerComponent>& img) {
    //FIXME(ledyba-z): add error handling
    const size_t w = img.width();
    const size_t h = img.height();
    const size_t stride = img.stride();
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);
    int color_type = PNG_COLOR_TYPE_GRAY;
    switch(img.pixelOrder()) {
      case avif::img::PixelOrder::RGB:
        color_type = PNG_COLOR_TYPE_RGB;
        break;
      case avif::img::PixelOrder::RGBA:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
      case avif::img::PixelOrder::Mono:
        color_type = PNG_COLOR_TYPE_GRAY;
        break;
      case avif::img::PixelOrder::MonoA:
        color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
    }
    png_set_IHDR(png, info, w, h, BitsPerComponent,
                 color_type,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    auto const& colorProfile = img.colorProfile();

    if(colorProfile.iccProfile.has_value()){
      avif::img::ICCProfile const& icc = colorProfile.iccProfile.value();
      png_set_benign_errors(png, 1);
      png_set_iCCP(png, info, "ICC Profile", 0, icc.payload().data(), icc.payload().size());
      png_set_benign_errors(png, 0);
    } else if(colorProfile.cicp.has_value()) {
      avif::ColourInformationBox::CICP const& cicp = colorProfile.cicp.value();
      if (cicp.colourPrimaries == 1 && cicp.transferCharacteristics == 13) {
        // TODO(ledyba-z): How about intent?
        png_set_sRGB_gAMA_and_cHRM(png, info, PNG_sRGB_INTENT_PERCEPTUAL);
      }
    }

    std::vector<uint8_t *> rows;
    rows.resize(h);
    for(int y = 0; y < h; ++y) {
      rows[y] = img.data() + (stride * y);
    }
    png_set_rows(png, info, rows.data());
    avif::util::StreamWriter out;
    png_set_write_fn(png, &out, Writer::png_write_callback_, nullptr);
    png_write_png(png, info, BitsPerComponent == 16 ? PNG_TRANSFORM_SWAP_ENDIAN : PNG_TRANSFORM_IDENTITY, nullptr);
    png_destroy_write_struct(&png, nullptr);
    auto result = avif::util::writeFile(filename_, out.buffer());
    return result;
  }

private:
  static void png_write_callback_(png_structp png_ptr, png_bytep data, png_size_t length);
};

}
