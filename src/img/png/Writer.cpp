//
// Created by psi on 2020/02/16.
//

#include "Writer.hpp"

namespace img::png {

void Writer::png_write_callback_(png_structp png_ptr, png_bytep data, png_size_t length) {
  auto buff = reinterpret_cast<avif::util::StreamWriter *>(png_get_io_ptr(png_ptr));
  buff->append(data, length);
}

char const *Writer::version() {
  return PNG_LIBPNG_VER_STRING;
}

}
