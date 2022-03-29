//
// Created by psi on 2020/02/16.
//

#pragma once

#include <optional>
#include <variant>
#include <tuple>
#include <dav1d/picture.h>
#include <avif/img/Image.hpp>

namespace img {

std::variant<
  avif::img::Image<8>,
  avif::img::Image<16>
> createImage(
  Dav1dPicture& primary, avif::img::ColorProfile const& primaryProfile,
  std::optional<std::tuple<Dav1dPicture&, avif::img::ColorProfile const&>> alpha
);

}