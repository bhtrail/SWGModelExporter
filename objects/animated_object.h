#pragma once

#include "objects/base_object.h"
#include "geometry_common.h"
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
  virtual void store(const std::string& path) override { };
  virtual std::set<std::string> get_referenced_objects() const override;
  virtual void set_object_name(const std::string&) override { };

private:
  std::vector<std::string> m_mesh_names;
  std::vector<std::pair<std::string, std::string>> m_skeleton_info;
  std::unordered_map<std::string, std::string> m_animations_map;
};

class Animated_lod_list : public Base_object
{
public:
  Animated_lod_list() { }

  void add_lod_name(const std::string& name) { m_lod_names.emplace_back(name); }
  // overrides
  virtual bool is_object_correct() const override { return true; }
  virtual void store(const std::string& path) override { };
  virtual std::set<std::string> get_referenced_objects() const override;
  virtual void set_object_name(const std::string&) override { };
private:
  std::vector<std::string> m_lod_names;
};


class Animated_mesh : public Base_object
{
public:
#pragma pack(push, 1)
  struct Info
  {
    uint32_t num_skeletons;
    uint32_t num_joints;
    uint32_t position_counts;
    uint32_t joint_weight_data_count;
    uint32_t normals_count;
    uint32_t num_shaders;
    uint32_t blend_targets;
    uint16_t occlusion_zones;
    uint16_t occlusion_zones_combinations;
    uint16_t occluded_zones;
    int16_t occlusion_layer;
  };
#pragma pack(pop)

  class Vertex
  {
  public:
    Vertex(const Geometry::Point& position) : m_position(position) { }
    void set_position(const Geometry::Point& point) { m_position = point; }
    const Geometry::Point& get_position() const { return m_position; }
    std::vector<std::pair<uint32_t, float>>& get_weights() { return m_weights; }
  private:
    Geometry::Point m_position;
    std::vector<std::pair<uint32_t, float>> m_weights;
  };

  class Shader
  {
  public:
    Shader(const std::string& name) : m_name(name) { }
    const std::string& get_name() const { return m_name; }
    std::vector<uint32_t>& get_pos_indexes() { return m_position_indexes; }
    std::vector<uint32_t>& get_normal_indexes() { return m_normal_indexes; }
    std::vector<uint32_t>& get_light_indexes() { return m_light_indexes; }
    std::vector<Graphics::Tex_coord>& get_texels() { return m_texels; }
    std::vector<Graphics::Triangle_indexed>& get_triangles() { return m_triangles; }
    std::vector<std::pair<uint32_t, uint32_t>>& get_primivites() { return m_primitives; }

    void add_primitive();
    void close_primitive() { m_primitives.back().second = static_cast<uint32_t>(m_primitives.size()); }
  private:
    std::string m_name;
    std::vector<uint32_t> m_position_indexes;
    std::vector<uint32_t> m_normal_indexes;
    std::vector<uint32_t> m_light_indexes;
    std::vector<Graphics::Tex_coord> m_texels;
    std::vector<Graphics::Triangle_indexed> m_triangles;
    std::vector<std::pair<uint32_t, uint32_t>> m_primitives;
  };

public:
  Animated_mesh() { }

  const Info& get_info() const { return m_mesh_info; }
  void set_info(const Info& info);

  void add_vertex_position(const Geometry::Point& position) { m_vertices.emplace_back(position); }
  Vertex& get_vertex(const uint32_t pos_num) { return m_vertices[pos_num]; }

  void add_normal(const Geometry::Vector3& norm) { m_normals.emplace_back(norm); }
  void add_lighting_normal(const Geometry::Vector4& light_norm) { m_lighting_normals.emplace_back(light_norm); }
  void add_skeleton_name(const std::string& name) { m_skeletons_names.emplace_back(name); }
  void add_joint_name(const std::string& name) { m_joint_names.emplace_back(name); }
  void add_new_shader(const std::string& name) { m_shaders.emplace_back(name); }
  Shader& get_current_shader() { return m_shaders.back(); }

  // Inherited via Base_object
  virtual bool is_object_correct() const override;
  virtual void store(const std::string & path) override;
  virtual std::set<std::string> get_referenced_objects() const override;
  virtual void set_object_name(const std::string& obj_name) override { m_object_name = obj_name; }

private:
  std::string m_object_name;
  Info m_mesh_info;
  std::vector<std::string> m_skeletons_names;
  std::vector<std::string> m_joint_names;
  std::vector<Vertex> m_vertices;
  std::vector<Geometry::Vector3> m_normals;
  std::vector<Geometry::Vector4> m_lighting_normals;
  std::vector<Shader> m_shaders;
};
