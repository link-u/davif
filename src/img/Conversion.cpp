//
// Created by psi on 2020/02/16.
//

#include <fmt/format.h>
#include <avif/img/Conversion.hpp>
#include "Conversion.hpp"


template <size_t rgbBits, size_t yuvBits, bool isFullRange>
avif::img::Image<rgbBits> convertToRGB(Dav1dPicture& pic) {
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
  switch(pic.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToRGB<rgbBits, yuvBits, isFullRange>().fromI400(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0]);
      break;
    case DAV1D_PIXEL_LAYOUT_I420:
      avif::img::ToRGB<rgbBits, yuvBits, isFullRange>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I422:
      avif::img::ToRGB<rgbBits, yuvBits, isFullRange>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I444:
      avif::img::ToRGB<rgbBits, yuvBits, isFullRange>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
      break;
  }
  return img;
}
template <size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGB(Dav1dPicture& pic) {
  if(pic.seq_hdr->color_range == 0) {
    return convertToRGB<rgbBits, yuvBits, false>(pic);
  } else {
    return convertToRGB<rgbBits, yuvBits, true>(pic);
  }
}

std::variant<avif::img::Image<8>, avif::img::Image<16>> convertToRGB(Dav1dPicture& pic) {
  std::variant<avif::img::Image<8>, avif::img::Image<16>> result;
  switch(pic.p.bpc) {
    case 8:
      return convertToRGB<8, 8>(pic);
    case 10:
      return convertToRGB<16, 10>(pic);
    case 12:
      return convertToRGB<16, 12>(pic);
    default:
      throw std::runtime_error(fmt::format("Unknwon bpc={}", pic.p.bpc));
  }
}