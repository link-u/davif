//
// Created by psi on 2020/02/16.
//

#pragma once

#include <optional>
#include <variant>
#include <dav1d/picture.h>
#include <avif/img/Image.hpp>

std::variant<avif::img::Image<8>, avif::img::Image<16>> createImage(
    Dav1dPicture& primary, avif::av1::SequenceHeader::ColorConfig::MatrixCoefficients primaryMatrix,
    std::optional<Dav1dPicture>& alpha, std::optional<avif::av1::SequenceHeader::ColorConfig::MatrixCoefficients>& alphaMatrix);
