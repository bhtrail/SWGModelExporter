#pragma once

namespace Geometry
{
  struct Point
  {
    float x;
    float y;
    float z;

    bool operator < (const Point& right)
    {
      if (x != right.x)
        return x < right.x;
      if (y != right.y)
        return y < right.y;

      return z < right.z;
    }
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
    Tex_coord(float t1, float t2) : u(t1), v(t2) { }
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

  struct Triangle_indexed
  {
    Triangle_indexed(uint32_t v1, uint32_t v2, uint32_t v3)
    {
      points[0] = v1; points[1] = v2; points[2] = v3;
    }
    uint32_t points[3];
  };
} // Geometry