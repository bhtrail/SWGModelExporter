// SWGModelExporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tre_reader.h"
#include "tre_library.h"
#include "IFF_file.h"
#include "parsers/parser_selector.h"

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
    ("object", po::value<string>(&object_name)->required(), "name of object to extract")
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

  fs::path output_path(output_pathname);

  File_read_callback read_callback;
  std::cout << "Loaading TRE library..." << std::endl;

  auto library = make_shared<Tre_library>(swg_path, &read_callback);
  string full_name;
  cout << "Looking for object" << endl;

  queue<string> objects_to_process;

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

  while (objects_to_process.empty() == false)
  {
    full_name = objects_to_process.front();
    objects_to_process.pop();

    std::vector<uint8_t> buffer;
    if (!library->get_object(full_name, buffer))
      continue;

    IFF_file iff_file(buffer);

    shared_ptr<Parser_selector> parser = make_shared<Parser_selector>();
    iff_file.full_process(parser);
    if (parser->is_object_parsed())
    {
      auto object = parser->get_parsed_object();
      object->set_object_name(full_name);
      object->store(output_pathname);

      auto references_objects = object->get_referenced_objects();
      if (!references_objects.empty())
        for(const auto& object_name : references_objects)
          objects_to_process.push(object_name);
    }
    else
      std::cout << "Objects of this type could not be converted at this time. Sorry!" << std::endl;
  } 

  return 0;
}
