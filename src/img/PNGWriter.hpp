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
    png_set_write_fn(p, &out, PNGWriter::png_write_callback_, nullptr);
    png_write_png(p, info_ptr, BitsPerComponent == 16 ? PNG_TRANSFORM_SWAP_ENDIAN : PNG_TRANSFORM_IDENTITY, nullptr);
    png_destroy_write_struct(&p, nullptr);
    auto result = avif::util::writeFile(filename_, out.buffer());
    return std::move(result);
  }

private:
  static void png_write_callback_(png_structp  png_ptr, png_bytep data, png_size_t length);
};


