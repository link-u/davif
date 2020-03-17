//
// Created by psi on 2020/02/16.
//

#include <fmt/format.h>
#include "Conversion.hpp"

using MatrixType = avif::img::MatrixCoefficients;

template <MatrixType matrixType, size_t rgbBits, size_t yuvBits, bool toMonoRGB, bool isFullRange>
void writeImage(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch(src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToRGB<matrixType, rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI400(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0]);
      break;
    case DAV1D_PIXEL_LAYOUT_I420:
      avif::img::ToRGB<matrixType, rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI420(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I422:
      avif::img::ToRGB<matrixType, rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI422(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I444:
      avif::img::ToRGB<matrixType, rgbBits, yuvBits, toMonoRGB, isFullRange>().fromI444(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
  }
}

template <MatrixType matrixType, size_t rgbBits, size_t yuvBits, bool toMonoRGB>
void writeImage(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  if(src.seq_hdr->color_range == 0) {
    writeImage<matrixType, rgbBits, yuvBits, toMonoRGB, false>(dst, src);
  } else {
    writeImage<matrixType, rgbBits, yuvBits, toMonoRGB, true>(dst, src);
  }

}

template <MatrixType matrixType, size_t rgbBits, size_t yuvBits, bool isFullRange>
void writeAlpha(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch(src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToAlpha<matrixType, rgbBits, yuvBits, isFullRange>().fromI400(dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0]);
      break;
    default:
      throw std::domain_error("Alpha image must be monochrome");
  }
}

template <MatrixType matrixType, size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGB(Dav1dPicture& primary) {
  avif::img::PixelOrder const pixelOrder = primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::Mono : avif::img::PixelOrder::RGB;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if(img.isMonochrome()) {
    writeImage<matrixType, rgbBits, yuvBits, true>(img, primary);
  } else {
    writeImage<matrixType, rgbBits, yuvBits, false>(img, primary);
  }
  return std::move(img);
}

template <MatrixType matrixTypeRGB, MatrixType matrixTypeA, size_t rgbBits, size_t yuvBits, size_t monoBits>
avif::img::Image<rgbBits> convertToRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  avif::img::PixelOrder const pixelOrder = primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::MonoA : avif::img::PixelOrder::RGBA;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if(primary.seq_hdr->color_range == 0) {
    writeImage<matrixTypeRGB, rgbBits, yuvBits, false>(img, primary);
  } else {
    writeImage<matrixTypeRGB, rgbBits, yuvBits, true>(img, primary);
  }

  if(alpha.seq_hdr->color_range == 0) {
    writeAlpha<matrixTypeA, rgbBits, monoBits, false>(img, alpha);
  } else {
    writeAlpha<matrixTypeA, rgbBits, monoBits, true>(img, alpha);
  }
  return std::move(img);
}

template <MatrixType matrixTypeRGB, MatrixType matrixTypeA, size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  switch(alpha.p.bpc) {
    case 8:
      return convertToRGBA<matrixTypeRGB, matrixTypeA, rgbBits, yuvBits, 8>(primary, alpha);
    case 10:
      return convertToRGBA<matrixTypeRGB, matrixTypeA, rgbBits, yuvBits, 10>(primary, alpha);
    case 12:
      return convertToRGBA<matrixTypeRGB, matrixTypeA, rgbBits, yuvBits, 12>(primary, alpha);
    default:
      throw std::runtime_error(fmt::format("Unknwon alpha bpc={}", alpha.p.bpc));
  }
}

template <MatrixType matrixTypeRGB, MatrixType matrixTypeA>
    std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(Dav1dPicture& primary, std::optional<Dav1dPicture>& alpha) {
  if(!alpha.has_value()) {
    switch(primary.p.bpc) {
      case 8:
        return convertToRGB<matrixTypeRGB, 8, 8>(primary);
      case 10:
        return convertToRGB<matrixTypeRGB, 16, 10>(primary);
      case 12:
        return convertToRGB<matrixTypeRGB, 16, 12>(primary);
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
    }
  } else {
    if(primary.p.bpc == 8 && alpha.value().p.bpc == 8) {
      switch(primary.p.bpc) {
        case 8:
          return convertToRGBA<matrixTypeRGB, matrixTypeA, 8, 8>(primary, alpha.value());
        case 10:
          return convertToRGBA<matrixTypeRGB, matrixTypeA, 8, 10>(primary, alpha.value());
        case 12:
          return convertToRGBA<matrixTypeRGB, matrixTypeA, 8, 12>(primary, alpha.value());
        default:
          throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
      }
    } else {
      switch(primary.p.bpc) {
        case 8:
          return convertToRGBA<matrixTypeRGB, matrixTypeA, 16, 8>(primary, alpha.value());
        case 10:
          return convertToRGBA<matrixTypeRGB, matrixTypeA, 16, 10>(primary, alpha.value());
        case 12:
          return convertToRGBA<matrixTypeRGB, matrixTypeA, 16, 12>(primary, alpha.value());
        default:
          throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
      }
    }
  }
}

template <MatrixType matrixTypeRGB>
std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(
    Dav1dPicture& primary,
    std::optional<Dav1dPicture>& alpha, std::optional<MatrixType>& alphaMatrix)
{
  switch (alphaMatrix.value_or(MatrixType::MC_UNSPECIFIED)) {
    case MatrixType::MC_IDENTITY:
      return createImage<matrixTypeRGB, MatrixType::MC_IDENTITY>(primary, alpha);
    case MatrixType::MC_BT_709:
      return createImage<matrixTypeRGB, MatrixType::MC_BT_709>(primary, alpha);
    case MatrixType::MC_FCC:
      return createImage<matrixTypeRGB, MatrixType::MC_FCC>(primary, alpha);
    case MatrixType::MC_BT_470_B_G:
      return createImage<matrixTypeRGB, MatrixType::MC_BT_470_B_G>(primary, alpha);
    case MatrixType::MC_BT_601:
      return createImage<matrixTypeRGB, MatrixType::MC_BT_601>(primary, alpha);
    case MatrixType::MC_SMPTE_240:
      return createImage<matrixTypeRGB, MatrixType::MC_SMPTE_240>(primary, alpha);
    case MatrixType::MC_SMPTE_YCGCO:
      return createImage<matrixTypeRGB, MatrixType::MC_SMPTE_YCGCO>(primary, alpha);
    case MatrixType::MC_UNSPECIFIED:
    case MatrixType::MC_BT_2020_NCL:
      return createImage<matrixTypeRGB, MatrixType::MC_BT_2020_NCL>(primary, alpha);
    case MatrixType::MC_BT_2020_CL:
      return createImage<matrixTypeRGB, MatrixType::MC_BT_2020_CL>(primary, alpha);
    case MatrixType::MC_SMPTE_2085:
      return createImage<matrixTypeRGB, MatrixType::MC_SMPTE_2085>(primary, alpha);
    case MatrixType::MC_CHROMAT_NCL:
      return createImage<matrixTypeRGB, MatrixType::MC_CHROMAT_NCL>(primary, alpha);
    case MatrixType::MC_CHROMAT_CL:
      return createImage<matrixTypeRGB, MatrixType::MC_CHROMAT_CL>(primary, alpha);
    case MatrixType::MC_ICTCP:
      return createImage<matrixTypeRGB, MatrixType::MC_ICTCP>(primary, alpha);
    default:
      assert(false && "Unknown matrix coefficients");
  }
}

std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(
    Dav1dPicture& primary, MatrixType primaryMatrix,
    std::optional<Dav1dPicture>& alpha, std::optional<MatrixType>& alphaMatrix)
{
  switch (primaryMatrix) {
    case MatrixType::MC_IDENTITY:
      return createImage<MatrixType::MC_IDENTITY>(primary, alpha, alphaMatrix);
    case MatrixType::MC_BT_709:
      return createImage<MatrixType::MC_BT_709>(primary, alpha, alphaMatrix);
    case MatrixType::MC_FCC:
      return createImage<MatrixType::MC_FCC>(primary, alpha, alphaMatrix);
    case MatrixType::MC_BT_470_B_G:
      return createImage<MatrixType::MC_BT_470_B_G>(primary, alpha, alphaMatrix);
    case MatrixType::MC_BT_601:
      return createImage<MatrixType::MC_BT_601>(primary, alpha, alphaMatrix);
    case MatrixType::MC_SMPTE_240:
      return createImage<MatrixType::MC_SMPTE_240>(primary, alpha, alphaMatrix);
    case MatrixType::MC_SMPTE_YCGCO:
      return createImage<MatrixType::MC_SMPTE_YCGCO>(primary, alpha, alphaMatrix);
    case MatrixType::MC_UNSPECIFIED:
    case MatrixType::MC_BT_2020_NCL:
      return createImage<MatrixType::MC_BT_2020_NCL>(primary, alpha, alphaMatrix);
    case MatrixType::MC_BT_2020_CL:
      return createImage<MatrixType::MC_BT_2020_CL>(primary, alpha, alphaMatrix);
    case MatrixType::MC_SMPTE_2085:
      return createImage<MatrixType::MC_SMPTE_2085>(primary, alpha, alphaMatrix);
    case MatrixType::MC_CHROMAT_NCL:
      return createImage<MatrixType::MC_CHROMAT_NCL>(primary, alpha, alphaMatrix);
    case MatrixType::MC_CHROMAT_CL:
      return createImage<MatrixType::MC_CHROMAT_CL>(primary, alpha, alphaMatrix);
    case MatrixType::MC_ICTCP:
      return createImage<MatrixType::MC_ICTCP>(primary, alpha, alphaMatrix);
    default:
      assert(false && "Unknown matrix coefficients");
  }
}
