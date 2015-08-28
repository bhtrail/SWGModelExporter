// SWGModelExporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tre_reader.h"
#include "tre_library.h"

using namespace std;
namespace fs = std::experimental::filesystem;

void show_usage_guide();

class File_read_callback : public Tre_navigator::Tre_library_reader_callback
{
public:
  File_read_callback()
  { }
  virtual void number_of_files(size_t file_num) override
  {
    if (m_display == nullptr)
      m_display = std::make_shared<boost::progress_display>(static_cast<unsigned long>(file_num));
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
  if (argc < 3)
  {
    show_usage_guide();
    return -1;
  }

  fs::path output_path(argv[3]);

  std::wstring object_name(argv[2]);

  File_read_callback read_callback;
  Tre_navigator::Tre_library library(argv[1], &read_callback);
  string full_name;
  if (library.get_object_name("acklay.sat", full_name))
  {
    library.is_object_present(full_name);
  }
  return 0;
}

void show_usage_guide()
{
  cout << "Should have exactly 3 parameters in command line" << endl;
  cout << "\t<path to SWG directory> - path to location of TRE files" << endl;
  cout << "\t<name of object to export> - name of object to export" << endl;
  cout << "\t<path to directory to store data> - path to location to store exported data" << endl;
}
