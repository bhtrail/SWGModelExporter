#include "stdafx.h"

#include "animated_object.h"
#include "tre_library.h"

using namespace std;
namespace fs = boost::filesystem;

set<string> Animated_object_descriptor::get_referenced_objects() const
{
  set<string> names;

  // get mesh names
  names.insert(m_mesh_names.begin(), m_mesh_names.end());

  // get skeleton names
  for_each(m_skeleton_info.begin(), m_skeleton_info.end(),
           [&names](const pair<string, string>& item)
  {
    names.insert(item.first);
  });
  // get anim maps names;
  for_each(m_animations_map.begin(), m_animations_map.end(),
           [&names](const pair<string, string>& item)
  {
    names.insert(item.second);
  });
  return move(names);
}

set<string> Animated_lod_list::get_referenced_objects() const
{
  set<string> names;

  names.insert(m_lod_names.begin(), m_lod_names.end());

  return move(names);
}

void Animated_mesh::set_info(const Info & info)
{
  m_mesh_info = info;

  m_vertices.reserve(m_mesh_info.position_counts);
  m_normals.reserve(m_mesh_info.normals_count);
  m_joint_names.reserve(m_mesh_info.num_joints);
  m_shaders.reserve(m_mesh_info.num_shaders);
}

bool Animated_mesh::is_object_correct() const
{
  return true;
}

void Animated_mesh::store(const std::string & path)
{
}

std::set<std::string> Animated_mesh::get_referenced_objects() const
{
  set<string> names;
  names.insert(m_skeletons_names.begin(), m_skeletons_names.end());
  for_each(m_shaders.begin(), m_shaders.end(),
    [&names](const Shader& shader) { names.insert(shader.get_name()); });
  return names;
}

void Animated_mesh::Shader::add_primitive()
{
  uint32_t new_primitive_position = static_cast<uint32_t>(m_triangles.size());
  if (new_primitive_position > 0)
    m_primitives.back().second = new_primitive_position;

  m_primitives.emplace_back(new_primitive_position, 0);
}
