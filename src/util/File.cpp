//
// Created by psi on 2019/11/25.
//
#include <filesystem>
#include "../../external/tinyformat/tinyformat.h"

#include "File.h"

namespace util {

std::variant<std::vector<uint8_t>, std::string> readFile(std::string const& fname) {
  if(!std::filesystem::exists(fname)) {
    return std::variant<std::vector<uint8_t>, std::string>(tfm::format("File not found: %s", fname));
  }
  size_t const fsize = std::filesystem::file_size(fname);
  FILE* file = fopen(fname.c_str(), "rb");
  if(!file) {
    return std::variant<std::vector<uint8_t>, std::string>(tfm::format("Could not open file: %s", fname));
  }
  std::vector<uint8_t> data;
  data.resize(fsize);
  size_t const got = fread(data.data(), 1, fsize, file);
  if(fsize != got){
    return std::variant<std::vector<uint8_t>, std::string>(tfm::format("Could not read entire file: %s. Expected=%d, got=%d", fname, fsize, got));
  }
  fclose(file);
  return std::variant<std::vector<uint8_t>, std::string>(std::move(data));
}

std::optional<std::string> writeFile(std::string const& fname, std::vector<uint8_t> const& data){
  FILE* file = fopen(fname.c_str(), "wb");
  if(!file) {
    return std::make_optional(tfm::format("Could not open file: %s", fname));
  }
  size_t const got = fwrite(data.data(), 1, data.size(), file);
  size_t const fsize = data.size();
  if(fsize != got){
    return std::make_optional(tfm::format("Could not read entire file: %s. Expected=%d, got=%d", fname, fsize, got));
  }
  fflush(file);
  fclose(file);
  return std::optional<std::string>();
}

}
