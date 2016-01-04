#include "stdafx.h"

#include "animated_object.h"
#include "tre_library.h"

using namespace std;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

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
  return !m_skeletons_names.empty()
    && !m_joint_names.empty()
    && !m_vertices.empty()
    && !m_normals.empty()
    && !m_shaders.empty();
}

void Animated_mesh::store(const std::string& path)
{
  // extract object name and make full path name
  fs::path obj_name(m_object_name);
  fs::path target_path(path);
  target_path /= obj_name.filename();
  target_path.replace_extension("fbx");

  if (fs::exists(target_path))
    fs::remove(target_path);

  // get lod level (by _lX end of file name). If there is no such pattern - lod level will be zero.
  m_lod_level = 0;
  obj_name = obj_name.filename();
  obj_name.replace_extension();
  string name = obj_name.string();
  char last_char = name.back();
  if (isdigit(last_char))
    m_lod_level = (last_char - '0');

  // init FBX manager
  FbxManager* fbx_manager_ptr = FbxManager::Create();
  if (fbx_manager_ptr == nullptr)
    return;

  FbxIOSettings* ios_ptr = FbxIOSettings::Create(fbx_manager_ptr, IOSROOT);
  ios_ptr->SetIntProp(EXP_FBX_EXPORT_FILE_VERSION, FBX_FILE_VERSION_7400);
  fbx_manager_ptr->SetIOSettings(ios_ptr);

  FbxExporter* exporter_ptr = FbxExporter::Create(fbx_manager_ptr, "");
  if (!exporter_ptr)
    return;

  exporter_ptr->SetFileExportVersion(FBX_2014_00_COMPATIBLE);
  bool result = exporter_ptr->Initialize(target_path.string().c_str(), -1, fbx_manager_ptr->GetIOSettings());
  if (!result)
  {
    auto status = exporter_ptr->GetStatus();
    cout << "FBX error: " << status.GetErrorString() << endl;
    return;
  }
  FbxScene* scene_ptr = FbxScene::Create(fbx_manager_ptr, m_object_name.c_str());
  if (!scene_ptr)
    return;

  scene_ptr->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
  auto scale_factor = scene_ptr->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
  FbxNode* mesh_node_ptr = FbxNode::Create(scene_ptr, "mesh_node");
  FbxMesh* mesh_ptr = FbxMesh::Create(scene_ptr, "mesh");

  mesh_node_ptr->SetNodeAttribute(mesh_ptr);
  scene_ptr->GetRootNode()->AddChild(mesh_node_ptr);

  // prepare vertices
  uint32_t vertices_num = static_cast<uint32_t>(m_vertices.size());
  mesh_ptr->SetControlPointCount(vertices_num);
  auto mesh_vertices = mesh_ptr->GetControlPoints();

  for (uint32_t vertex_idx = 0; vertex_idx < vertices_num; ++vertex_idx)
  {
    const auto& pt = m_vertices[vertex_idx].get_position();
    mesh_vertices[vertex_idx] = FbxVector4(pt.x, pt.y, pt.z);
  }

  // add material layer
  auto material_layer = mesh_ptr->CreateElementMaterial();
  material_layer->SetMappingMode(FbxLayerElement::eByPolygon);
  material_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);

  // process polygons
  vector<uint32_t> normal_indexes;
  vector<uint32_t> tangents_idxs;
  vector<Graphics::Tex_coord> uvs;
  vector<uint32_t> uv_indexes;

  for (uint32_t shader_idx = 0; shader_idx < m_shaders.size(); ++shader_idx)
  {
    auto& shader = m_shaders[shader_idx];
    auto& material = shader.get_definition()->material();
    auto& textures = shader.get_definition()->textures();

    // create material for this shader
    auto material_ptr = FbxSurfacePhong::Create(scene_ptr, shader.get_name().c_str());
    material_ptr->ShadingModel.Set("Phong");
    material_ptr->Ambient.Set(FbxDouble3(material.ambient.r, material.ambient.g, material.ambient.g));
    material_ptr->Diffuse.Set(FbxDouble3(material.diffuse.r, material.diffuse.g, material.diffuse.g));
    material_ptr->Emissive.Set(FbxDouble3(material.emissive.r, material.emissive.g, material.emissive.g));
    material_ptr->Specular.Set(FbxDouble3(material.specular.r, material.specular.g, material.specular.g));

    // add texture definitions
    for (auto& texture_def : textures)
    {
      FbxFileTexture* texture = FbxFileTexture::Create(scene_ptr, texture_def.tex_file_name.c_str());
      boost::filesystem::path tex_path(path);
      tex_path /= texture_def.tex_file_name;
      tex_path.replace_extension("tga");

      texture->SetFileName(tex_path.string().c_str());
      texture->SetTextureUse(FbxTexture::eStandard);
      texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
      texture->SetMappingType(FbxTexture::eUV);
      texture->SetWrapMode(FbxTexture::eRepeat, FbxTexture::eRepeat);
      texture->SetTranslation(0.0, 0.0);
      texture->SetScale(1.0, 1.0);
      texture->SetTranslation(0.0, 0.0);
      switch (texture_def.texture_type)
      {
      case Shader::texture_type::main:
        material_ptr->Diffuse.ConnectSrcObject(texture);
        break;
      case Shader::texture_type::normal:
        material_ptr->Bump.ConnectSrcObject(texture);
        break;
      case Shader::texture_type::specular:
        material_ptr->Specular.ConnectSrcObject(texture);
        break;
      }
    }

    mesh_ptr->GetNode()->AddMaterial(material_ptr);

    // get geometry element
    auto& triangles = shader.get_triangles();
    auto& positions = shader.get_pos_indexes();
    auto& normals = shader.get_normal_indexes();
    auto& tangents = shader.get_light_indexes();

    auto idx_offset = static_cast<uint32_t>(uvs.size());
    copy(shader.get_texels().begin(), shader.get_texels().end(), back_inserter(uvs));

    normal_indexes.reserve(normal_indexes.size() + normals.size());
    tangents_idxs.reserve(tangents_idxs.size() + tangents.size());

    for (uint32_t tri_idx = 0; tri_idx < triangles.size(); ++tri_idx)
    {
      auto& tri = triangles[tri_idx];
      mesh_ptr->BeginPolygon(shader_idx, -1, shader_idx, false);
      for (size_t i = 0; i < 3; ++i)
      {
        auto remapped_pos_idx = positions[tri.points[i]];
        mesh_ptr->AddPolygon(remapped_pos_idx);

        auto remapped_normal_idx = normals[tri.points[i]];
        normal_indexes.emplace_back(remapped_normal_idx);

        if (!tangents.empty())
        {
          auto remapped_tangent = tangents[tri.points[i]];
          tangents.emplace_back(remapped_tangent);
        }
        uv_indexes.emplace_back(idx_offset + tri.points[i]);
      }
      mesh_ptr->EndPolygon();
    }
  }
  // add UVs
  FbxGeometryElementUV* uv_ptr = mesh_ptr->CreateElementUV("UVSet1");
  uv_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
  uv_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

  for_each(uvs.begin(), uvs.end(), [&uv_ptr](const Graphics::Tex_coord& coord)
  {
    uv_ptr->GetDirectArray().Add(FbxVector2(coord.u, coord.v));
  });
  for_each(uv_indexes.begin(), uv_indexes.end(), [&uv_ptr](const uint32_t idx)
  {
    uv_ptr->GetIndexArray().Add(idx);
  });

  // add normals
  if (!normal_indexes.empty())
  {
    FbxGeometryElementNormal* normals_ptr = mesh_ptr->CreateElementNormal();
    normals_ptr->SetMappingMode(FbxGeometryElementNormal::eByPolygonVertex);
    normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
    auto& direct_array = normals_ptr->GetDirectArray();
    for_each(m_normals.begin(), m_normals.end(),
      [&direct_array](const Geometry::Vector3& elem)
    {
      direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
    });

    auto& index_array = normals_ptr->GetIndexArray();
    for_each(normal_indexes.begin(), normal_indexes.end(),
      [&index_array](const uint32_t& idx) { index_array.Add(idx); });
  }

  // add tangents
  if (!tangents_idxs.empty())
  {
    FbxGeometryElementTangent* normals_ptr = mesh_ptr->CreateElementTangent();
    normals_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
    normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
    auto& direct_array = normals_ptr->GetDirectArray();
    for_each(m_lighting_normals.begin(), m_lighting_normals.end(),
      [&direct_array](const Geometry::Vector4& elem)
    {
      direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
    });

    auto& index_array = normals_ptr->GetIndexArray();
    for_each(tangents_idxs.begin(), tangents_idxs.end(),
      [&index_array](const uint32_t& idx) { index_array.Add(idx); });
  }

  mesh_ptr->BuildMeshEdgeArray();

  // build skeletons
  for_each(m_used_skeletons.begin(), m_used_skeletons.end(),
    [&scene_ptr, &mesh_node_ptr, this](const pair<string, shared_ptr<Skeleton>>& item)
  {
    if (item.second->get_lod_count() > m_lod_level)
    {
      item.second->set_current_lod(m_lod_level);
      item.second->generate_skeleton_in_scene(scene_ptr, mesh_node_ptr, this);
    }
  });

  // build morph targets
  // prepare base vector
  auto total_vertices = m_vertices.size();

  FbxBlendShape* blend_shape_ptr = FbxBlendShape::Create(scene_ptr, "BlendShapes");
  for (const auto& morph : m_morphs)
  {
    FbxBlendShapeChannel * morph_channel = FbxBlendShapeChannel::Create(scene_ptr, morph.get_name().c_str());
    FbxShape * shape = FbxShape::Create(scene_ptr, morph_channel->GetName());

    shape->InitControlPoints(static_cast<int>(total_vertices));
    auto shape_vertices = shape->GetControlPoints();

    // copy base vertices to shape vertices
    for (size_t idx = 0; idx < total_vertices; ++idx)
    {
      auto& pos = m_vertices[idx].get_position();
      shape_vertices[idx].Set(pos.x, pos.y, pos.z);
    }

    // apply morph
    for (auto& morph_pt : morph.get_positions())
    {
      size_t idx = morph_pt.first;
      auto& offset = morph_pt.second;
      auto& pos = m_vertices[idx].get_position();
      shape_vertices[idx].Set(pos.x + offset.x, pos.y + offset.y, pos.z + offset.z);
    }

    if (!normal_indexes.empty())
    {
      // get normals
      auto normal_element = shape->CreateElementNormal();
      normal_element->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
      normal_element->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

      auto& direct_array = normal_element->GetDirectArray();
      // set a base normals 
      for_each(m_normals.begin(), m_normals.end(),
        [&direct_array](const Geometry::Vector3& elem)
      {
        direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
      });

      for (auto &morph_normal : morph.get_normals())
      {
        uint32_t idx = morph_normal.first;
        auto& offset = morph_normal.second;
        auto& base = m_normals[idx];
        direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
      }

      auto& index_array = normal_element->GetIndexArray();
      for_each(normal_indexes.begin(), normal_indexes.end(),
        [&index_array](const uint32_t& idx) { index_array.Add(idx); });
    }

    if (!tangents_idxs.empty())
    {
      // get tangents
      auto tangents_ptr = shape->CreateElementTangent();
      tangents_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
      tangents_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

      auto& direct_array = tangents_ptr->GetDirectArray();
      for_each(m_lighting_normals.begin(), m_lighting_normals.end(),
        [&direct_array](const Geometry::Vector4& elem)
      {
        direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
      });

      for (auto& morph_tangent : morph.get_tangents())
      {
        uint32_t idx = morph_tangent.first;
        auto& offset = morph_tangent.second;
        auto& base = m_lighting_normals[idx];
        direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
      }

      auto& index_array = tangents_ptr->GetIndexArray();
      for_each(tangents_idxs.begin(), tangents_idxs.end(),
        [&index_array](const uint32_t& idx) { index_array.Add(idx); });
    }

    morph_channel->AddTargetShape(shape);
    blend_shape_ptr->AddBlendShapeChannel(morph_channel);
  }
  mesh_node_ptr->GetGeometry()->AddDeformer(blend_shape_ptr);

  FbxSystemUnit::cm.ConvertScene(scene_ptr);

  exporter_ptr->Export(scene_ptr);
  // cleanup
  fbx_manager_ptr->Destroy();
}

set<string> Animated_mesh::get_referenced_objects() const
{
  set<string> names;
  names.insert(m_skeletons_names.begin(), m_skeletons_names.end());
  for_each(m_shaders.begin(), m_shaders.end(),
    [&names](const Shader_appliance& shader) { names.insert(shader.get_name()); });
  return names;
}

void Animated_mesh::resolve_dependencies(const Context& context)
{
  // find object description through open by collection;
  auto it = context.opened_by.find(m_object_name);
  string opened_by_name;
  while (it != context.opened_by.end())
  {
    opened_by_name = it->second;
    it = context.opened_by.find(opened_by_name);
  }

  // get shaders
  for (auto it_shad = m_shaders.begin(); it_shad != m_shaders.end(); ++it_shad)
  {
    auto obj_it = context.object_list.find(it_shad->get_name());
    if (obj_it != context.object_list.end() && dynamic_pointer_cast<Shader>(obj_it->second))
      it_shad->set_definition(dynamic_pointer_cast<Shader>(obj_it->second));
  }

  // get skeletons
  for (auto it_skel = m_skeletons_names.begin(); it_skel != m_skeletons_names.end(); ++it_skel)
  {
    auto obj_it = context.object_list.find(*it_skel);
    if (obj_it != context.object_list.end() && dynamic_pointer_cast<Skeleton>(obj_it->second))
      m_used_skeletons.emplace_back(*it_skel, dynamic_pointer_cast<Skeleton>(obj_it->second));
  }

  if (!opened_by_name.empty() && m_used_skeletons.size() > 1)
  {
    // check if we opened through SAT and have multiple skeleton definitions
    //  then we have unite them to one, so we need SAT to get join information from it's skeleton info;
    auto it = context.object_list.find(opened_by_name);
    if (it != context.object_list.end())
    {
      auto cat_obj = std::dynamic_pointer_cast<Animated_object_descriptor>(it->second);
      if (cat_obj)
      {
        auto skel_count = cat_obj->get_skeletons_count();
        shared_ptr<Skeleton> root_skeleton;
        for (uint32_t skel_idx = 0; skel_idx < skel_count; ++skel_idx)
        {
          auto skel_name = cat_obj->get_skeleton_name(skel_idx);
          auto point_name = cat_obj->get_skeleton_attach_point(skel_idx);

          auto it = std::find_if(m_used_skeletons.begin(), m_used_skeletons.end(),
            [&skel_name](const pair<string, shared_ptr<Skeleton>>& info)
          {
            return (boost::iequals(skel_name, info.first));
          });

          if (point_name.empty() && it != m_used_skeletons.end())
            // mark this skeleton as root
            root_skeleton = it->second;
          else
          {
            // attach skeleton to root
            root_skeleton->join_skeleton_to_point(point_name, it->second);
            // remove it from used list
            m_used_skeletons.erase(it);
          }
        }
      }
    }
  }
}

void Animated_mesh::Shader_appliance::add_primitive()
{
  uint32_t new_primitive_position = static_cast<uint32_t>(m_triangles.size());
  if (!m_primitives.empty() && m_primitives.back().second == 0)
    close_primitive();

  m_primitives.emplace_back(new_primitive_position, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void Skeleton::generate_skeleton_in_scene(FbxScene* scene_ptr, FbxNode * parent_ptr, Animated_mesh* source_mesh)
{
  assert(parent_ptr != nullptr && scene_ptr != nullptr);

  auto bones_count = get_bones_count();
  vector<FbxNode*> nodes(bones_count, nullptr);
  vector<FbxCluster *> clusters(bones_count, nullptr);
  // Set bone attributes
  for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
  {
    auto& bone = get_bone(bone_num);

    FbxNode* node_ptr = FbxNode::Create(scene_ptr, bone.name.c_str());
    FbxSkeleton* skeleton_ptr = FbxSkeleton::Create(scene_ptr, bone.name.c_str());
    if (bone.parent_idx == -1)
      skeleton_ptr->SetSkeletonType(FbxSkeleton::eRoot);
    else
      skeleton_ptr->SetSkeletonType(FbxSkeleton::eLimbNode);

    node_ptr->SetNodeAttribute(skeleton_ptr);

    FbxQuaternion pre_rot_quat { bone.pre_rot_quaternion.x, bone.pre_rot_quaternion.y, bone.pre_rot_quaternion.z, bone.pre_rot_quaternion.a };
    FbxQuaternion post_rot_quat { bone.post_rot_quaternion.x, bone.post_rot_quaternion.y, bone.post_rot_quaternion.z, bone.post_rot_quaternion.a };
    FbxQuaternion bind_rot_quat { bone.bind_pose_rotation.x, bone.bind_pose_rotation.y, bone.bind_pose_rotation.z, bone.bind_pose_rotation.a };

    auto full_rot = post_rot_quat * bind_rot_quat * pre_rot_quat;

    node_ptr->SetPreRotation(FbxNode::eSourcePivot, pre_rot_quat.DecomposeSphericalXYZ());
    node_ptr->SetPostTargetRotation(post_rot_quat.DecomposeSphericalXYZ());

    node_ptr->LclRotation.Set(full_rot.DecomposeSphericalXYZ());
    node_ptr->LclTranslation.Set(FbxDouble3 { bone.bind_pose_transform.x, bone.bind_pose_transform.y, bone.bind_pose_transform.z });

    node_ptr->ResetPivotSetAndConvertAnimation();

    nodes[bone_num] = node_ptr;
  }

  // build hierarchy
  for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
  {
    auto& bone = get_bone(bone_num);
    auto idx_parent = bone.parent_idx;
    if (idx_parent == -1)
      parent_ptr->AddChild(nodes[bone_num]);
    else
    {
      auto& parent = nodes[idx_parent];
      parent->AddChild(nodes[bone_num]);
    }
  }

  // build bind pose

  auto mesh_attr = reinterpret_cast<FbxGeometry *>(parent_ptr->GetNodeAttribute());
  auto skin = FbxSkin::Create(scene_ptr, parent_ptr->GetName());
  auto xmatr = parent_ptr->EvaluateGlobalTransform();
  FbxAMatrix link_transform;

  // create clusters
  // create vertex index arrays for clusters
  const auto& vertices = source_mesh->get_vertices();
  map<string, vector<pair<uint32_t, float>>> cluster_vertices;
  const auto& mesh_joint_names = source_mesh->get_joint_names();
  for (uint32_t vertex_num = 0; vertex_num < vertices.size(); ++vertex_num)
  {
    const auto& vertex = vertices[vertex_num];
    for (const auto& weight : vertex.get_weights())
    {
      auto joint_name = mesh_joint_names[weight.first];
      boost::to_lower(joint_name);
      cluster_vertices[joint_name].emplace_back(vertex_num, weight.second);
    }
  }

  for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
  {
    auto& bone = get_bone(bone_num);
    auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
    cluster->SetLink(nodes[bone_num]);
    cluster->SetLinkMode(FbxCluster::eTotalOne);

    auto bone_name = bone.name;
    boost::to_lower(bone_name);

    if (cluster_vertices.find(bone_name) != cluster_vertices.end())
    {
      auto& cluster_vertex_array = cluster_vertices[bone_name];
      for (const auto& vertex_weight : cluster_vertex_array)
        cluster->AddControlPointIndex(vertex_weight.first, vertex_weight.second);
    }

    cluster->SetTransformMatrix(xmatr);
    link_transform = nodes[bone_num]->EvaluateGlobalTransform();
    cluster->SetTransformLinkMatrix(link_transform);

    clusters[bone_num] = cluster;
    skin->AddCluster(cluster);
  }
  mesh_attr->AddDeformer(skin);

  auto pose_ptr = FbxPose::Create(scene_ptr, parent_ptr->GetName());
  pose_ptr->SetIsBindPose(true);

  FbxAMatrix matrix;
  for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
  {
    matrix = nodes[bone_num]->EvaluateGlobalTransform();
    pose_ptr->Add(nodes[bone_num], matrix);
  }
  scene_ptr->AddPose(pose_ptr);
}

void Skeleton::join_skeleton_to_point(const string& attach_point, const shared_ptr<Skeleton>& skel_to_join)
{
  uint32_t attach_point_idx = numeric_limits<uint32_t>::max();
  auto lods_to_join = min(get_lod_count(), skel_to_join->get_lod_count());
  for (uint32_t lod = 0; lod < lods_to_join; ++lod)
  {
    // find attach point;
    for (uint32_t idx = 0; idx < m_bones[lod].size(); ++idx)
    {
      if (boost::iequals(attach_point, m_bones[lod][idx].name))
        attach_point_idx = idx;
    }

    // attach point found
    if (attach_point_idx < numeric_limits<uint32_t>::max())
    {
      auto& bones_to_join = skel_to_join->m_bones[lod];
      uint32_t bones_offset = static_cast<uint32_t>(m_bones[lod].size());
      for (uint32_t idx = 0; idx < bones_to_join.size(); ++idx)
      {
        auto bone = bones_to_join[idx];
        if (bone.parent_idx == -1)
          bone.parent_idx = attach_point_idx;
        else
        {
          bone.parent_idx += bones_offset;
        }
        m_bones[lod].push_back(bone);
      }
    }
  }
}


bool Skeleton::is_object_correct() const
{
  return !m_bones.empty();
}

void Skeleton::store(const std::string & path)
{
}

set<string> Skeleton::get_referenced_objects() const
{
  return set<string>();
}

void Skeleton::resolve_dependencies(const Context& context)
{
}

void Skeleton::set_object_name(const std::string & obj_name)
{
  m_skeleton_name = obj_name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

Shader::texture_type Shader::get_texture_type(const std::string & texture_tag)
{
  if (texture_tag == "MAIN")
    return texture_type::main;
  else if (texture_tag == "NRML")
    return texture_type::normal;
  else if (texture_tag == "DOT3")
    return texture_type::lightmap;
  else if (texture_tag == "SPEC")
    return texture_type::specular;
  return texture_type::none_type;
}

set<string> Shader::get_referenced_objects() const
{
  set<string> result;
  for_each(m_textures.begin(), m_textures.end(),
    [&result](const Texture& texture) { result.insert(texture.tex_file_name); });

  for_each(m_palettes.begin(), m_palettes.end(),
    [&result](const Palette& palette) { result.insert(palette.palette_file); });

  return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

shared_ptr<DDS_Texture> DDS_Texture::construct(const string& name, const uint8_t* buffer, size_t buf_size)
{
  DirectX::TexMetadata meta;
  shared_ptr<DirectX::ScratchImage> image = make_shared<DirectX::ScratchImage>();
  HRESULT hr = DirectX::LoadFromDDSMemory(buffer, buf_size, DirectX::DDS_FLAGS_NONE, &meta, *image);
  if (SUCCEEDED(hr))
  {
    shared_ptr<DDS_Texture> ret_object = make_shared<DDS_Texture>();
    ret_object->set_object_name(name);
    ret_object->m_image = image;
    return ret_object;
  }

  return nullptr;
}

void DDS_Texture::store(const string& path)
{
  boost::filesystem::path out_path(path);

  out_path /= m_name;
  out_path.replace_extension("tga");
  out_path.normalize();

  auto directory = out_path.parent_path();
  if (!boost::filesystem::exists(directory))
    boost::filesystem::create_directories(directory);

  auto image = m_image->GetImage(0, 0, 0);
  const DirectX::Image* out_image = nullptr;
  DirectX::ScratchImage decompressed;
  if (DirectX::IsCompressed(image->format))
  {
    DirectX::Decompress(*image, DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
    out_image = decompressed.GetImage(0, 0, 0);
  }
  else
    out_image = image;

  DirectX::SaveToTGAFile(*out_image, out_path.wstring().c_str());
}
