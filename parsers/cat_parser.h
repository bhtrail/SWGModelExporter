#pragma once

#include "..\IFF_file.h"

class Animated_object_descriptor;

class cat_parser : public IFF_visitor
{
public:
  cat_parser()
    : m_meshes_count(0), m_skeletons_count(0), m_latx_present(false)
  { }
  virtual void section_begin(const std::string& name, uint8_t * data_ptr, size_t data_size) override;
  virtual void parse_data(const std::string& name, uint8_t * data_ptr, size_t data_size) override;

  bool is_result_correct();
  std::shared_ptr<Animated_object_descriptor> get_result_descriptor() { return m_object; }

  void reset();
private:
  enum sections_received
  {
    smat = 0,
    info = 1,
    msgn = 2,
    skti = 3,
    latx = 4
  };

  uint32_t m_meshes_count;
  uint32_t m_skeletons_count;
  bool m_latx_present;
  std::shared_ptr<Animated_object_descriptor> m_object;
  std::bitset<5> m_section_received;
};