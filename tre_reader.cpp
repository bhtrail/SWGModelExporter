#include "stdafx.h"
#include "tre_reader.h"

namespace Tre_navigator
{
  Tre_reader::Tre_reader(const std::wstring & filename)
    : m_filename(filename)
  {
    m_stream.open(filename, std::ios_base::binary);

    read_header_();
    read_resource_block_();
    read_names_block_();
    build_lookup_table_();
  }

  bool Tre_reader::is_resource_present(const std::string & res_name)
  {
    auto result = m_lookup.find(res_name);
    return (result != m_lookup.end());
  }

  std::string Tre_reader::get_resource_name(uint32_t index)
  {
    if (index >= m_resources.size())
      return std::move(std::string(""));
    std::string name = (char *)(m_names.data() + m_resources[index].name_offset);
    return std::move(name);
  }

  bool Tre_reader::get_resource(uint32_t index, std::vector<char>& buffer)
  {
    if (index >= m_resources.size())
      return false;

    auto& res_info = m_resources[index];

    if (res_info.data_size > 0)
    {
      if (buffer.size() < res_info.data_size)
        buffer.resize(res_info.data_size);

      read_block_(res_info.data_offset,
                  res_info.data_compression,
                  res_info.data_compressed_size,
                  res_info.data_size,
                  buffer.data());
    }
    return true;
  }

  bool Tre_reader::get_resource(const std::string & res_name, std::vector<char>& buffer)
  {
    auto index = get_resourece_index_(res_name);
    if (index == size_t(-1))
      return false;

    return get_resource(static_cast<uint32_t>(index), buffer);
  }

  void Tre_reader::read_header_()
  {
    m_stream.read(reinterpret_cast<char *>(&m_header), sizeof(m_header));

    if (strncmp(m_header.file_type, "EERT", 4) != 0)
      throw new std::runtime_error("Invalid TRE file format");

    if (strncmp(m_header.file_version, "5000", 4) != 0)
      throw new std::runtime_error("Invalid TRE file format");
  }

  void Tre_reader::read_resource_block_()
  {
    m_resources.resize(m_header.resource_count);
    uint32_t uncomp_size = m_header.resource_count * sizeof(Resource_info);

    read_block_(m_header.info_offset,
                m_header.info_compression,
                m_header.info_compressed_size,
                uncomp_size,
                reinterpret_cast<char *>(m_resources.data()));
  }

  void Tre_reader::read_names_block_()
  {
    m_names.resize(m_header.name_uncompressed_size);

    auto name_offset = m_header.info_offset + m_header.info_compressed_size;

    read_block_(
      name_offset,
      m_header.name_compression,
      m_header.name_compressed_size,
      m_header.name_uncompressed_size,
      m_names.data());
  }

  void Tre_reader::build_lookup_table_()
  {
    for (size_t index = 0; index < m_resources.size(); ++index)
    {
      std::string resource_name(get_resource_name(static_cast<uint32_t>(index)));
      m_lookup.insert(std::make_pair(resource_name, index));
    }
  }

  size_t Tre_reader::get_resourece_index_(const std::string & name)
  {
    auto result = m_lookup.find(name);
    if (result == m_lookup.end())
      return size_t(-1);

    return result->second;
  }

  void Tre_reader::read_block_(uint32_t offset, uint32_t compression, uint32_t comp_size, uint32_t uncomp_size, char * buffer)
  {
    if (buffer == nullptr)
      return;

    if (compression == 0)
    {
      m_stream.seekg(offset, std::ios_base::beg);
      m_stream.read(buffer, uncomp_size);
    }
    else if (compression == 2)
    {
      std::vector<char> compressed_data(comp_size);

      m_stream.seekg(offset, std::ios_base::beg);
      m_stream.read(compressed_data.data(), comp_size);

      z_stream stream;
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;
      stream.avail_in = Z_NULL;
      stream.next_in = Z_NULL;
      auto result = inflateInit(&stream);
      if (result != Z_OK)
        throw new std::runtime_error("Zlib error");

      stream.next_in = reinterpret_cast<Bytef *>(compressed_data.data());
      stream.avail_in = comp_size;
      stream.next_out = reinterpret_cast<Bytef *>(buffer);
      stream.avail_out = uncomp_size;

      inflate(&stream, Z_FINISH);
      if (stream.total_out > 0)
        inflateEnd(&stream);
    }
    else
      throw new std::runtime_error("Unknown format");
  }
}