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
  ios_ptr->SetIntProp(EXP_FBX_EXPORT_FILE_VERSION, FBX_FILE_VERSION_7000);
  fbx_manager_ptr->SetIOSettings(ios_ptr);

  FbxExporter* exporter_ptr = FbxExporter::Create(fbx_manager_ptr, "");
  if (!exporter_ptr)
    return;

  exporter_ptr->SetFileExportVersion(FBX_2013_00_COMPATIBLE);
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

    // create material for this shader
    auto material_ptr = FbxSurfacePhong::Create(scene_ptr, shader.get_name().c_str());
    material_ptr->ShadingModel.Set("Phong");
    material_ptr->Ambient.Set(FbxDouble3(0.7, 0.7, 0.7));
    mesh_ptr->GetNode()->AddMaterial(material_ptr);

    // get geometry element
    auto& triangles = shader.get_triangles();
    auto& positions = shader.get_pos_indexes();
    auto& normals = shader.get_normal_indexes();
    auto& tangents = shader.get_light_indexes();

    uint32_t idx_offset = static_cast<uint32_t>(uvs.size());
    copy(shader.get_texels().begin(), shader.get_texels().end(), back_inserter(uvs));

    normal_indexes.reserve(normal_indexes.size() + normals.size());
    tangents_idxs.reserve(tangents_idxs.size() + tangents.size());

    for (uint32_t tri_idx = 0; tri_idx < triangles.size(); ++tri_idx)
    {
      auto& tri = triangles[tri_idx];
      mesh_ptr->BeginPolygon(shader_idx, -1, shader_idx, false);
      for (size_t i = 0; i < 3; ++i)
      {
        uint32_t remapped_pos_idx = positions[tri.points[i]];
        mesh_ptr->AddPolygon(remapped_pos_idx);

        uint32_t remapped_normal_idx = normals[tri.points[i]];
        normal_indexes.emplace_back(remapped_normal_idx);

        if (!tangents.empty())
        {
          uint32_t remapped_tangent = tangents[tri.points[i]];
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
    [&names](const Shader& shader) { names.insert(shader.get_name()); });
  return names;
}

void Animated_mesh::resolve_dependencies(const Object_cache& obj_list)
{
  // get skeletons
  for (auto it_skel = m_skeletons_names.begin(); it_skel != m_skeletons_names.end(); ++it_skel)
  {
    auto obj_it = obj_list.find(*it_skel);
    if (obj_it != obj_list.end() && dynamic_pointer_cast<Skeleton>(obj_it->second))
      m_used_skeletons.emplace_back(*it_skel, dynamic_pointer_cast<Skeleton>(obj_it->second));
  }
}

void Animated_mesh::Shader::add_primitive()
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

    auto full_rot = pre_rot_quat * bind_rot_quat * post_rot_quat;

    node_ptr->LclRotation.Set(full_rot.DecomposeSphericalXYZ());
    node_ptr->LclTranslation.Set(FbxDouble3 { bone.bind_pose_transform.x, bone.bind_pose_transform.y, bone.bind_pose_transform.z });

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
      const auto& joint_name = mesh_joint_names[weight.first];
      cluster_vertices[joint_name].emplace_back(vertex_num, weight.second);
    }
  }

  for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
  {
    auto& bone = get_bone(bone_num);
    auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
    cluster->SetLink(nodes[bone_num]);
    cluster->SetLinkMode(FbxCluster::eTotalOne);

    if (cluster_vertices.find(bone.name) != cluster_vertices.end())
    {
      auto& cluster_vertex_array = cluster_vertices[bone.name];
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
  auto matrix = parent_ptr->EvaluateGlobalTransform();
  pose_ptr->Add(parent_ptr, matrix);

  for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
  {
    matrix = nodes[bone_num]->EvaluateGlobalTransform();
    pose_ptr->Add(nodes[bone_num], matrix);
  }
  scene_ptr->AddPose(pose_ptr);
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

void Skeleton::resolve_dependencies(const Object_cache & object_list)
{
}

void Skeleton::set_object_name(const std::string & obj_name)
{
  m_skeleton_name = obj_name;
}
