#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

#define ENABLE_VHACD_IMPLEMENTATION 1
#define VHACD_DISABLE_THREADING 1
#include "../include/VHACD.h"

using namespace VHACD;

class JsHull {
private:
  IVHACD::ConvexHull const& m_hull;
public:
  explicit JsHull(IVHACD::ConvexHull const& hull) : m_hull(hull) { }

  double const* GetPoints() const { return &(m_hull.m_points[0].mX); }
  uint32_t GetNumPoints() const { return static_cast<uint32_t>(m_hull.m_points.size()); }
  uint32_t const* GetTriangles() const { return &(m_hull.m_triangles[0].mI0); }
  uint32_t GetNumTriangles() const { return static_cast<uint32_t>(m_hull.m_triangles.size()); }
};

class JsVHACD {
private:
  IVHACD* m_vhacd;
  IVHACD::Parameters m_parameters;
public:
  JsVHACD(IVHACD::Parameters const& parameters) : m_vhacd(CreateVHACD()), m_parameters(parameters) { }

  ~JsVHACD() {
    m_vhacd->Release();
  }

  void Dispose() {
    delete this;
  }

  // Compute convex hulls for the specified mesh, writing the results into `output`.
  // @param output Buffer to hold the output. It must be large enough to hold the maximum number of convex hulls specified in the Parameters passed to the constructor.
  // @returns The number of convex hulls actually produced.
  // Any space in the output array beyond that required for the produced hulls is left untouched.
  // The pointers in `output` refer to memory owned by the JsVHACD object. They remain valid until Dispose is invoked, at which point they are freed.
  uint32_t Compute(JsHull* output, const double* points, uint32_t nPoints, const uint32_t* triangles, uint32_t nTriangles) {
    if (!m_vhacd->Compute(points, nPoints, triangles, nTriangles, m_parameters))
      return 0;

    return m_vhacd->GetNConvexHulls();
  }
};

EMSCRIPTEN_BINDINGS(vhacdjs) {
  class_<JsHull>("ConvexHull")
    // These are functions instead of properties because properties can specify raw pointer policy.
    .function("getPoints", &JsHull::GetPoints, allow_raw_pointers())
    .function("getTriangles", &JsHull::GetTriangles, allow_raw_pointers())
    .property("numPoints", &JsHull::GetNumPoints)
    .property("numTriangles", &JsHull::GetNumTriangles);
}
