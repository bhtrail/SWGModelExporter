#pragma once

class Base_object
{
public:
  virtual ~Base_object() { }
  virtual bool is_object_correct() const = 0;
  virtual void store(const std::string& path) = 0;
};