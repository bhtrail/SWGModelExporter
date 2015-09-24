#include "stdafx.h"
#include "tre_library.h"

namespace Tre_navigator
{
  namespace fs = std::experimental::filesystem;

  Tre_library::Tre_library(const std::string & path, Tre_library_reader_callback * callback_ptr)
  {
    fs::directory_iterator file_enum(path);
    std::vector<std::string> files;

    for (auto& file_item : file_enum)
    {
      if (fs::is_regular_file(file_item) && file_item.path().extension() == ".tre")
        files.push_back(file_item.path().string());
    }

    std::sort(files.begin(), files.end());
    if (callback_ptr != nullptr)
      callback_ptr->number_of_files(files.size());

    for (auto& filename : files)
    {
      auto reader = std::make_shared<Tre_reader>(filename);
      m_readers.push_back(reader);

      auto resources_num = reader->get_resource_count();
      for (uint32_t idx = 0; idx < resources_num; ++idx)
      {
        auto res_name = reader->get_resource_name(idx);
        m_reader_lookup.insert(std::make_pair(res_name, reader));
      }
      if (callback_ptr != nullptr)
        callback_ptr->file_read();
    }
  }

  bool Tre_library::is_object_present(const std::string & name)
  {
    auto result = m_reader_lookup.find(name);

    return result != m_reader_lookup.end();
  }

  bool Tre_library::get_object_name(const std::string & partial_name, std::string & full_name)
  {
    std::regex pattern("(.*)" + partial_name);
    auto result = std::find_if(m_reader_lookup.begin(), m_reader_lookup.end(),
                               [&pattern](const std::pair<std::string, std::weak_ptr<Tre_reader>> &item)
    {
      return std::regex_match(item.first, pattern);
    });

    auto ret = result != m_reader_lookup.end();
    if (ret)
      full_name = result->first;
    return ret;
  }

  size_t Tre_library::number_of_object_versions(const std::string & name)
  {
    auto object = m_reader_lookup.find(name);
    if (object != m_reader_lookup.end())
    {
      auto readers_range = m_reader_lookup.equal_range(name);
      return std::distance(readers_range.first, readers_range.second);
    }

    return size_t(-1);
  }

  std::vector<std::weak_ptr<Tre_reader>> Tre_library::get_versioned_readers(const std::string & name)
  {
    std::vector<std::weak_ptr<Tre_reader>> result;

    auto object = m_reader_lookup.find(name);
    if (object != m_reader_lookup.end())
    {
      auto range = m_reader_lookup.equal_range(name);
      for (auto it = range.first; it != range.second; ++it)
        result.push_back(it->second);
    }
    return result;
  }

  bool Tre_library::get_object(const std::string & name, std::vector<uint8_t>& buffer, size_t version_num)
  {
    std::string fullname;
    if (!is_object_present(name))
    {
      if (!get_object_name(name, fullname))
        return false;
    }
    else
      fullname = name;

    auto readers = get_versioned_readers(fullname);
    if (version_num > readers.size())
      return false;

    auto reader_lock = readers[version_num].lock();
    if (reader_lock)
      return reader_lock->get_resource(fullname, buffer);

    return false;
  }
}
