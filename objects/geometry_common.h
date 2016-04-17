#pragma once

namespace Geometry
{
struct Point
{
  float x = 0;
  float y = 0;
  float z = 0;

  Point() { }
  Point(float x_, float y_, float z_) : x(x_), y(y_), z(z_) { }

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
  float a = 0;

  Vector4() { }
  Vector4(float x_, float y_, float z_, float a_) : Point(x_, y_, z_), a(a_) { }
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
}

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
  float a;
  float r;
  float g;
  float b;
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