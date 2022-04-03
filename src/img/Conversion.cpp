//
// Created by psi on 2020/02/16.
//

#include <fmt/format.h>
#include <avif/img/Conversion.hpp>
#include "Conversion.hpp"

namespace img {

template<typename Converter, size_t rgbBits, size_t yuvBits, bool toMonoRGB, bool isFullRange>
void writeImage(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch (src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToRGB<Converter, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI400(dst,
                                                                                     reinterpret_cast<uint8_t *>(src.data[0]),
                                                                                     src.stride[0]);
      break;
    case DAV1D_PIXEL_LAYOUT_I420:
      avif::img::ToRGB<Converter, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI420(dst,
                                                                                      reinterpret_cast<uint8_t *>(src.data[0]),
                                                                                      src.stride[0],
                                                                                      reinterpret_cast<uint8_t *>(src.data[1]),
                                                                                      src.stride[1],
                                                                                      reinterpret_cast<uint8_t *>(src.data[2]),
                                                                                      src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I422:
      avif::img::ToRGB<Converter, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI422(dst,
                                                                                      reinterpret_cast<uint8_t *>(src.data[0]),
                                                                                      src.stride[0],
                                                                                      reinterpret_cast<uint8_t *>(src.data[1]),
                                                                                      src.stride[1],
                                                                                      reinterpret_cast<uint8_t *>(src.data[2]),
                                                                                      src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I444:
      avif::img::ToRGB<Converter, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI444(dst,
                                                                                      reinterpret_cast<uint8_t *>(src.data[0]),
                                                                                      src.stride[0],
                                                                                      reinterpret_cast<uint8_t *>(src.data[1]),
                                                                                      src.stride[1],
                                                                                      reinterpret_cast<uint8_t *>(src.data[2]),
                                                                                      src.stride[1]);
      break;
  }
}

template<typename Converter, size_t rgbBits, size_t yuvBits, bool toMonoRGB>
void writeImage(avif::img::Image<rgbBits> &dst, Dav1dPicture &src) {
  if (src.seq_hdr->color_range == 0) {
    writeImage<Converter, rgbBits, yuvBits, toMonoRGB, false>(dst, src);
  } else {
    writeImage<Converter, rgbBits, yuvBits, toMonoRGB, true>(dst, src);
  }

}

template<typename Converter, size_t rgbBits, size_t yuvBits, bool isFullRange>
void writeAlpha(avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch (src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToAlpha<Converter, rgbBits, yuvBits, isFullRange>::fromI400(dst,
                                                                             reinterpret_cast<uint8_t *>(src.data[0]),
                                                                             src.stride[0]);
      break;
    default:
      throw std::domain_error("Alpha image must be monochrome");
  }
}

template<typename Converter, size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGB(Dav1dPicture& primary) {
  avif::img::PixelOrder const pixelOrder =
      primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ?
        avif::img::PixelOrder::Mono :
        avif::img::PixelOrder::RGB;

  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if (img.isMonochrome()) {
    writeImage<Converter, rgbBits, yuvBits, true>(img, primary);
  } else {
    writeImage<Converter, rgbBits, yuvBits, false>(img, primary);
  }
  return img;
}

template<typename ConverterRGB, typename ConverterA, size_t rgbBits, size_t yuvBits, size_t monoBits>
avif::img::Image<rgbBits>
convertToRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  avif::img::PixelOrder const pixelOrder =
      primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::MonoA : avif::img::PixelOrder::RGBA;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if (primary.seq_hdr->color_range == 0) {
    writeImage<ConverterRGB, rgbBits, yuvBits, false>(img, primary);
  } else {
    writeImage<ConverterRGB, rgbBits, yuvBits, true>(img, primary);
  }

  if (alpha.seq_hdr->color_range == 0) {
    writeAlpha<ConverterA, rgbBits, monoBits, false>(img, alpha);
  } else {
    writeAlpha<ConverterA, rgbBits, monoBits, true>( img, alpha);
  }
  return img;
}

template<typename ConverterRGB, typename ConverterA, size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits>
convertToRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  switch (alpha.p.bpc) {
    case 8:
      return convertToRGBA<ConverterRGB, ConverterA, rgbBits, yuvBits, 8>(primary, alpha);
    case 10:
      return convertToRGBA<ConverterRGB, ConverterA, rgbBits, yuvBits, 10>(primary, alpha);
    case 12:
      return convertToRGBA<ConverterRGB, ConverterA, rgbBits, yuvBits, 12>(primary, alpha);
    default:
      throw std::runtime_error(fmt::format("Unknwon alpha bpc={}", alpha.p.bpc));
  }
}

template<typename ConverterRGB>
std::variant<avif::img::Image<8>, avif::img::Image<16>>
createImageRGB(Dav1dPicture& primary) {
  switch (primary.p.bpc) {
    case 8:
      return convertToRGB<ConverterRGB, 8, 8>(primary);
    case 10:
      return convertToRGB<ConverterRGB, 16, 10>(primary);
    case 12:
      return convertToRGB<ConverterRGB, 16, 12>(primary);
    default:
      throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
  }
}

template<typename ConverterRGB, typename ConverterA>
std::variant<avif::img::Image<8>, avif::img::Image<16>>
createImageRGBA(Dav1dPicture& primary, Dav1dPicture& alpha) {
  if (primary.p.bpc == 8 && alpha.p.bpc == 8) {
    switch (primary.p.bpc) {
      case 8:
        return convertToRGBA<ConverterRGB, ConverterA, 8, 8>(primary, alpha);
      case 10:
        return convertToRGBA<ConverterRGB, ConverterA, 8, 10>(primary, alpha);
      case 12:
        return convertToRGBA<ConverterRGB, ConverterA, 8, 12>(primary, alpha);
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
    }
  } else {
    switch (primary.p.bpc) {
      case 8:
        return convertToRGBA<ConverterRGB, ConverterA, 16, 8>(primary, alpha);
      case 10:
        return convertToRGBA<ConverterRGB, ConverterA, 16, 10>(primary, alpha);
      case 12:
        return convertToRGBA<ConverterRGB, ConverterA, 16, 12>(primary, alpha);
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
    }
  }
}

template<typename ConverterRGB>
std::variant<
  avif::img::Image<8>,
  avif::img::Image<16>
> createImage(
  Dav1dPicture& primary,
  std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha
) {
  using avif::img::color::ConverterFactory;
  if (!alpha.has_value()) {
    return createImageRGB<ConverterRGB>(primary);
  }

  auto const& [alphaPic, alphaProf] = alpha.value();

  avif::ColourInformationBox::CICP cicp;
  if(alphaProf.cicp.has_value()) {
    cicp = alphaProf.cicp.value();
  }
  using avif::img::color::MatrixCoefficients;
  switch (static_cast<MatrixCoefficients>(cicp.matrixCoefficients)) {
    case MatrixCoefficients::MC_IDENTITY: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_IDENTITY>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_BT_709: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_709>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_FCC: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_FCC>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_UNSPECIFIED:
    case MatrixCoefficients::MC_BT_470_B_G: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_470_B_G>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_NSTC: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_NSTC>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_SMPTE_240: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_SMPTE_240>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_SMPTE_YCGCO: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_SMPTE_YCGCO>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_BT_2020_NCL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_2020_NCL>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_BT_2020_CL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_2020_CL>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_SMPTE_2085: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_SMPTE_2085>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_CHROMAT_NCL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_CHROMAT_NCL>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_CHROMAT_CL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_CHROMAT_CL>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    case MatrixCoefficients::MC_BT_2100_ICTCP: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_2100_ICTCP>;
      return createImageRGBA<ConverterRGB, converter>(primary, alphaPic);
    }
    default:
      assert(false && "Unknown matrix coefficients");
  }
}

std::variant<
  avif::img::Image<8>,
  avif::img::Image<16>
> createImage(
  Dav1dPicture& primary, avif::img::ColorProfile const& primaryProfile,
  std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha
) {

  avif::ColourInformationBox::CICP cicp = {};
  if(primaryProfile.cicp.has_value()) {
    cicp = primaryProfile.cicp.value();
  }

  using avif::img::color::MatrixCoefficients;
  switch (static_cast<MatrixCoefficients>(cicp.matrixCoefficients)) {
    case MatrixCoefficients::MC_IDENTITY: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_IDENTITY>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_BT_709: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_709>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_FCC: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_FCC>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_UNSPECIFIED:
    case MatrixCoefficients::MC_BT_470_B_G: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_470_B_G>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_NSTC: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_NSTC>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_SMPTE_240: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_SMPTE_240>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_SMPTE_YCGCO: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_SMPTE_YCGCO>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_BT_2020_NCL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_2020_NCL>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_BT_2020_CL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_2020_CL>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_SMPTE_2085: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_SMPTE_2085>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_CHROMAT_NCL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_CHROMAT_NCL>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_CHROMAT_CL: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_CHROMAT_CL>;
      return createImage<converter>(primary, alpha);
    }
    case MatrixCoefficients::MC_BT_2100_ICTCP: {
      using converter = avif::img::color::ConverterFactory<MatrixCoefficients::MC_BT_2100_ICTCP>;
      return createImage<converter>(primary, alpha);
    }
    default:
      assert(false && "Unknown matrix coefficients");
  }
}

}
