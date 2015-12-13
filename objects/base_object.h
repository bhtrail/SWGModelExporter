#pragma once

class Base_object;
typedef std::unordered_map<std::string, std::shared_ptr<Base_object>> Object_cache;
// keeps hierarchy of transitions in form object_name opened by object name
typedef std::unordered_map<std::string, std::string> Objects_opened_by;

class Base_object
{
public:
  virtual ~Base_object() { }
  virtual bool is_object_correct() const = 0;
  virtual void store(const std::string& path) = 0;
  virtual std::set<std::string> get_referenced_objects() const = 0;
  virtual void resolve_dependencies(const Object_cache& object_list, const Objects_opened_by& open_by) = 0;
  virtual void set_object_name(const std::string& obj_name) = 0;
  virtual std::string get_object_name() const = 0;
};