//
// Shared by C++ application code and glsl shader code.
struct Constants {
   uint minRayBounces; // the minimum number of ray bounces before we will consider terminating rays (via Russian Roulette)
   uint maxRayBounces;
   float lensAperture;
   float lensFocalLength;
};
