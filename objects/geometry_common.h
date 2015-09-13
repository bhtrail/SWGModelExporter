#pragma once

namespace Geometry
{
  struct Point
  {
    float x;
    float y;
    float z;
  };

  typedef Point Vector3;

  struct Vector4 : public Vector3
  {
    float a;
  };

  struct Sphere
  {
    Point center;
    float radius;
  };

  struct Box
  {
    Point min;
    Point max;
  };
};

namespace Graphics
{
  struct Tex_coord
  {
    float u;
    float v;
  };

  struct Color
  {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };
} // Geometry