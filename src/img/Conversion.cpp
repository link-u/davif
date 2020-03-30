//
// Created by psi on 2020/02/16.
//

#include <fmt/format.h>
#include "Conversion.hpp"

template <typename ConverterImpl, size_t rgbBits, size_t yuvBits, bool toMonoRGB, bool isFullRange>
void writeImage(ConverterImpl const& converter, avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch(src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToRGB<ConverterImpl, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI400(converter, dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0]);
      break;
    case DAV1D_PIXEL_LAYOUT_I420:
      avif::img::ToRGB<ConverterImpl, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI420(converter, dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I422:
      avif::img::ToRGB<ConverterImpl, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI422(converter, dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
    case DAV1D_PIXEL_LAYOUT_I444:
      avif::img::ToRGB<ConverterImpl, rgbBits, yuvBits, toMonoRGB, isFullRange>::fromI444(converter, dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0], reinterpret_cast<uint8_t*>(src.data[1]), src.stride[1], reinterpret_cast<uint8_t*>(src.data[2]), src.stride[1]);
      break;
  }
}

template <typename ConverterImpl, size_t rgbBits, size_t yuvBits, bool toMonoRGB>
void writeImage(ConverterImpl const& converter, avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  if(src.seq_hdr->color_range == 0) {
    writeImage<ConverterImpl, rgbBits, yuvBits, toMonoRGB, false>(converter, dst, src);
  } else {
    writeImage<ConverterImpl, rgbBits, yuvBits, toMonoRGB, true>(converter, dst, src);
  }

}

template <typename ConverterImpl, size_t rgbBits, size_t yuvBits, bool isFullRange>
void writeAlpha(ConverterImpl const& converter, avif::img::Image<rgbBits>& dst, Dav1dPicture& src) {
  switch(src.p.layout) {
    case DAV1D_PIXEL_LAYOUT_I400:
      avif::img::ToAlpha<ConverterImpl, rgbBits, yuvBits, isFullRange>::fromI400(converter, dst, reinterpret_cast<uint8_t*>(src.data[0]), src.stride[0]);
      break;
    default:
      throw std::domain_error("Alpha image must be monochrome");
  }
}

template <typename ConverterImpl, size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGB(ConverterImpl const& converter, Dav1dPicture& primary) {
  avif::img::PixelOrder const pixelOrder = primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::Mono : avif::img::PixelOrder::RGB;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if(img.isMonochrome()) {
    writeImage<ConverterImpl, rgbBits, yuvBits, true>(converter, img, primary);
  } else {
    writeImage<ConverterImpl, rgbBits, yuvBits, false>(converter, img, primary);
  }
  return img;
}

template <typename ConverterImplRGB, typename ConverterImplA, size_t rgbBits, size_t yuvBits, size_t monoBits>
avif::img::Image<rgbBits> convertToRGBA(ConverterImplRGB const& converterRgb, Dav1dPicture& primary, ConverterImplA const& converterAlpha, Dav1dPicture& alpha) {
  avif::img::PixelOrder const pixelOrder = primary.p.layout == DAV1D_PIXEL_LAYOUT_I400 ? avif::img::PixelOrder::MonoA : avif::img::PixelOrder::RGBA;
  avif::img::Image<rgbBits> img = avif::img::Image<rgbBits>::createEmptyImage(pixelOrder, primary.p.w, primary.p.h);

  if(primary.seq_hdr->color_range == 0) {
    writeImage<ConverterImplRGB, rgbBits, yuvBits, false>(converterRgb, img, primary);
  } else {
    writeImage<ConverterImplRGB, rgbBits, yuvBits, true>(converterRgb, img, primary);
  }

  if(alpha.seq_hdr->color_range == 0) {
    writeAlpha<ConverterImplA, rgbBits, monoBits, false>(converterAlpha, img, alpha);
  } else {
    writeAlpha<ConverterImplA, rgbBits, monoBits, true>(converterAlpha, img, alpha);
  }
  return img;
}

template <typename ConverterImplRGB, typename ConverterImplA, size_t rgbBits, size_t yuvBits>
avif::img::Image<rgbBits> convertToRGBA(ConverterImplRGB const& converterRgb, Dav1dPicture& primary, ConverterImplA const& converterAlpha, Dav1dPicture& alpha) {
  switch(alpha.p.bpc) {
    case 8:
      return convertToRGBA<ConverterImplRGB, ConverterImplA, rgbBits, yuvBits, 8>(converterRgb, primary, converterAlpha, alpha);
    case 10:
      return convertToRGBA<ConverterImplRGB, ConverterImplA, rgbBits, yuvBits, 10>(converterRgb, primary, converterAlpha, alpha);
    case 12:
      return convertToRGBA<ConverterImplRGB, ConverterImplA, rgbBits, yuvBits, 12>(converterRgb, primary, converterAlpha, alpha);
    default:
      throw std::runtime_error(fmt::format("Unknwon alpha bpc={}", alpha.p.bpc));
  }
}

template <typename ConverterImplRGB>
std::variant<avif::img::Image<8>, avif::img::Image<16>> createImageRGB(ConverterImplRGB const& converterRgb, Dav1dPicture& primary) {
  switch(primary.p.bpc) {
    case 8:
      return convertToRGB<ConverterImplRGB, 8, 8>(converterRgb, primary);
    case 10:
      return convertToRGB<ConverterImplRGB, 16, 10>(converterRgb, primary);
    case 12:
      return convertToRGB<ConverterImplRGB, 16, 12>(converterRgb, primary);
    default:
      throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
  }
}

template <typename ConverterImplRGB, typename ConverterImplA>
    std::variant<avif::img::Image<8>, avif::img::Image<16>> createImageRGBA(ConverterImplRGB const& converterRgb, Dav1dPicture& primary, ConverterImplA const& converterAlpha, Dav1dPicture& alpha) {
  if(primary.p.bpc == 8 && alpha.p.bpc == 8) {
    switch(primary.p.bpc) {
      case 8:
        return convertToRGBA<ConverterImplRGB, ConverterImplA, 8, 8>(converterRgb, primary, converterAlpha, alpha);
      case 10:
        return convertToRGBA<ConverterImplRGB, ConverterImplA, 8, 10>(converterRgb, primary, converterAlpha, alpha);
      case 12:
        return convertToRGBA<ConverterImplRGB, ConverterImplA, 8, 12>(converterRgb, primary, converterAlpha, alpha);
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
    }
  } else {
    switch(primary.p.bpc) {
      case 8:
        return convertToRGBA<ConverterImplRGB, ConverterImplA, 16, 8>(converterRgb, primary, converterAlpha, alpha);
      case 10:
        return convertToRGBA<ConverterImplRGB, ConverterImplA, 16, 10>(converterRgb, primary, converterAlpha, alpha);
      case 12:
        return convertToRGBA<ConverterImplRGB, ConverterImplA, 16, 12>(converterRgb, primary, converterAlpha, alpha);
      default:
        throw std::runtime_error(fmt::format("Unknwon bpc={}", primary.p.bpc));
    }
  }
}

template <typename ConverterImplRGB>
std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(
    ConverterImplRGB const& converterRgb,
    Dav1dPicture& primary,
    std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha)
{
  namespace converters = avif::img::converters;
  if(!alpha.has_value()) {
    return createImageRGB<ConverterImplRGB>(converterRgb, primary);
  }

  Dav1dPicture& alphaPic = std::get<0>(alpha.value());
  avif::img::ColorProfile const& alphaProf = std::get<1>(alpha.value());
  if(std::holds_alternative<avif::img::ICCProfile>(alphaProf)) {
    auto const& profile = std::get<avif::img::ICCProfile>(alphaProf);
    auto icc = profile.calcColorCoefficients();
    return createImageRGBA<ConverterImplRGB, decltype(icc)>(converterRgb, primary, icc, alphaPic);
  }
  avif::ColourInformationBox::NCLX nclx = {
      .colourPrimaries = static_cast<uint16_t>(avif::img::ColorPrimaries::CP_BT_709),
      .transferCharacteristics = static_cast<uint16_t>(avif::img::TransferCharacteristics::TC_BT_709),
      .matrixCoefficients = static_cast<uint16_t>(avif::img::MatrixCoefficients::MC_BT_709),
      .fullRangeFlag = false,
  };
  if(std::holds_alternative<avif::ColourInformationBox::NCLX>(alphaProf)){
    nclx = std::get<avif::ColourInformationBox::NCLX>(alphaProf);
  }

  switch (static_cast<avif::img::MatrixCoefficients>(nclx.matrixCoefficients)) {
    case avif::img::MatrixCoefficients::MC_IDENTITY: {
      constexpr auto converter = converters::Identity;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_UNSPECIFIED:
    case avif::img::MatrixCoefficients::MC_BT_709: {
      constexpr auto converter = converters::BT_709;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_FCC: {
      constexpr auto converter = converters::FCC;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_BT_470_B_G: {
      constexpr auto converter = converters::BT_470_B_G;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_BT_601: {
      constexpr auto converter = converters::BT_601;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_SMPTE_240: {
      constexpr auto converter = converters::SMPTE_240;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_SMPTE_YCGCO: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_SMPTE_YCGCO);
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_BT_2020_NCL: {
      constexpr auto converter = converters::MC_BT_2020_NCL;
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_BT_2020_CL: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_SMPTE_YCGCO);
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_SMPTE_2085: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_SMPTE_2085);
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_CHROMAT_NCL: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_CHROMAT_NCL);
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_CHROMAT_CL: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_CHROMAT_CL);
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    case avif::img::MatrixCoefficients::MC_ICTCP: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_ICTCP);
      return createImageRGBA<ConverterImplRGB, decltype(converter)>(converterRgb, primary, converter, alphaPic);
    }
    default:
      assert(false && "Unknown matrix coefficients");
  }
}

std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(
    Dav1dPicture& primary, avif::img::ColorProfile const& primaryProfile,
    std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha)
{
  if(std::holds_alternative<avif::img::ICCProfile>(primaryProfile)) {
    auto const& profile = std::get<avif::img::ICCProfile>(primaryProfile);
    auto icc = profile.calcColorCoefficients();
    return createImage<decltype(icc)>(icc, primary, alpha);
  }

  avif::ColourInformationBox::NCLX nclx = {
      .colourPrimaries = static_cast<uint16_t>(avif::img::ColorPrimaries::CP_BT_709),
      .transferCharacteristics = static_cast<uint16_t>(avif::img::TransferCharacteristics::TC_BT_709),
      .matrixCoefficients = static_cast<uint16_t>(avif::img::MatrixCoefficients::MC_BT_709),
      .fullRangeFlag = false,
  };
  if(std::holds_alternative<avif::ColourInformationBox::NCLX>(primaryProfile)){
    nclx = std::get<avif::ColourInformationBox::NCLX>(primaryProfile);
  }
  namespace converters = avif::img::converters;
  switch (static_cast<avif::img::MatrixCoefficients>(nclx.matrixCoefficients)) {
    case avif::img::MatrixCoefficients::MC_IDENTITY: {
      constexpr auto converter = converters::Identity;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_UNSPECIFIED:
    case avif::img::MatrixCoefficients::MC_BT_709:{
      constexpr auto converter = converters::BT_709;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_FCC:{
      constexpr auto converter = converters::FCC;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_BT_470_B_G:{
      constexpr auto converter = converters::BT_470_B_G;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_BT_601:{
      constexpr auto converter = converters::BT_601;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_SMPTE_240:{
      constexpr auto converter = converters::SMPTE_240;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_SMPTE_YCGCO: {
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_SMPTE_YCGCO);
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_BT_2020_NCL: {
      constexpr auto converter = converters::MC_BT_2020_NCL;
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_BT_2020_CL:{
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_SMPTE_YCGCO);
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_SMPTE_2085:{
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_SMPTE_2085);
      return createImage<decltype(converter)>(converter, primary, alpha);
    }
    case avif::img::MatrixCoefficients::MC_CHROMAT_NCL:{
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_CHROMAT_NCL);
      return createImage<decltype(converter)>(converter, primary, alpha);
    }

    case avif::img::MatrixCoefficients::MC_CHROMAT_CL:{
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_CHROMAT_CL);
      return createImage<decltype(converter)>(converter, primary, alpha);
    }

    case avif::img::MatrixCoefficients::MC_ICTCP:{
      constexpr auto converter = converters::Unimplementd(avif::img::MatrixCoefficients::MC_ICTCP);
      return createImage<decltype(converter)>(converter, primary, alpha);
    }

    default:
      assert(false && "Unknown matrix coefficients");
  }
}
