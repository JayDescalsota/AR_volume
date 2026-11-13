#pragma once

// Stable C ABI used by the Swift, Kotlin and JSI layers.
// All point arrays are flat triples: x0,y0,z0,x1,y1,z1,...
// Ray arrays are flat sextuples: ox,oy,oz,dx,dy,dz,...
// Planes are nx,ny,nz,d.

#ifdef __cplusplus
extern "C" {
#endif

// intrinsics4 = fx,fy,cx,cy. camToWorld16 is row-major.
// outOrigin3 / outDir3 receive the world-space ray.
// Returns 1 on success.
int vm_unproject_ray(const double* intrinsics4, const double* camToWorld16,
                     double px, double py, double* outOrigin3, double* outDir3);

// outStats4 = rms, baseline, maxParallaxRad, confidence.
// Returns 1 when the triangulation is valid.
int vm_triangulate(const double* rays6, int rayCount, double* outPoint3,
                   double* outStats4);

// Square metres.
double vm_polygon_area(const double* points3, int count);

// outStats4 = volume, area, height, statusCode.
// Returns 1 on success.
int vm_volume_extrusion(const double* footprint3, int count, double height,
                        double* outStats4);

int vm_volume_plane_bounded(const double* footprint3, int count,
                            const double* groundPlane4,
                            const double* roofPlane4, const double* up3,
                            double* outStats4);

int vm_volume_polyhedron(const double* bottom3, const double* top3, int count,
                         double* outStats4);

// Vertical extent along up3.
double vm_height_from_taps(const double* base3, const double* top3,
                           const double* up3);

// Release any process-global resources (currently a no-op, reserved).
void vm_shutdown(void);

#ifdef __cplusplus
}
#endif
