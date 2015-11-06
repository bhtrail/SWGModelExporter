#include "stdafx.h"
#include "mgn_parser.h"

using namespace std;

void mgn_parser::section_begin(const string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
  if (name == "SKMGFORM")
  {
    m_section_received[skmg] = true;
    m_object = make_shared<Animated_mesh>();
  }
}

void mgn_parser::parse_data(const string& name, uint8_t * data_ptr, size_t data_size)
{
  base_buffer buffer(data_ptr, data_size);

  if (name == "0004INFO" && m_section_received[skmg])
  {
    read_mesh_info_(buffer);
  }
  else if (name == "SKTM" && m_section_received[info])
  {
    read_skeletons_list_(buffer);
  }
  else if (name == "XFNM" && m_section_received[sktm])
  {
    read_joints_names_(buffer);
  }
  else if (name == "POSN" && m_section_received[xfnm])
  {
    read_vertices_list_(buffer);
  }
  else if (name == "TWHD" && m_section_received[posn])
  {
    read_weight_counters_(buffer);
  }
  else if (name == "TWDT" && m_section_received[twhd])
  {
    read_vertex_weights_(buffer);
  }
  else if (name == "NORM" && m_section_received[twdt])
  {
    read_normals_(buffer);
  }
  else if (name == "DOT3" && m_section_received[norm])
  {
    const auto& mesh_info = m_object->get_info();
    uint32_t light_readed = 0;
    uint32_t lights_to_read = buffer.read_uint32();
    while (!buffer.end_of_buffer() && light_readed < lights_to_read)
    {
      Geometry::Vector3 vec;
      vec.x = buffer.read_float();
      vec.y = buffer.read_float();
      vec.z = buffer.read_float();

      m_object->add_lightning_normal(vec);
      light_readed++;
    }
    m_section_received[dot3] = buffer.end_of_buffer() && light_readed == lights_to_read;
  }
}

void mgn_parser::read_normals_(base_buffer &buffer)
{
  const auto& mesh_info = m_object->get_info();
  uint32_t norm_readed = 0;
  while (!buffer.end_of_buffer() && norm_readed < mesh_info.normals_count)
  {
    Geometry::Vector3 vec;
    vec.x = buffer.read_float();
    vec.y = buffer.read_float();
    vec.z = buffer.read_float();

    m_object->add_normal(vec);
    norm_readed++;
  }
  m_section_received[norm] = buffer.end_of_buffer() && norm_readed == mesh_info.normals_count;
}

void mgn_parser::read_vertex_weights_(base_buffer &buffer)
{
  const auto& mesh_info = m_object->get_info();
  uint32_t vertex_pos = 0;
  uint32_t weights_readed = 0;
  while (!buffer.end_of_buffer() && vertex_pos < mesh_info.position_counts)
  {
    auto& vertex = m_object->get_vertex(vertex_pos);

    auto num_weights = vertex.get_weights().size();
    size_t weight_pos = 0;

    while (num_weights > 0)
    {
      uint32_t joint_idx = buffer.read_uint32();
      float joint_weight = buffer.read_float();

      vertex.get_weights()[weight_pos++] = make_pair(joint_idx, joint_weight);

      num_weights--;
      weights_readed++;
    }

    vertex_pos++;
  }
  m_section_received[twdt] = buffer.end_of_buffer()
    && weights_readed == mesh_info.joint_weight_data_count
    && vertex_pos == mesh_info.position_counts;
}

void mgn_parser::read_weight_counters_(base_buffer &buffer)
{
  const auto& mesh_info = m_object->get_info();
  uint32_t counters_readed = 0;
  while (!buffer.end_of_buffer() && counters_readed < mesh_info.position_counts)
  {
    auto& vertex = m_object->get_vertex(counters_readed);
    uint32_t num_weights = buffer.read_uint32();
    vertex.get_weights().resize(num_weights);
    counters_readed++;
  }
  m_section_received[twhd] = buffer.end_of_buffer() && counters_readed == mesh_info.position_counts;
}

void mgn_parser::read_vertices_list_(base_buffer &buffer)
{
  const auto& mesh_info = m_object->get_info();
  uint32_t points_readed = 0;
  while (!buffer.end_of_buffer() && points_readed < mesh_info.position_counts)
  {
    Geometry::Point pt;
    pt.x = buffer.read_float();
    pt.y = buffer.read_float();
    pt.z = buffer.read_float();

    m_object->add_vertex_position(pt);
    points_readed++;
  }
  m_section_received[posn] = buffer.end_of_buffer() && points_readed == mesh_info.position_counts;
}

void mgn_parser::read_joints_names_(base_buffer &buffer)
{
  const auto& mesh_info = m_object->get_info();
  uint32_t joints_readed = 0;
  while (!buffer.end_of_buffer() && joints_readed < mesh_info.num_joints)
  {
    string joint_name = buffer.read_stringz();
    m_object->add_joint_name(joint_name);
    joints_readed++;
  }
  m_section_received[xfnm] = buffer.end_of_buffer() && (joints_readed == mesh_info.num_joints);
}

void mgn_parser::read_skeletons_list_(base_buffer &buffer)
{
  // read list of skeletals
  const auto& mesh_info = m_object->get_info();

  uint32_t skels_readed = 0;
  while (!buffer.end_of_buffer() && skels_readed < mesh_info.num_skeletons)
  {
    string skel_name = buffer.read_stringz();
    m_object->add_skeleton_name(skel_name);
    skels_readed++;
  }
  m_section_received[sktm] = buffer.end_of_buffer() && (skels_readed == mesh_info.num_skeletons);
}

void mgn_parser::read_mesh_info_(base_buffer &buffer)
{
  // read info
  Animated_mesh::Info mesh_info;

  auto max_tranforms_per_vector = buffer.read_uint32();
  auto max_tranforms_per_shader = buffer.read_uint32();

  buffer.read_buffer(reinterpret_cast<uint8_t *>(&mesh_info), sizeof(mesh_info));

  m_object->set_info(mesh_info);
  m_section_received[info] = buffer.end_of_buffer();
}
