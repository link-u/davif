//
// Created by psi on 2019/11/25.
//

#pragma once
#include <variant>
#include <optional>
#include <vector>
#include <string>

namespace util {
std::variant<std::vector<uint8_t>, std::string> readFile(std::string const& fname);
std::optional<std::string> writeFile(std::string const& fname, std::vector<uint8_t> const& data);
}