//
// Created by psi on 2020/02/16.
//

#include <fmt/format.h>
#include <avif/img/Conversion.hpp>
#include "Conversion.hpp"

std::variant<avif::img::Image<8>, avif::img::Image<16>> convertToRGB(Dav1dPicture& pic) {
  std::variant<avif::img::Image<8>, avif::img::Image<16>> result;
  if(pic.seq_hdr->color_range == 0) {
    switch(pic.p.bpc) {
      case 8: {
        avif::img::Image<8> img = avif::img::Image<8>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
        switch(pic.p.layout) {
          case DAV1D_PIXEL_LAYOUT_I400:
          case DAV1D_PIXEL_LAYOUT_I420:
            avif::img::ToRGB<8, 8, false>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I422:
            avif::img::ToRGB<8, 8, false>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I444:
            avif::img::ToRGB<8, 8, false>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
        }
        return img;
      }
      case 10: {
        avif::img::Image<16> img = avif::img::Image<16>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
        switch(pic.p.layout) {
          case DAV1D_PIXEL_LAYOUT_I400:
          case DAV1D_PIXEL_LAYOUT_I420:
            avif::img::ToRGB<16, 10, false>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I422:
            avif::img::ToRGB<16, 10, false>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I444:
            avif::img::ToRGB<16, 10, false>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
        }
        return img;
      }
      case 12: {
        avif::img::Image<16> img = avif::img::Image<16>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
        switch(pic.p.layout) {
          case DAV1D_PIXEL_LAYOUT_I400:
          case DAV1D_PIXEL_LAYOUT_I420:
            avif::img::ToRGB<16, 12, false>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I422:
            avif::img::ToRGB<16, 12, false>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I444:
            avif::img::ToRGB<16, 12, false>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
        }
        return img;
      }
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", pic.p.bpc));
    }
  } else {
    switch(pic.p.bpc) {
      case 8: {
        avif::img::Image<8> img = avif::img::Image<8>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
        switch(pic.p.layout) {
          case DAV1D_PIXEL_LAYOUT_I400:
          case DAV1D_PIXEL_LAYOUT_I420:
            avif::img::ToRGB<8, 8, true>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I422:
            avif::img::ToRGB<8, 8, true>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I444:
            avif::img::ToRGB<8, 8, true>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
        }
        return img;
      }
      case 10: {
        avif::img::Image<16> img = avif::img::Image<16>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
        switch(pic.p.layout) {
          case DAV1D_PIXEL_LAYOUT_I400:
          case DAV1D_PIXEL_LAYOUT_I420:
            avif::img::ToRGB<16, 10, true>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I422:
            avif::img::ToRGB<16, 10, true>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I444:
            avif::img::ToRGB<16, 10, true>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
        }
        return img;
      }
      case 12: {
        avif::img::Image<16> img = avif::img::Image<16>::createEmptyImage(avif::img::PixelOrder::RGB, pic.p.w, pic.p.h);
        switch(pic.p.layout) {
          case DAV1D_PIXEL_LAYOUT_I400:
          case DAV1D_PIXEL_LAYOUT_I420:
            avif::img::ToRGB<16, 12, true>().fromI420(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I422:
            avif::img::ToRGB<16, 12, true>().fromI422(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
          case DAV1D_PIXEL_LAYOUT_I444:
            avif::img::ToRGB<16, 12, true>().fromI444(img, reinterpret_cast<uint8_t*>(pic.data[0]), pic.stride[0], reinterpret_cast<uint8_t*>(pic.data[1]), pic.stride[1], reinterpret_cast<uint8_t*>(pic.data[2]), pic.stride[1]);
            break;
        }
        return img;
      }
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc=%d", pic.p.bpc));
    }
  }
}