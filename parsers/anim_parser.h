#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class anim_parser : public IFF_visitor
{
public:
  // Inherited via IFF_visitor
  virtual void section_begin(const std::string& name, uint8_t * data_ptr, size_t data_size, uint32_t depth) override;
  virtual void parse_data(const std::string& name, uint8_t * data_ptr, size_t data_size) override;

private:
  std::shared_ptr<Animation> m_animation;
};
