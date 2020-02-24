//
// Created by psi on 2020/02/16.
//

#include <fmt/format.h>
#include <avif/img/Conversion.hpp>
#include "Conversion.hpp"


template <size_t rgbBits, size_t yuvBits, bool toMonoRGB, bool isFullRange>
void writeImage(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch(src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToRGB<rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI400(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0]);
      break;
    case DAV1D_PIXEL_LAYOUT_I420:
      avif::img::ToRGB<rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI420(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I422:
      avif::img::ToRGB<rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI422(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I444:
      avif::img::ToRGB<rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI444(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
  }
}

template <size_t rgbBits, size_t yuvBits, bool toMonoRGB>
void writeImage(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  if(src.seq_hdr->color_range == 0) {
    writeImage<rgbBits, yuvBits, toMonoRGB, false>(dst, src);
  } else {
    writeImage<rgbBits, yuvBits, toMonoRGB, true>(dst, src);
  }

}

template <size_t rgbBits, size_t yuvBits, bool isFullRange>
void writeAlpha(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch(src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToAlpha<rgbBits, yuvBits, isFullRange>().fromI400(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0]);
      break;
    default:
      throw std::domain_error("Alpha image must be monochrome");
  }
}

template <size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGB(Dav1dPicture& primary) {
  avif::img::PixelOrder const pixelOrder = primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::Mono : avif::img::PixelOrder::RGB;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if(img.isMonochrome()) {
    writeImage<rgbBits, yuvBits, true>(img, primary);
  } else {
    writeImage<rgbBits, yuvBits, false>(img, primary);
  }
  return std::move(img);
}

template <size_t rgbBits, size_t yuvBits, size_t monoBits>
avif::img::Image<rgbBits> convertToRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  avif::img::PixelOrder const pixelOrder = primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::MonoA : avif::img::PixelOrder::RGBA;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if(primary.seq_hdr->color_range == 0) {
    writeImage<rgbBits, yuvBits, false>(img, primary);
  } else {
    writeImage<rgbBits, yuvBits, true>(img, primary);
  }

  if(alpha.seq_hdr->color_range == 0) {
    writeAlpha<rgbBits, yuvBits, false>(img, alpha);
  } else {
    writeAlpha<rgbBits, yuvBits, true>(img, alpha);
  }
  return std::move(img);
}

template <size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  switch(alpha.p.bpc) {
    case 8:
      return convertToRGBA<rgbBits, 8, 8>(primary, alpha);
    case 10:
      return convertToRGBA<rgbBits, 10, 8>(primary, alpha);
    case 12:
      return convertToRGBA<rgbBits, 12, 8>(primary, alpha);
    default:
      throw std::runtime_error(fmt::format("Unknwon alpha bpc={}", alpha.p.bpc));
  }
}

std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(Dav1dPicture& primary, std::optional<Dav1dPicture>& alpha) {
  if(!alpha.has_value()) {
    switch(primary.p.bpc) {
      case 8:
        return convertToRGB<8, 8>(primary);
      case 10:
        return convertToRGB<16, 10>(primary);
      case 12:
        return convertToRGB<16, 12>(primary);
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
    }
  } else {
    if(primary.p.bpc == 8 && alpha.value().p.bpc == 8) {
      switch(primary.p.bpc) {
        case 8:
          return convertToRGBA<8, 8>(primary, alpha.value());
        case 10:
          return convertToRGBA<8, 10>(primary, alpha.value());
        case 12:
          return convertToRGBA<8, 12>(primary, alpha.value());
        default:
          throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
      }
    } else {
      switch(primary.p.bpc) {
        case 8:
          return convertToRGBA<16, 8>(primary, alpha.value());
        case 10:
          return convertToRGBA<16, 10>(primary, alpha.value());
        case 12:
          return convertToRGBA<16, 12>(primary, alpha.value());
        default:
          throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
      }
    }
  }
}