#include <emscripten.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#define VHACD_DISABLE_THREADING 1
#include "../include/VHACD.h"

using namespace VHACD;

class JsHull {
  double const* m_points;
  uint32_t m_nPoints;
  uint32_t const* m_triangles;
  uint32_t m_nTriangles;
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
    return m_vhacd->Compute(points, nPoints, triangles, nTriangles, m_parameters);
  }
};
