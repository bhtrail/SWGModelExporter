#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class mgn_parser : public IFF_visitor
{
public:
  mgn_parser()
    : m_num_skeletons(0), m_num_shaders(0), m_blend_targets(0), m_occluded_zones(0), m_occlusion_zones_combinations(0)
    , m_occlusion_zones(0)
  { }
  // Inherited via IFF_visitor
  virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
  virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;
private:
  enum section_received
  {
    skmg = 30,
    info = 0,
    sktm = 1,
    xfnm = 2,
    posn = 3,
    twhd = 4,
    twdt = 5,
    norm = 6,
    dot3 = 7,
    blts = 8, blts_blt = 9, blts_blt_info = 10, blts_blt_posn = 11, blts_blt_norm = 12, blts_blt_dot3 = 13,
    ozn = 14,
    fozc = 15,
    ozc = 16,
    zto = 17,
    psdt = 18, psdt_name = 19, psdt_pidx = 20, psdt_nidx = 21, psdt_dot3 = 22, psdt_txci = 23,
    psdt_tcsf = 24, psdt_tcsf_tcsd = 25,
    psdt_prim = 26, psdt_prim_info = 27, psdt_prim_itl = 28, psdt_prim_oitl = 29
  };

  std::bitset<31> m_section_received;

  uint32_t m_num_skeletons;
  uint32_t m_num_shaders;
  uint32_t m_blend_targets;
  uint16_t m_occlusion_zones;
  uint16_t m_occlusion_zones_combinations;
  uint16_t m_occluded_zones;
  int16_t m_occlusion_layer;
};

