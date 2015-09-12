#pragma once

#include "base_buffer.h"

class IFF_visitor
{
public:
  virtual void section_begin(const std::string& name, uint8_t * data_ptr, size_t data_size) = 0;
  virtual void parse_data(const std::string& name, uint8_t * data_ptr, size_t data_size) = 0;
};

class IFF_file
{
public:
  IFF_file() = delete;
  explicit IFF_file(uint8_t *data_ptr, size_t data_size);
  explicit IFF_file(const std::vector<uint8_t>& buffer);

  ~IFF_file();

  void full_process(std::shared_ptr<IFF_visitor> visitor);
private:
  std::string _get_iff_name();
  bool _is_form(const std::string& name);

  base_buffer m_buffer;
};

namespace IFF_utility
{
  uint16_t swap_bytes(uint16_t value);
  uint32_t swap_bytes(uint32_t value);
  uint64_t swap_bytes(uint64_t value);
}

