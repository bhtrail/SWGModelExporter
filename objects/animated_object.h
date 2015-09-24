#pragma once

#include "objects\base_object.h"
//
// Animated object descriptor, builded from SAT files
//
class Animated_object_descriptor : public Base_object
{
public:
  Animated_object_descriptor()
  { }

  // getters
  // meshes
  size_t get_mesh_count() const { return m_mesh_names.size(); }
  std::string get_mesh_name(size_t idx) { return m_mesh_names[idx]; }

  // skeletons
  size_t get_skeletons_count() const { return m_skeleton_info.size(); }
  std::string get_skeleton_name(size_t idx) { return m_skeleton_info[idx].first; }
  std::string get_skeleton_attach_point(size_t idx) { return m_skeleton_info[idx].second; }

  // anim maps
  size_t get_animation_maps() const { return m_animations_map.size(); }
  std::string get_animation_for_skeleton(const std::string& skt_name) { return m_animations_map[skt_name]; }

  // fillers
  void add_mesh_name(const std::string& mesh_name) { m_mesh_names.push_back(mesh_name); }
  void add_skeleton_info(const std::string& skt_name, const std::string& slot)
  {
    m_skeleton_info.push_back(make_pair(skt_name, slot));
  }
  void add_anim_map(const std::string& skt_name, const std::string& anim_map)
  {
    m_animations_map[skt_name] = anim_map;
  }

  virtual bool is_object_correct() const override { return true; }
  virtual void store(const std::string& path) override { }

private:
  std::vector<std::string> m_mesh_names;
  std::vector<std::pair<std::string, std::string>> m_skeleton_info;
  std::unordered_map<std::string, std::string> m_animations_map;
};
