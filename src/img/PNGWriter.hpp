//
// Created by psi on 2020/02/16.
//

#pragma once


#include <string>
#include <avif/util/Logger.hpp>
#include <avif/img/Image.hpp>
#include <png.h>
#include <avif/util/File.hpp>
#include <avif/util/StreamWriter.hpp>

class PNGWriter final {
private:
  avif::util::Logger& log_;
  std::string filename_;
public:
  PNGWriter() = delete;
  PNGWriter(PNGWriter const&) = delete;
  PNGWriter(PNGWriter&&) = delete;
  PNGWriter& operator=(PNGWriter const&) = delete;
  PNGWriter& operator=(PNGWriter&&) = delete;
  ~PNGWriter() noexcept = default;
public:
  explicit PNGWriter(avif::util::Logger& log, std::string filename) noexcept
  :log_(log)
  ,filename_(std::move(filename)){
  }
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

    if(std::holds_alternative<avif::img::ICCProfile>(img.colorProfile())){
      avif::img::ICCProfile const& icc = std::get<avif::img::ICCProfile>(img.colorProfile());
      png_set_benign_errors(png, 1);
      png_set_iCCP(png, info, "ICC Profile", 0, icc.payload().data(), icc.payload().size());
      png_set_benign_errors(png, 0);
    }
    std::vector<uint8_t *> rows;
    rows.resize(h);
    for(int y = 0; y < h; ++y) {
      rows[y] = img.data() + (stride * y);
    }
    png_set_rows(png, info, rows.data());
    avif::util::StreamWriter out;
    png_set_write_fn(png, &out, PNGWriter::png_write_callback_, nullptr);
    png_write_png(png, info, BitsPerComponent == 16 ? PNG_TRANSFORM_SWAP_ENDIAN : PNG_TRANSFORM_IDENTITY, nullptr);
    png_destroy_write_struct(&png, nullptr);
    auto result = avif::util::writeFile(filename_, out.buffer());
    return result;
  }

private:
  static void png_write_callback_(png_structp  png_ptr, png_bytep data, png_size_t length);
};


