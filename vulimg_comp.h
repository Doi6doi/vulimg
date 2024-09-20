#ifndef VULIMG_COMPH
#define VULIMG_COMPH

#ifdef VULKAN

#define INT int
#define UINT uint
#define STRUCT

#else

#define INT int32_t
#define UINT uint32_t
#define U16 uint16_t
#define STRUCT struct
#pragma pack(push,1)

#endif

#define UGR 16
#define FLOAT float

#ifdef VULKAN
struct VigRect {
   UINT dx_dy;
   UINT weight;
   UINT left_top;
   UINT width_height;
};
#else
struct VigRect {
   U16 dx;
   U16 dy;
   UINT weight;
   U16 left;
   U16 top;
   U16 width;
   U16 height;
};
#endif

struct VigImgParam {
   UINT width;
   UINT height;
   UINT stride;
};

#ifdef VULKAN
struct VigTransform {
   FLOAT sx, ry, rx, sy, dx, dy;
};
#endif

struct VigCopyParams {
   STRUCT VigImgParam src;
   STRUCT VigImgParam dst;
   UINT sleft;
   UINT stop;
   UINT width;
   UINT height;
   UINT dleft;
   UINT dtop;
};

struct VigTransParams {
   STRUCT VigImgParam src;
   STRUCT VigImgParam dst;
   STRUCT VigTransform trans;
   INT compBits;
   INT compCount;
};

struct VigPyrParams {
   STRUCT VigImgParam src;
   STRUCT VigImgParam dst;
   INT compBits;
   INT compCount;
   UINT width;
   UINT height;
   UINT row;
};

struct VigWhiteParams {
   STRUCT VigImgParam img;
   FLOAT limit;
   UINT phase;
   UINT count;
};

struct VigJoinParams {
   UINT width;
   UINT height;
   UINT srcstride;
   UINT dststride;
   UINT index;
};

struct VigDSumParams {
   UINT mode;
   UINT astride;
   UINT bstride;
   UINT aleft;
   UINT atop;
   UINT bleft;
   UINT btop;
   UINT width;
   UINT height;
};

struct VigDeltaParams {
   STRUCT VigImgParam img;
   UINT compCount;
   UINT pixel;
   INT min;
   INT max;
};

#ifndef VULKAN

#pragma pack(pop)

#endif

#endif // VULIMG_COMPH
