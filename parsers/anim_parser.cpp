#include "stdafx.h"
#include "anim_parser.h"

using namespace std;

void anim_parser::section_begin(const string & name, uint8_t * data_ptr, size_t data_size, uint32_t depth)
{
  if (name == "CKATFORM")
    m_animation = make_shared<Animation>();
}

void anim_parser::parse_data(const string & name, uint8_t * data_ptr, size_t data_size)
{
  if (!m_animation)
    return;

  base_buffer buffer(data_ptr, data_size);

  if (name == "0001INFO")
  {
    Animation::Info info;
    info.FPS = buffer.read_float();
    info.frame_count = buffer.read_uint16();
    info.transform_count = buffer.read_uint16();
    info.rotation_channel_count = buffer.read_uint16();
    info.static_rotation_count = buffer.read_uint16();
    info.translation_channel_count = buffer.read_uint16();
    info.static_translation_count = buffer.read_uint16();

    m_animation->set_info(info);
  }
  else if (name == "XFRMXFIN" || name == "XFIN")
  {
    Animation::Bone_info bone;

    bone.name = buffer.read_stringz();
    bone.has_rotations = buffer.read_uint8() == 1;
    bone.rotation_channel_id = buffer.read_uint16();
    bone.translation_mask = buffer.read_uint8();
    bone.x_translation_channel_id = buffer.read_uint16();
    bone.y_translation_channel_id = buffer.read_uint16();
    bone.z_translation_channel_id = buffer.read_uint16();

    m_animation->get_bones().push_back(bone);
  }
  else if (name == "AROTQCHN" || name == "QCHN")
  {
    auto key_count = buffer.read_uint16();
    auto x_format = buffer.read_uint8();
    auto y_format = buffer.read_uint8();
    auto z_format = buffer.read_uint8();

    for (uint16_t count = 0; count < key_count; count++)
    {
      auto frame_num = buffer.read_uint16();
      auto x = buffer.read_uint8() / 255.f;
      auto y = buffer.read_uint8() / 255.f;
      auto z = buffer.read_uint8() / 255.f;
      auto w = buffer.read_uint8() / 255.f;

      Geometry::Vector4 vec { x, y, z, w };
    }
  }
}
