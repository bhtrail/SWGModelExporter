#include "stdafx.h"
#include "mgn_parser.h"

using namespace std;

void mgn_parser::section_begin(const string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
  if (name == "SKMGFORM")
  {
    m_section_received[skmg] = true;
  }
}

void mgn_parser::parse_data(const string& name, uint8_t * data_ptr, size_t data_size)
{
  base_buffer buffer(data_ptr, data_size);

  if (name == "0004INFO" && m_section_received[skmg])
  {
    // read info
    auto max_tranforms_per_vector = buffer.read_uint32();
    auto max_tranforms_per_shader = buffer.read_uint32();
    m_num_skeletons = buffer.read_uint32();
    auto joints = buffer.read_uint32();
    auto pos_count = buffer.read_uint32();
    auto joints_weights = buffer.read_uint32();
    auto normals = buffer.read_uint32();
    m_num_shaders = buffer.read_uint32();
    m_blend_targets = buffer.read_uint32();
    m_occlusion_zones = buffer.read_uint16();
    m_occlusion_zones_combinations = buffer.read_uint16();
    m_occluded_zones = buffer.read_uint16();
    m_occlusion_layer = static_cast<int16_t>(buffer.read_uint16());

    m_section_received[info] = buffer.end_of_buffer();
  }
}
