#include "stdafx.h"
#include "parser_selector.h"
#include "cat_parser.h"
#include "lmg_parser.h"
#include "mgn_parser.h"
#include "skt_parser.h"


Parser_selector::Parser_selector()
{ }

void Parser_selector::section_begin(const std::string& name, uint8_t * data_ptr, size_t data_size, uint32_t depth)
{
  if (depth == 1)
  {
    // at depth 0 we have base FORM object
    if (name == "SMATFORM")
      m_selected_parser = std::make_shared<cat_parser>();
    else if (name == "MLODFORM")
      m_selected_parser = std::make_shared<lmg_parser>();
    else if (name == "SKMGFORM")
      m_selected_parser = std::make_shared<mgn_parser>();
    else if (name == "SLODFORM")
      m_selected_parser = std::make_shared<skt_parser>();
    else
      m_selected_parser = nullptr;
  }

  if (m_selected_parser)
    m_selected_parser->section_begin(name, data_ptr, data_size, depth);
}

void Parser_selector::parse_data(const std::string& name, uint8_t * data_ptr, size_t data_size)
{
  if (m_selected_parser)
    m_selected_parser->parse_data(name, data_ptr, data_size);
}

void Parser_selector::section_end(uint32_t depth)
{
  if (m_selected_parser)
    m_selected_parser->section_end(depth);
}