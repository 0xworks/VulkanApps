struct RayPayload
{
   vec4 colorAndDistance; // rgb + t
   vec4 scatterDirection; // direction + isScattered
   uint randomSeed;
};
