#include "stdafx.h"
#include "parser_selector.h"
#include "cat_parser.h"


Parser_selector::Parser_selector()
{ }

void Parser_selector::section_begin(const std::string& name, uint8_t * data_ptr, size_t data_size, uint32_t depth)
{
  if (depth == 1)
  {
    // at depth 0 we have base FORM object
    if (name == "SMATFORM")
      m_selected_parser = std::make_shared<cat_parser>();
  }

  if (m_selected_parser)
    m_selected_parser->section_begin(name, data_ptr, data_size, depth);
}

void Parser_selector::parse_data(const std::string& name, uint8_t * data_ptr, size_t data_size)
{
  if (m_selected_parser)
    m_selected_parser->parse_data(name, data_ptr, data_size);
}
