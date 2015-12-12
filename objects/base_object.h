#pragma once

class Base_object;
typedef std::unordered_map<std::string, std::shared_ptr<Base_object>> Object_cache;

class Base_object
{
public:
  virtual ~Base_object() { }
  virtual bool is_object_correct() const = 0;
  virtual void store(const std::string& path) = 0;
  virtual std::set<std::string> get_referenced_objects() const = 0;
  virtual void resolve_dependencies(const Object_cache& object_list) = 0;
  virtual void set_object_name(const std::string& obj_name) = 0;
};