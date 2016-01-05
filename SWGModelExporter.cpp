// SWGModelExporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tre_reader.h"
#include "tre_library.h"
#include "IFF_file.h"
#include "parsers/parser_selector.h"
#include "objects/animated_object.h"

//////
#include "objects/static_object.h"

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace Tre_navigator;

class File_read_callback : public Tre_library_reader_callback
{
public:
  File_read_callback()
  { }
  virtual void number_of_files(size_t file_num) override
  {
    if (m_display == nullptr)
      m_display = make_shared<boost::progress_display>(static_cast<unsigned long>(file_num));
  }
  virtual void file_read() override
  {
    if (m_display)
      m_display->operator++();
  }

private:
  std::shared_ptr<boost::progress_display> m_display;
};

int _tmain(int argc, _TCHAR* argv[])
{
  std::string swg_path;
  std::string object_name;
  std::string output_pathname;

  po::options_description flags("Program options");
  flags.add_options()
    ("help", "get this help message")
    ("swg-path", po::value<string>(&swg_path)->required(), "path to Star Wars Galaxies")
    ("object", po::value<string>(&object_name)->required(), "name of object to extract. use batch:<ext> to extract all files of given ext")
    ("output-path", po::value<string>(&output_pathname)->required(), "path to output location");

  try
  {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, flags), vm);
    po::notify(vm);
  }
  catch (...)
  {
    std::cout << flags << std::endl;
    return -1;
  }

  CoInitialize(NULL);

  fs::path output_path(output_pathname);

  File_read_callback read_callback;
  std::cout << "Loaading TRE library..." << std::endl;

  auto library = make_shared<Tre_library>(swg_path, &read_callback);
  string full_name;
  std::cout << "Looking for object" << endl;

  queue<string> objects_to_process;
  set<string> unknown_objects;

  // check if we in batch mode by given object name special look
  boost::char_separator<char> separators(":");
  boost::tokenizer<boost::char_separator<char>> object_name_tokens(object_name, separators);
  vector<string> tokens(object_name_tokens.begin(), object_name_tokens.end());
  if (tokens.size() > 1)
  {
    if (tokens[0] != "batch")
    {
      cout << "Incorrect format for batch mode" << endl;
      return 0;
    }

    auto filetype = tokens[1];

    vector<string> selected_objects;
    if (library->select_objects_by_ext(filetype, selected_objects))
    {
      for (const auto& obj_name : selected_objects)
        objects_to_process.push(obj_name);
    }
    else
    {
      cout << "no object selected for batch - extension is wrong?";
      return 0;
    }
  }
  else
  {
    // normalize filename
    replace_if(object_name.begin(), object_name.end(), [](const char& value) { return value == '\\'; }, '/');
    if (library->is_object_present(object_name))
    {
      objects_to_process.push(object_name);
    }
    else if (library->get_object_name(object_name, full_name))
    {
      objects_to_process.push(full_name);
    }
    else
      std::cout << "Object with name \"" << object_name << "\" has not been found" << std::endl;
  }

  Context context;

  while (objects_to_process.empty() == false)
  {
    full_name = objects_to_process.front();
    objects_to_process.pop();

    replace_if(full_name.begin(), full_name.end(), [](const char& value) { return value == '\\'; }, '/');
    string ext = full_name.substr(full_name.length() - 3);
    boost::to_lower(ext);

    // skip already parsed object
    if (context.object_list.find(full_name) != context.object_list.end())
      continue;

    cout << "Processing : " << full_name << endl;
    std::vector<uint8_t> buffer;
    if (!library->get_object(full_name, buffer))
      continue;

    //special processing for pure binary files (WAV, DDS, TGA, etc)
    if (ext == "dds")
    {
      auto texture = DDS_Texture::construct(full_name, buffer.data(), buffer.size());
      if (texture)
        context.object_list.insert(make_pair(full_name, dynamic_pointer_cast<Base_object>(texture)));

      continue;
    }

    IFF_file iff_file(buffer);

    shared_ptr<Parser_selector> parser = make_shared<Parser_selector>();
    iff_file.full_process(parser);
    if (parser->is_object_parsed())
    {
      auto object = parser->get_parsed_object();
      if (object)
      {
        object->set_object_name(full_name);
        context.object_list.insert(make_pair(full_name, object));

        auto references_objects = object->get_referenced_objects();
        std::for_each(references_objects.begin(), references_objects.end(),
          [&context, &objects_to_process, &full_name](const string& object_name)
        {
          if (context.object_list.find(object_name) == context.object_list.end() &&
            context.unknown.find(object_name) == context.unknown.end())
          {
            objects_to_process.push(object_name);
            context.opened_by[object_name] = full_name;
          }
        }
        );
      }
    }
    else
    {
      std::cout << "Objects of this type could not be converted at this time. Sorry!" << std::endl;
      context.unknown.insert(full_name);
    }
  }

  std::cout << "Resolve dependencies..." << endl;
  std::for_each(context.object_list.begin(), context.object_list.end(),
    [&context](const pair<string, shared_ptr<Base_object>>& item)
  {
    std::cout << "Object : " << item.first;
    item.second->resolve_dependencies(context);
    std::cout << " done." << endl;
  });

  std::cout << "Store objects..." << endl;
  for_each(context.object_list.begin(), context.object_list.end(),
    [&output_pathname](const pair<string, shared_ptr<Base_object>>& item)
  {
    std::cout << "Object : " << item.first;
    item.second->store(output_pathname);
    std::cout << " done." << endl;
  });

  CoUninitialize();
  return 0;
}
