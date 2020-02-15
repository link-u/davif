//
// Created by psi on 2020/02/16.
//

#include "PNGWriter.hpp"

void PNGWriter::png_write_callback_(png_structp  png_ptr, png_bytep data, png_size_t length) {
  auto buff = reinterpret_cast<avif::util::StreamWriter*>(png_get_io_ptr(png_ptr));
  buff->append(data, length);
}

