/* orh.h - v0.51 - C++ utility library. Includes types, math, string, memory arena, and other stuff.

In _one_ C++ file, #define ORH_IMPLEMENTATION before including this header to create the
 implementation. 
Like this:
#include ...
#include ...
#define ORH_IMPLEMENTATION
#include "orh.h"

REVISION HISTORY:
0.51 - added array_find_index().
0.50 - Improved arenas and added thread local scratch arenas.
0.49 - added context cracking.
0.48 - added operator!= for String8.
0.47 - added fps_max option.
0.46 - arena_init() now takes max size as parameter. And changed default auto_align from 8 to 1.
0.45 - added f32 version of move_towards().
0.44 - added fullscreen flag. OS layer has to check it after update and toggle fullscreen.
0.43 - removed mouse_screen from OS_State.
0.42 - added useful functions for Array: array_remove_range(), array_pop_().
0.41 - added random_range_v2().
0.40 - added rotate_point_around_pivot().
0.39 - completely reworked key state processing and implemented move_towards().
0.38 - added more functions to dynamic array.
0.37 - added templated String8 helper function get().
0.36 - added was_released().
0.35 - added round, clamp, ceil, for V2.
0.34 - Added section for helper functions and implemented ones for u32 flags.
0.33 - Expanded Button_State to have was_pressed() with duration_until_trigger_seconds.
0.32 - added frac().
0.31 - added random for v3 and random_nextf() for floats.
0.30 - added mouse_scroll.
0.29 - changed Rect size to half_size, added utils for Rect, and added overloads for hadamard_mul.
0.28 - added quaternion_identity().
0.27 - added min and max for V3s.
0.26 - added os->time, repeat(), ping_pong(), clamp for V3, bezier2() and other things.
0.25 - added PRNG.
0.24 - refined and tested generic hash table.
0.23 - implemented generic hash table, but still untested and incomplete.
0.22 - added generic dynamic arrays using templates. 
0.21 - added defer() and made many improvements to string formatting and arena APIs.
0.20 - renamed String8 functions and added sb_reset() for String_Builder.
0.19 - added more operator overloads for V2s and fixed mistake with some overloads.
0.18 - made small functions inline.
0.17 - added operator overloads for V2s.
0.16 - added write_entire_file in OS_State, prefixed String_Builder functions with sb_.
0.15 - added smooth_step() and smoother_step() for V2, V3, and V4.
0.14 - fixed bug in string_format_list()
0.13 - you can now specify angles in turns using sin_turns(), cos_turns(), and so on. Replaced to_radians() and to_degrees() with macros.
0.12 - added unproject().
0.11 - replaced some u32 indices with s32.
0.10 - fixed bug in String_Builder append().
0.09 - made mouse_screen and mouse_ndc V3 with .z = 0.
0.08 - expanded enum for keyboard keys.
0.07 - added orthographic_2d and orthographic_3d.
0.06 - added V3R, V3U and V3F.
0.05 - added V2s.
0.04 - re-structured functions to use result value to make debugging easier.
0.03 - added user input utilities and expanded OS_State, added CLAMP01_RANGE(), added print().
0.02 - added V2u, expanded OS_State, added aspect_ratio_fit().

CONVENTIONS:
* When storing paths, if string name has "folder" in it, then it ends with '/' or '\\'.
* Right-handed coordinate system. +Y is up.
* CCW winding order for front faces.
* Matrices are row-major with column vectors.
* First pixel pointed to is top-left-most pixel in image.
* UV-coords origin is at top-left corner (DOESN'T match with vertex coordinates).

TODO:
[] Null-terminate strings when using string_format_list()!! 
[] Establish convention to always write strings in files null-terminated.
[] Remove data_folder and use working_folder (Get/SetCurrentDirectory()) instead.
[] arenas never decommit memory. Find a good way to add that.

*/

#ifndef ORH_H
#define ORH_H

/////////////////////////////////////////
//
// Context cracking
//
// @Note: From Allen "mr4th" Webster.
#if defined(__clang__)
#    define COMPILER_CLANG 1
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#        define OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#        define OS_MAC 1
#    else
#        error missing OS detection
#    endif
#    if defined(__amd64__)
#        define ARCH_X64 1
#    elif defined(__i386__) // Verify this works on clang.
#        define ARCH_X86 1
#    elif defined(__arm__) // Verify this works on clang.
#        define ARCH_ARM 1
#    elif defined(__aarch64__) // Verify this works on clang.
#        define ARCH_ARM64 1
#    else
#        error missing ARCH detection
#    endif
#elif defined(_MSC_VER)
#    define COMPILER_CL 1
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    else
#        error missing OS detection
#    endif
#    if defined(_M_AMD64)
#        define ARCH_X64 1
#    elif defined(_M_I86)
#        define ARCH_X86 1
#    elif defined(_M_ARM)
#        define ARCH_ARM 1
#    else
#        error missing ARCH detection
#    endif
#elif defined(__GNUC__)
#    define COMPILER_GCC 1
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#        define OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#        define OS_MAC 1
#    else
#        error missing OS detection
#    endif
#    if defined(__amd64__)
#        define ARCH_X64 1
#    elif defined(__i386__)
#        define ARCH_X86 1
#    elif defined(__arm__)
#        define ARCH_ARM 1
#    elif defined(__aarch64__)
#        define ARCH_ARM64 1
#    else
#        error missing ARCH detection
#    endif
#else
#    error no context cracking for this compiler
#endif

#if !defined(COMPILER_CL)
#    define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
#    define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
#    define COMPILER_GCC 0
#endif
#if !defined(OS_WINDOWS)
#    define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#    define OS_LINUX 0
#endif
#if !defined(OS_MAC)
#    define OS_MAC 0
#endif
#if !defined(ARCH_X64)
#    define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#    define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
#    define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
#    define ARCH_ARM64 0
#endif

/////////////////////////////////////////
//
// Types
//
#ifndef ORH_NO_STD_MATH
#    include <math.h> 
#else
#    include <intrin.h>
#endif

#ifndef FUNCDEF
#    ifdef ORH_STATIC
#        define FUNCDEF static
#    else
#        define FUNCDEF extern
#    endif
#endif

#include <stddef.h>
#include <stdint.h>

typedef int8_t             s8;
typedef int16_t            s16;
typedef int32_t            s32;
typedef int32_t            b32;
typedef int64_t            s64;

typedef uint8_t            u8;
typedef uint16_t           u16;
typedef uint32_t           u32;
typedef uint64_t           u64;

typedef uintptr_t          umm;
typedef intptr_t           smm;

typedef float              f32;
typedef double             f64;

#define FUNCTION           static
#define LOCAL_PERSIST      static
#define GLOBAL             static

#define S8_MIN             (-S8_MAX  - 1)
#define S8_MAX             0x7F
#define S16_MIN            (-S16_MAX - 1)
#define S16_MAX            0x7FFF
#define S32_MIN            (-S32_MAX - 1)
#define S32_MAX            0x7FFFFFFF
#define S64_MIN            (-S64_MAX - 1)
#define S64_MAX            0x7FFFFFFFFFFFFFFFLL

// Un-signed minimum is zero.  
#define U8_MAX             0xFFU
#define U16_MAX            0xFFFFU
#define U32_MAX            0xFFFFFFFFU
#define U64_MAX            0xFFFFFFFFFFFFFFFFULL

#define F32_MIN            1.17549435e-38F
#define F32_MAX            3.40282347e+38F
#define F64_MIN            2.2250738585072014e-308
#define F64_MAX            1.7976931348623157e+308

#if COMPILER_CL
#    define threadvar __declspec(thread)
#elif COMPILER_CLANG
#    define threadvar __thread
#else
#    error threadvar not defined for this compiler
#endif

// Nesting macros in if-statements that have no curly-brackets causes issues. Using DOWHILE() avoids all of them.
#define DOWHILE(s)         do{s}while(0)
#define SWAP(a, b, Type)   DOWHILE(Type _t = a; a = b; b = _t;)

#if defined(_DEBUG)
#    if defined(COMPILER_CL)
#        define ASSERT(expr) DOWHILE(if (!(expr)) __debugbreak();)
#    else
#        define ASSERT(expr) DOWHILE(if (!(expr)) __builtin_trap();)
#    endif
#    if defined(OS_WINDOWS)
#        define ASSERT_HR(hr) ASSERT(SUCCEEDED(hr))
#    endif
#else
#    define ASSERT(expr)   (void)(expr)
#    define ASSERT_HR(expr)   (void)(expr)
#endif

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define STRINGIFY_(S) #S
#define STRINGIFY(s) STRINGIFY_(s)
#define GLUE_(a, b) a##b
#define GLUE(a, b) GLUE_(a, b)

#define STATIC_ASSERT(cond, name) typedef u8 GLUE(name,__LINE__) [(cond)?1:-1]

/////////////////////////////////////////
//
// Helper functions
//
FUNCTION inline void toggle(u32 *dst_flag, u32 src_flag)
{
    *dst_flag ^= src_flag;
}
FUNCTION inline void set(u32 *dst_flag, u32 src_flag)
{
    *dst_flag |= src_flag;
}
FUNCTION inline void clear(u32 *dst_flag, u32 src_flag)
{
    *dst_flag &= ~src_flag;
}
FUNCTION inline b32 is_set(u32 src_flag, u32 flag)
{
    return ((src_flag & flag) == flag);
}
FUNCTION inline b32 is_cleared(u32 src_flag, u32 flag)
{
    return ((src_flag & flag) == 0);
}

/////////////////////////////////////////
//
// Defer Scope
//
// @Note: From gingerBill and Ignacio Castano. Similar to defer() in Go but executes at end of scope.
template <typename F>
struct DeferScope 
{
    DeferScope(F f) : f(f) {}
    ~DeferScope() { f(); }
    F f;
};

template <typename F>
DeferScope<F> MakeDeferScope(F f) { return DeferScope<F>(f); };

#define defer(code) \
auto GLUE(__defer_, __COUNTER__) = MakeDeferScope([&](){code;})

/////////////////////////////////////////
//
// Math
//
#define PI    3.14159265358979323846
#define TAU   6.28318530717958647692
#define PI32  3.14159265359f
#define TAU32 6.28318530718f
#define RADS_TO_DEGS  (360.0f / TAU32)
#define RADS_TO_TURNS (1.0f   / TAU32)
#define DEGS_TO_RADS  (TAU32  / 360.0f)
#define DEGS_TO_TURNS (1.0f   / 360.0f)
#define TURNS_TO_RADS (TAU32  / 1.0f)
#define TURNS_TO_DEGS (360.0f / 1.0f)

#define MIN(A, B)              ((A < B) ? (A) : (B))
#define MAX(A, B)              ((A > B) ? (A) : (B))
#define MIN3(A, B, C)          MIN(MIN(A, B), C)
#define MAX3(A, B, C)          MAX(MAX(A, B), C)

#define CLAMP_UPPER(upper, x)      MIN(upper, x)
#define CLAMP_LOWER(x, lower)      MAX(x, lower)
#define CLAMP(lower, x, upper)    (CLAMP_LOWER(CLAMP_UPPER(upper, x), lower))
#define CLAMP01(x)                 CLAMP(0, x, 1)
#define CLAMP01_RANGE(min, x, max) CLAMP01(safe_div0(x-min, max-min))

#define SQUARE(x) ((x) * (x))
#define CUBE(x)   ((x) * (x) * (x))
#define ABS(x)    ((x) > 0 ? (x) : -(x))
#define SIGN(x)   ((x) > 0 ?  1  :  -1 )

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable:4201)
#endif
union V2
{
    struct { f32 x, y; };
    struct { f32 u, v; };
    f32 I[2];
};

union V2u
{
    struct { u32 x, y; };
    struct { u32 width, height; };
    u32 I[2];
};

union V2s
{
    struct { s32 x, y; };
    struct { s32 s, t; };
    s32 I[2];
};

union V3
{
    struct { f32 x, y, z; };
    struct { V2 xy; f32 ignored0_; };
    struct { f32 ignored1_; V2 yz; };
    struct { f32 r, g, b; };
    f32 I[3];
};

union V4
{
    struct 
    { 
        union
        {
            V3 xyz;
            struct { f32 x, y, z; };
        };
        f32 w;
    };
    struct { V2 xy, zw; };
    struct { f32 ignored0_; V2 yz; f32 ignored1_; };
    struct 
    { 
        union
        {
            V3 rgb;
            struct { f32 r, g, b; };
        };
        f32 a;
    };
    
    f32 I[4];
};

union Quaternion
{
    struct
    {
        union
        {
            V3 xyz, v;
            struct { f32 x, y, z; };
        };
        f32 w;
    };
    V4 xyzw;
    f32 I[4];
};

union M4x4
{
    // @Note: We use row-major with column vectors!
    f32 II [4][4];
    f32 I    [16];
    V4  row   [4];
    struct
    {
        f32 _11, _12, _13, _14;
        f32 _21, _22, _23, _24;
        f32 _31, _32, _33, _34;
        f32 _41, _42, _43, _44;
    };
};

struct M4x4_Inverse
{
    M4x4 forward;
    M4x4 inverse;
};

struct Range { f32 min, max; };
struct Rect2 { V2  min, max; };
struct Rect3 { V3  min, max; };
#ifdef _MSC_VER
#    pragma warning(pop)
#endif

FUNCDEF inline f32 _pow(f32 x, f32 y); // x^y
FUNCDEF inline f32 _mod(f32 x, f32 y); // x%y
FUNCDEF inline f32 _sqrt(f32 x);
FUNCDEF inline f32 _sin(f32 radians);
FUNCDEF inline f32 _cos(f32 radians);
FUNCDEF inline f32 _tan(f32 radians);
FUNCDEF inline f32 _sin_turns(f32 turns); // Takes angle in turns.
FUNCDEF inline f32 _cos_turns(f32 turns); // Takes angle in turns.
FUNCDEF inline f32 _tan_turns(f32 turns); // Takes angle in turns.
FUNCDEF inline f32 _arcsin(f32 x);
FUNCDEF inline f32 _arccos(f32 x);
FUNCDEF inline f32 _arctan(f32 x);
FUNCDEF inline f32 _arctan2(f32 y, f32 x);
FUNCDEF inline f32 _arcsin_turns(f32 x);          // Returns angle in turns.
FUNCDEF inline f32 _arccos_turns(f32 x);          // Returns angle in turns.
FUNCDEF inline f32 _arctan_turns(f32 x);          // Returns angle in turns.
FUNCDEF inline f32 _arctan2_turns(f32 y, f32 x);  // Returns angle in turns.
FUNCDEF inline f32 _round(f32 x); // Towards nearest integer.
FUNCDEF inline f32 _floor(f32 x); // Towards negative infinity.
FUNCDEF inline f32 _ceil(f32 x);  // Towards positive infinity.

FUNCDEF V2 clamp(V2 min, V2 x, V2 max);
FUNCDEF V2 round(V2 v);
FUNCDEF V2 floor(V2 v);
FUNCDEF V2 ceil(V2 v);

FUNCDEF V3 clamp(V3 min, V3 x, V3 max);
FUNCDEF V3 round(V3 v);
FUNCDEF V3 floor(V3 v);
FUNCDEF V3 ceil(V3 v);

FUNCDEF f32 frac(f32 x);
FUNCDEF V3  frac(V3 v);

FUNCDEF f32 safe_div(f32 x, f32 y, f32 n);
FUNCDEF f32 safe_div0(f32 x, f32 y);
FUNCDEF f32 safe_div1(f32 x, f32 y);

FUNCDEF V2 v2(f32 x, f32 y);
FUNCDEF V2 v2(f32 c);

FUNCDEF V2u v2u(u32 x, u32 y);
FUNCDEF V2u v2u(u32 c);

FUNCDEF V2s v2s(s32 x, s32 y);
FUNCDEF V2s v2s(s32 c);

FUNCDEF V3 v3(f32 x, f32 y, f32 z);
FUNCDEF V3 v3(f32 c);
FUNCDEF V3 v3(V2 xy, f32 z);

FUNCDEF V4 v4(f32 x, f32 y, f32 z, f32 w);
FUNCDEF V4 v4(f32 c);
FUNCDEF V4 v4(V3 xyz, f32 w);
FUNCDEF V4 v4(V2 xy, V2 zw);

FUNCDEF V2 hadamard_mul(V2 a, V2 b);
FUNCDEF V2 hadamard_div(V2 a, V2 b);
FUNCDEF V3 hadamard_mul(V3 a, V3 b);
FUNCDEF V3 hadamard_div(V3 a, V3 b);
FUNCDEF V4 hadamard_mul(V4 a, V4 b);
FUNCDEF V4 hadamard_div(V4 a, V4 b);

FUNCDEF f32 dot(V2 a, V2 b);
FUNCDEF s32 dot(V2s a, V2s b);
FUNCDEF f32 dot(V3 a, V3 b);
FUNCDEF f32 dot(V4 a, V4 b);

FUNCDEF V2  perp(V2 v); // Counter-Clockwise
FUNCDEF V3  cross(V3 a, V3 b);

FUNCDEF f32 length2(V2 v);
FUNCDEF f32 length2(V3 v);
FUNCDEF f32 length2(V4 v);

FUNCDEF f32 length(V2 v);
FUNCDEF f32 length(V3 v);
FUNCDEF f32 length(V4 v);

FUNCDEF V2 min_v2(V2 a, V2 b);
FUNCDEF V3 min_v3(V3 a, V3 b);
FUNCDEF V4 min_v4(V4 a, V4 b);

FUNCDEF V2 max_v2(V2 a, V2 b);
FUNCDEF V3 max_v3(V3 a, V3 b);
FUNCDEF V4 max_v4(V4 a, V4 b);

FUNCDEF V2 normalize(V2 v);
FUNCDEF V3 normalize(V3 v);
FUNCDEF V4 normalize(V4 v);

FUNCDEF V2 normalize0(V2 v);
FUNCDEF V3 normalize0(V3 v);
FUNCDEF V4 normalize0(V4 v);

FUNCDEF V3 reflect(V3 incident, V3 normal);

FUNCDEF f32        dot(Quaternion a, Quaternion b);
FUNCDEF f32        length(Quaternion q);
FUNCDEF Quaternion normalize(Quaternion q);

FUNCDEF inline Quaternion quaternion(f32 x, f32 y, f32 z, f32 w);
FUNCDEF inline Quaternion quaternion(V3 v, f32 w);
FUNCDEF inline Quaternion quaternion_identity();
FUNCDEF Quaternion quaternion_from_axis_angle(V3 axis, f32 angle); // Rotation around _axis_ by _angle_ radians.
FUNCDEF inline Quaternion quaternion_from_axis_angle_turns(V3 axis, f32 angle_turns); // Rotation around _axis_ by _angle_ turns.
FUNCDEF Quaternion quaternion_conjugate(Quaternion q);
FUNCDEF Quaternion quaternion_inverse(Quaternion q);
FUNCDEF V3         quaternion_get_axis(Quaternion q);
FUNCDEF f32        quaternion_get_angle(Quaternion q);
FUNCDEF inline f32 quaternion_get_angle_turns(Quaternion q);

FUNCDEF M4x4 m4x4_identity();
FUNCDEF M4x4 m4x4_from_quaternion(Quaternion q);
FUNCDEF M4x4 transpose(M4x4 m);
FUNCDEF V4   transform(M4x4 m, V4 v);
FUNCDEF inline V3   get_column(M4x4 m, u32 c);
FUNCDEF inline V3   get_row(M4x4 m, u32 r);
FUNCDEF inline V3   get_translation(M4x4 m);
FUNCDEF inline V3   get_scale(M4x4 m);
FUNCDEF inline M4x4 get_rotaion(M4x4 m, V3 scale);

FUNCDEF inline f32 lerp(f32 a, f32 t, f32 b);
FUNCDEF inline V2  lerp(V2 a, f32 t, V2 b);
FUNCDEF inline V3  lerp(V3 a, f32 t, V3 b);
FUNCDEF inline V4  lerp(V4 a, f32 t, V4 b);

FUNCDEF inline Quaternion  lerp(Quaternion a, f32 t, Quaternion b);
FUNCDEF inline Quaternion nlerp(Quaternion a, f32 t, Quaternion b);
FUNCDEF inline Quaternion slerp(Quaternion a, f32 t, Quaternion b);

FUNCDEF inline f32 smooth_step(f32 a, f32 t, f32 b);
FUNCDEF inline V2  smooth_step(V2 a, f32 t, V2 b);
FUNCDEF inline V3  smooth_step(V3 a, f32 t, V3 b);
FUNCDEF inline V4  smooth_step(V4 a, f32 t, V4 b);

FUNCDEF inline f32 smoother_step(f32 a, f32 t, f32 b);
FUNCDEF inline V2  smoother_step(V2 a, f32 t, V2 b);
FUNCDEF inline V3  smoother_step(V3 a, f32 t, V3 b);
FUNCDEF inline V4  smoother_step(V4 a, f32 t, V4 b);

FUNCDEF f32 move_towards(f32 current, f32 target, f32 max_distance);
FUNCDEF V2 move_towards(V2 current, V2 target, f32 max_distance);
FUNCDEF V3 move_towards(V3 current, V3 target, f32 max_distance);
FUNCDEF V2 rotate_point_around_pivot(V2 point, V2 pivot, Quaternion q);
FUNCDEF V3 rotate_point_around_pivot(V3 point, V3 pivot, Quaternion q);

FUNCDEF inline Range range(f32 min, f32 max);
FUNCDEF inline Rect2 rect2(V2  min, V2  max);
FUNCDEF inline Rect3 rect3(V3  min, V3  max);
FUNCDEF inline Rect2 rect2_from_center(V2 center, V2 half_size);
FUNCDEF inline Rect3 rect3_from_center(V3 center, V3 half_size);

FUNCDEF inline V2 get_size(Rect2 r);
FUNCDEF inline V2 get_center(Rect2 r);
FUNCDEF inline V3 get_size(Rect3 r);
FUNCDEF inline V3 get_center(Rect3 r);

FUNCDEF f32 repeat(f32 x, f32 max_y);
FUNCDEF f32 ping_pong(f32 x, f32 max_y);

FUNCDEF V3 bezier2(V3 p0, V3 p1, V3 p2, f32 t);

FUNCDEF M4x4_Inverse look_at(V3 pos, V3 at, V3 up);
FUNCDEF M4x4_Inverse perspective(f32 fov, f32 aspect, f32 n, f32 f);
FUNCDEF M4x4_Inverse infinite_perspective(f32 fov, f32 aspect, f32 n);
FUNCDEF M4x4_Inverse orthographic_3d(f32 left, f32 right, f32 bottom, f32 top, f32 n, f32 f, b32 normalized_z);
FUNCDEF M4x4_Inverse orthographic_2d(f32 left, f32 right, f32 bottom, f32 top);

FUNCDEF void calculate_tangents(V3 normal, V3 *tangent_out, V3 *bitangent_out);


//
// Operator overloading
//
#if (defined(__GCC__) || defined(__GNUC__)) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wattributes"
#    pragma GCC diagnostic ignored "-Wmissing-braces"
#elif __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wattributes"
#    pragma clang diagnostic ignored "-Wmissing-braces"
#endif

inline V2 operator+(V2 v)         
{ 
    V2 result = v;
    return result;
}
inline V2 operator-(V2 v)         
{ 
    V2 result = {-v.x, -v.y};
    return result;
}
inline V2 operator+(V2 a, V2 b)   
{ 
    V2 result = {a.x+b.x, a.y+b.y};
    return result;
}
inline V2 operator-(V2 a, V2 b)   
{ 
    V2 result = {a.x-b.x, a.y-b.y};
    return result;
}
inline V2 operator*(V2 v, f32 s)  
{ 
    V2 result = {v.x*s, v.y*s};
    return result;
}
inline V2 operator*(f32 s, V2 v)  
{ 
    V2 result = operator*(v, s);
    return result;
}
inline V2 operator*(V2 a, V2 b)  
{ 
    V2 result = hadamard_mul(a, b);
    return result;
}
inline V2 operator/(V2 v, f32 s)  
{ 
    V2 result = operator*(v, 1.0f/s);
    return result;
}
inline V2& operator+=(V2 &a, V2 b) 
{ 
    return (a = a + b);
}
inline V2& operator-=(V2 &a, V2 b) 
{ 
    return (a = a - b);
}
inline V2& operator*=(V2 &v, f32 s)
{ 
    return (v = v * s);
}
inline V2& operator/=(V2 &v, f32 s)
{ 
    return (v = v / s);
}

inline V2s operator+(V2s v)         
{ 
    V2s result = v;
    return result;
}
inline V2s operator-(V2s v)         
{ 
    V2s result = {-v.x, -v.y};
    return result;
}
inline V2s operator+(V2s a, V2s b)   
{ 
    V2s result = {a.x+b.x, a.y+b.y};
    return result;
}
inline V2s operator-(V2s a, V2s b)   
{ 
    V2s result = {a.x-b.x, a.y-b.y};
    return result;
}
inline V2s operator*(V2s v, s32 s)  
{ 
    V2s result = {v.x*s, v.y*s};
    return result;
}
inline V2s operator*(s32 s, V2s v)  
{ 
    V2s result = operator*(v, s);
    return result;
}
inline V2s& operator+=(V2s &a, V2s b) 
{ 
    return (a = a + b);
}
inline V2s& operator-=(V2s &a, V2s b) 
{ 
    return (a = a - b);
}
inline b32 operator==(V2s a, V2s b)
{
    b32 result = ((a.x == b.x) && (a.y == b.y));
    return result;
}
inline b32 operator!=(V2s a, V2s b)
{
    b32 result = !(a == b);
    return result;
}

inline V3 operator+(V3 v)         
{ 
    V3 result = v;
    return result;
}
inline V3 operator-(V3 v)         
{ 
    V3 result = {-v.x, -v.y, -v.z};
    return result;
}
inline V3 operator+(V3 a, V3 b)   
{ 
    V3 result = {a.x+b.x, a.y+b.y, a.z+b.z};
    return result;
}
inline V3 operator-(V3 a, V3 b)   
{ 
    V3 result = {a.x-b.x, a.y-b.y, a.z-b.z};
    return result;
}
inline V3 operator*(V3 v, f32 s)  
{ 
    V3 result = {v.x*s, v.y*s, v.z*s};
    return result;
}
inline V3 operator*(f32 s, V3 v)  
{ 
    V3 result = operator*(v, s);
    return result;
}
inline V3 operator*(V3 a, V3 b)  
{ 
    V3 result = hadamard_mul(a, b);
    return result;
}
inline V3 operator/(V3 v, f32 s)  
{ 
    V3 result = operator*(v, 1.0f/s);
    return result;
}
inline V3& operator+=(V3 &a, V3 b) 
{ 
    a = a + b;
    return a;
}
inline V3& operator-=(V3 &a, V3 b) 
{ 
    a = a - b;
    return a;
}
inline V3& operator*=(V3 &v, f32 s)
{ 
    v = v * s;
    return v;
}
inline V3& operator/=(V3 &v, f32 s)
{ 
    V3 result = (v = v / s);
    return result;
}

inline V4 operator+(V4 v)         
{ 
    V4 result = v;
    return result;
}
inline V4 operator-(V4 v)         
{ 
    V4 result = {-v.x, -v.y, -v.z, -v.w};
    return result;
}
inline V4 operator+(V4 a, V4 b)   
{ 
    V4 result = {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w};
    return result;
}
inline V4 operator-(V4 a, V4 b)   
{ 
    V4 result = {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w};
    return result;
}
inline V4 operator*(V4 v, f32 s)  
{ 
    V4 result = {v.x*s, v.y*s, v.z*s, v.w*s};
    return result;
}
inline V4 operator*(f32 s, V4 v)  
{ 
    V4 result = operator*(v, s);
    return result;
}
inline V4 operator/(V4 v, f32 s)  
{ 
    V4 result = operator*(v, 1.0f/s);
    return result;
}
inline V4& operator+=(V4 &a, V4 b) 
{ 
    V4 result = (a = a + b);
    return result;
}
inline V4& operator-=(V4 &a, V4 b) 
{ 
    V4 result = (a = a - b);
    return result;
}
inline V4& operator*=(V4 &v, f32 s)
{ 
    V4 result = (v = v * s);
    return result;
}
inline V4& operator/=(V4 &v, f32 s)
{ 
    V4 result = (v = v / s);
    return result;
}

inline Quaternion operator+(Quaternion v)                
{ 
    Quaternion result = v;
    return result;
}
inline Quaternion operator-(Quaternion v)                
{ 
    Quaternion result = {-v.x, -v.y, -v.z, -v.w};
    return result;
}
inline Quaternion operator+(Quaternion a, Quaternion b)  
{ 
    Quaternion result = {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w};
    return result;
}
inline Quaternion operator-(Quaternion a, Quaternion b)  
{ 
    Quaternion result = {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w};
    return result;
}
inline Quaternion operator*(Quaternion v, f32 s)         
{ 
    Quaternion result = {v.x*s, v.y*s, v.z*s, v.w*s};
    return result;
}
inline Quaternion operator*(f32 s, Quaternion v)         
{ 
    Quaternion result = operator*(v, s);
    return result;
}
inline Quaternion operator/(Quaternion v, f32 s)         
{ 
    Quaternion result = operator*(v, 1.0f/s);
    return result;
}
inline Quaternion& operator+=(Quaternion &a, Quaternion b)
{ 
    Quaternion result = (a = a + b);
    return result;
}
inline Quaternion& operator-=(Quaternion &a, Quaternion b)
{ 
    Quaternion result = (a = a - b);
    return result;
}
inline Quaternion& operator*=(Quaternion &v, f32 s)       
{ 
    Quaternion result = (v = v * s);
    return result;
}
inline Quaternion& operator/=(Quaternion &v, f32 s)       
{ 
    Quaternion result = (v = v / s);
    return result;
}
inline Quaternion operator*(Quaternion a, Quaternion b)
{ 
    Quaternion result = 
    {
        a.w*b.v + b.w*a.v + cross(a.v, b.v),
        a.w*b.w - dot(a.v, b.v)
    };
    return result; 
}
inline V3 operator*(Quaternion q, V3 v)
{
    // Rotating vector `v` by quaternion `q`:
    // Formula by the optimization expert Fabian "ryg" Giesen, check his blog:
    // https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/
    
    // Canonical formula for rotating v by q (slower):
    // result = q * quaternion(v, 0) * quaternion_conjugate(q);
    
    // Fabian's method (faster):
    V3 t      = 2 * cross(q.v, v);
    V3 result = v + q.w*t + cross(q.v, t);
    return result;
}

inline V3 operator*(M4x4 m, V3 v)
{ 
    V3 result = transform(m, v4(v, 1.0f)).xyz;
    return result;
}
inline V4 operator*(M4x4 m, V4 v)
{ 
    V4 result = transform(m, v);
    return result;
}
inline M4x4 operator*(M4x4 a, M4x4 b)
{
    M4x4 result = {};
    for(s32 r = 0; r < 4; r++)
    {for(s32 c = 0; c < 4; c++)
        {for(s32 i = 0; i < 4; i++)
            {result.II[r][c] += a.II[r][i] * b.II[i][c];}}}
    return result;
}

#if defined(__GCC__) || defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(__clang__)
#    pragma clang diagnostic pop
#endif

/////////////////////////////////////////
//
// PRNG
//
struct Random_PCG
{
    u64 state;
};
FUNCDEF inline Random_PCG random_seed(u64 seed = 78953890);
FUNCDEF inline u32 random_next(Random_PCG *rng);
FUNCDEF inline f32 random_nextf(Random_PCG *rng);         // [0, 1) interval
FUNCDEF inline u32 random_range(Random_PCG *rng, u32 min, u32 max);  // [min, max) interval.
FUNCDEF inline V2  random_range_v2(Random_PCG *rng, V2 min, V2 max); // [min, max) interval.
FUNCDEF inline V3  random_range_v3(Random_PCG *rng, V3 min, V3 max); // [min, max) interval.


/////////////////////////////////////////
//
// Memory Arena
//
#define KILOBYTES(x)   (         (x) * 1024LL)
#define MEGABYTES(x)   (KILOBYTES(x) * 1024LL)
#define GIGABYTES(x)   (MEGABYTES(x) * 1024LL)
#define TERABYTES(x)   (GIGABYTES(x) * 1024LL)

#define ALIGN_UP(x, pow2)   (((x) + ((pow2) - 1)) & ~((pow2) - 1))
#define ALIGN_DOWN(x, pow2) ( (x)                 & ~((pow2) - 1))

#define PUSH_STRUCT(arena, T)        ((T *) arena_push(arena, sizeof(T)))
#define PUSH_STRUCT_ZERO(arena, T)   ((T *) arena_push_zero(arena, sizeof(T)))
#define PUSH_ARRAY(arena, T, c)      ((T *) arena_push(arena, sizeof(T) * (c)))
#define PUSH_ARRAY_ZERO(arena, T, c) ((T *) arena_push_zero(arena, sizeof(T) * (c)))
#define MEMORY_ZERO(m, z)            memory_set(m, 0, z)
#define MEMORY_ZERO_STRUCT(s)        memory_set(s, 0, sizeof(*(s)))
#define MEMORY_ZERO_ARRAY(a)         memory_set(a, 0, sizeof(a))
#define MEMORY_COPY_STRUCT(d, s)     memory_copy(d, s, sizeof(*(d)));

#define ARENA_MAX_DEFAULT GIGABYTES(8)
#define ARENA_COMMIT_SIZE KILOBYTES(4)

#define ARENA_SCRATCH_COUNT 2

typedef struct Arena
{
    u64 max;
    u64 used;
    u64 commit_used;
} Arena;

typedef struct Arena_Temp
{
    Arena *arena;
    u64    used;
} Arena_Temp;

FUNCDEF Arena*     arena_init(u64 max_size = ARENA_MAX_DEFAULT);
FUNCDEF void       arena_free(Arena *arena);
FUNCDEF void*      arena_push(Arena *arena, u64 size);
FUNCDEF void*      arena_push_zero(Arena *arena, u64 size);
FUNCDEF void       arena_pop(Arena *arena, u64 size);
FUNCDEF void       arena_reset(Arena *arena);
FUNCDEF Arena_Temp arena_temp_begin(Arena *arena);
FUNCDEF void       arena_temp_end(Arena_Temp temp);
FUNCDEF Arena_Temp get_scratch(Arena **conflict_array, s32 count);
#define            free_scratch(temp) arena_temp_end(temp)

FUNCDEF void* memory_copy(void *dst, void *src, u64 size);
FUNCDEF void* memory_set(void *dst, s32 val, u64 size);

/////////////////////////////////////////
//
// String8
//
#define S8LIT(literal) string((u8 *) literal, ARRAY_COUNT(literal) - 1)
#define S8ZERO         string(0, 0)

// @Note: Allocation size is count+1 to account for null terminator.
struct String8
{
    u8 *data;
    u64 count;
    
    inline u8& operator[](s32 index)
    {
        ASSERT(index < count);
        return data[index];
    }
};

// @Note: Functions with Arena argument will allocate memory. If you don't pass an Arena to
//        these functions, it will default to allocating using os->permanent_arena.
FUNCDEF String8    string(u8 *data, u64 count);
FUNCDEF String8    string(const char *c_string);
FUNCDEF u64        string_length(const char *c_string);
FUNCDEF String8    string_copy(String8 s, Arena *arena = 0);
FUNCDEF String8    string_cat(String8 a, String8 b, Arena *arena = 0);
FUNCDEF inline b32 string_empty(String8 s);
FUNCDEF b32        string_match(String8 a, String8 b);

inline b32 operator==(String8 lhs, String8 rhs)
{
    return string_match(lhs, rhs);
}
inline b32 operator!=(String8 lhs, String8 rhs)
{
    return !(lhs == rhs);
}

/////////////////////////////////////////
//
// String Helper Functions
//
FUNCDEF inline void advance(String8 *s, u64 count);
FUNCDEF        u32  get_hash(String8 s);
FUNCDEF inline b32  is_spacing(char c);
FUNCDEF inline b32  is_end_of_line(char c);
FUNCDEF inline b32  is_whitespace(char c);
FUNCDEF inline b32  is_alpha(char c);
FUNCDEF inline b32  is_numeric(char c);
FUNCDEF inline b32  is_alphanumeric(char c);
FUNCDEF inline b32  is_file_separator(char c);
template<typename T>
void get(String8 *s, T *value)
{
    ASSERT(sizeof(T) <= s->count);
    
    memory_copy(value, s->data, sizeof(T));
    advance(s, sizeof(T));
}
void get(String8 *s, void *data, u64 size)
{
    ASSERT(size <= s->count);
    
    memory_copy(data, s->data, size);
    advance(s, size);
}

/////////////////////////////////////////
//
// String Format
//
#include <stdarg.h>

FUNCDEF void    put_char(String8 *dest, char c);
FUNCDEF void    put_c_string(String8 *dest, const char *c_string);
FUNCDEF void    u64_to_ascii(String8 *dest, u64 value, u32 base, char *digits);
FUNCDEF void    f64_to_ascii(String8 *dest, f64 value, u32 precision);
FUNCDEF u64     ascii_to_u64(char **at);
FUNCDEF u64     string_format_list(char *dest_start, u64 dest_count, const char *format, va_list arg_list);
FUNCDEF u64     string_format(char *dest_start, u64 dest_count, const char *format, ...);
FUNCDEF void    print(const char *format, ...);

// @Note: The sprint() function allocates memory; it is split into two versions: either
//        pass an arena explicitly, or automatically use the default os->permanent_arena.
FUNCDEF String8 sprint(const char *format, ...);
FUNCDEF String8 sprint(Arena *arena, const char *format, ...);

/////////////////////////////////////////
//
// String Builder
//
#define SB_BLOCK_SIZE KILOBYTES(4)

struct String_Builder
{
    Arena *arena;
    String8 buffer;
    
    u8 *start;
    u64 length;
    u64 capacity;
};

FUNCDEF String_Builder sb_init(u64 capacity = SB_BLOCK_SIZE);
FUNCDEF void           sb_free(String_Builder *builder);
FUNCDEF void           sb_reset(String_Builder *builder);
FUNCDEF void           sb_append(String_Builder *builder, void *data, u64 size);
FUNCDEF void           sb_appendf(String_Builder *builder, char *format, ...);
FUNCDEF String8        sb_to_string(String_Builder *builder, Arena *arena = 0);
template<typename T>
void sb_append(String_Builder *builder, T *data)
{
    sb_append(builder, data, sizeof(T));
}

/////////////////////////////////////////
//
// Dynamic Array
//
#define ARRAY_SIZE_MIN 32
template<typename T>
struct Array
{
    Arena *arena;
    T    *data;
    s64   count;
    s64   capacity;
    
    inline T& operator[](s32 index)
    {
        ASSERT((index >= 0) && (index < count));
        return data[index];
    }
};

template<typename T>
void array_init(Array<T> *array, s64 capacity = ARRAY_SIZE_MIN)
{
    array->arena    = arena_init();
    array->data     = 0;
    array->count    = 0;
    array_reserve(array, capacity);
}

template<typename T>
void array_free(Array<T> *array)
{
    arena_free(array->arena);
}

template<typename T>
void array_reserve(Array<T> *array, s64 desired_items)
{
    array->capacity = desired_items;
    
    arena_reset(array->arena);
    array->data = PUSH_ARRAY_ZERO(array->arena, T, desired_items);
}

template<typename T>
void array_copy(Array<T> *dst, Array<T> src)
{
    if (src.count > dst->capacity)
        array_reserve(dst, src.count);
    
    dst->count = src.count;
    memory_copy(dst->data, src.data, src.count * sizeof(T));
}

template<typename T>
void array_expand(Array<T> *array)
{
    s64 new_size = array->capacity * 2;
    if (new_size < ARRAY_SIZE_MIN) new_size = ARRAY_SIZE_MIN;
    
    Array<T> old_array;
    array_init(&old_array, array->count);
    defer(array_free(&old_array));
    array_copy(&old_array, *array);
    
    array_reserve(array, new_size);
    
    // Copy data.
    array_copy(array, old_array);
}

template<typename T>
void array_reset(Array<T> *array)
{
    array->count = 0;
}

template<typename T>
void array_add(Array<T> *array, T item)
{
    if (array->count >= array->capacity)
        array_expand(array);
    
    array->data[array->count] = item;
    array->count++;
}

template<typename T>
void array_remove_range(Array<T> *array, s32 start_index, s32 end_index)
{
    ASSERT((start_index >= 0) && (end_index < array->count) && (start_index < end_index));
    
    s32 num_elements = end_index - start_index + 1;
    for (s32 i = start_index; i < end_index+1; i++)
        array->data[i] = array->data[i+num_elements];
    array->count -= num_elements;
}

template<typename T>
void array_unordered_remove_by_index(Array<T> *array, s32 index)
{
    ASSERT((index >= 0) && (index < array->count));
    
    array->data[index] = array->data[array->count-1];
    array->count--;
}

template<typename T>
void array_ordered_remove_by_index(Array<T> *array, s32 index)
{
    ASSERT((index >= 0) && (index < array->count));
    
    for (s32 i = index; i < array->count-1; i++)
        array->data[i] = array->data[i+1];
    array->count--;
}

template<typename T>
T array_pop_front(Array<T> *array)
{
    if (array->count <= 0) {
        T dummy = {};
        return dummy;
    }
    
    T result = array->data[0];
    array_ordered_remove_by_index(array, 0);
    
    return result;
}

template<typename T>
T array_pop_back(Array<T> *array)
{
    if (array->count <= 0) {
        T dummy = {};
        return dummy;
    }
    
    T result = array->data[array->count-1];
    array->count--;
    
    return result;
}

template<typename T>
s32 array_find_index(Array<T> *array, T value)
{
    s32 result = -1;
    
    for (s32 index = 0; index < array->count; index++) {
        if (array->data[index] == value) {
            result = index;
            break;
        }
    }
    
    return result;
}

/////////////////////////////////////////
//
// Hash Table
//

// @Note: I don't like this... this header is needed to make generic hashing work.
//
#include <typeinfo> 

#define TABLE_SIZE_MIN 32
template<typename Key_Type, typename Value_Type>
struct Table
{
    s64 count;
    s64 capacity;
    
    Array<Key_Type>   keys;
    Array<Value_Type> values;
    Array<b32>        occupancy_mask;
    Array<u32>        hashes;
};

template<typename K, typename V>
void table_init(Table<K, V> *table, s64 size = 0)
{
    table->count    = 0;
    table->capacity = size;
    
    array_init(&table->keys,           size);
    array_init(&table->values,         size);
    array_init(&table->occupancy_mask, size);
    array_init(&table->hashes,         size);
}

template<typename K, typename V>
void table_free(Table<K, V> *table)
{
    array_free(&table->keys);
    array_free(&table->values);
    array_free(&table->occupancy_mask);
    array_free(&table->hashes);
}

template<typename K, typename V>
void table_reset(Table<K, V> *table)
{
    table->count = 0;
    MEMORY_ZERO(table->occupancy_mask.data, table->occupancy_mask.capacity * sizeof(table->occupancy_mask[0]));
}

template<typename K, typename V>
void table_expand(Table<K, V> *table)
{
    auto old_keys   = table->keys;
    auto old_values = table->values;
    auto old_mask   = table->occupancy_mask;
    auto old_hashes = table->hashes;
    
    s64 new_size    = table->capacity * 2;
    if (new_size < TABLE_SIZE_MIN) new_size = TABLE_SIZE_MIN;
    
    table_init(table, new_size);
    
    // Copy old data.
    for (s32 i = 0; i < old_mask.capacity; i++) {
        if (old_mask[i])
            table_add(table, old_keys[i], old_values[i]);
    }
    
    array_free(&old_keys);
    array_free(&old_values);
    array_free(&old_mask);
    array_free(&old_hashes);
}

template<typename K, typename V>
void table_add(Table<K, V> *table, K key, V value)
{
    if ((table->count * 2) >= table->capacity)
        table_expand(table);
    
    ASSERT(table->count <= table->capacity);
    
    // @Note: Some structs need padding; compiler will set padding bytes to arbitrary values. 
    // If we run into issues, we should manually add padding bytes for these types.
    //
    String8 s = {};
    if (typeid(K) == typeid(String8)) { s = *(String8*)&key; }
    else                              { s = string((u8*)&key, sizeof(K)); }
    
    u32 hash  = get_hash(s);
    s32 index = hash % table->capacity;
    
    // Linear probing.
    while (table->occupancy_mask[index]) {
        // @Note: Key_Types need to to have valid operator==().
        //
        if ((table->hashes[index] == hash) && (table->keys[index] == key)) {
            ASSERT(!"The passed key is already in use; Override is not allowed!");
        }
        
        index++;
        if (index >= table->capacity) index = 0;
    }
    
    table->count++;
    table->keys[index]           = key;
    table->values[index]         = value;
    table->occupancy_mask[index] = true;
    table->hashes[index]         = hash;
}

template<typename K, typename V>
V table_find(Table<K, V> *table, K key)
{
    if (!table->capacity) {
        V dummy = {};
        return dummy;
    }
    
    // @Note: Some structs need padding; compiler will set padding bytes to arbitrary values. 
    // If we run into issues, we should manually add padding bytes for these types.
    //
    String8 s = {};
    if (typeid(K) == typeid(String8)) { s = *(String8*)&key; }
    else                              { s = string((u8*)&key, sizeof(K)); }
    
    u32 hash  = get_hash(s);
    s32 index = hash % table->capacity;
    
    while (table->occupancy_mask[index]) {
        // @Note: Key_Types need to to have valid operator==().
        //
        if ((table->hashes[index] == hash) &&  (table->keys[index] == key)) {
            return table->values[index];
        }
        
        index++;
        if (index >= table->capacity) index = 0;
    }
    
    V dummy = {};
    return dummy;
}

/////////////////////////////////////////
//
// OS
//
enum Key
{
    Key_NONE,
    
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    
    Key_ENTER,
    Key_SHIFT,
    Key_CONTROL,
    Key_ALT,
    Key_ESCAPE,
    Key_SPACE,
    
    // Arrow keys.
    Key_LEFT,
    Key_UP,
    Key_RIGHT,
    Key_DOWN,
    
    // Mouse buttons.
    Key_MLEFT,
    Key_MRIGHT,
    Key_MMIDDLE,
    
    Key_COUNT,
};

struct Queued_Input
{
    s32 key;
    b32 down;
};

struct OS_State
{
    // Meta-data.
    String8 exe_full_path;
    String8 exe_parent_folder;
    String8 data_folder;
    
    // Arenas.
    Arena *permanent_arena; // Default arena.
    
    // User Input.
    Array<Queued_Input> inputs_to_process;
    b32 pressed [Key_COUNT];
    b32 held    [Key_COUNT];
    b32 released[Key_COUNT];
    V3  mouse_ndc;           // In [-1, 1] interval - relative to drawing_rect.
    V2  mouse_scroll;
    
    // Options.
    volatile b32 exit;
    b32 fullscreen;
    b32 vsync;
    b32 fix_aspect_ratio;
    V2u render_size;         // Determines aspect ratio.
    V2u window_size;         // Client window width and height.
    Rect2 drawing_rect;      // In pixel space - relative to client window.
    f32 dt;                  // The timestep - must be fixed!
    s32 fps_max;             // FPS limiter when vsync is off. Set to 0 for unlimited.
    f64 time;                // Incremented by dt at the end of each update.
    
    // Functions.
    void*   (*reserve) (u64 size);
    void    (*release) (void *memory);
    b32     (*commit)  (void *memory, u64 size);
    void    (*decommit)(void *memory, u64 size);
    void    (*print_to_console)(String8 text);
    void    (*free_file_memory)(void *memory);  // @Redundant: Does same thing as release().
    String8 (*read_entire_file)(String8 full_path);
    b32     (*write_entire_file)(String8 full_path, String8 data);
};
extern OS_State *os;

FUNCDEF b32 key_pressed(s32 key, b32 capture = false);
FUNCDEF b32 key_held(s32 key, b32 capture = false);
FUNCDEF b32 key_released(s32 key, b32 capture = false);
FUNCDEF Rect2 aspect_ratio_fit(V2u render_dim, V2u window_dim);
FUNCDEF V3    unproject(V3 camera_position, f32 Zworld_distance_from_camera, V3 mouse_ndc, M4x4_Inverse view, M4x4_Inverse proj);
#endif //ORH_H





#if defined(ORH_IMPLEMENTATION) && !defined(ORH_IMPLEMENTATION_LOCK)
#define ORH_IMPLEMENTATION_LOCK

/////////////////////////////////////////
//
// Math Implementation
//
#if (defined(__GCC__) || defined(__GNUC__)) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wattributes"
#    pragma GCC diagnostic ignored "-Wmissing-braces"
#elif __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wattributes"
#    pragma clang diagnostic ignored "-Wmissing-braces"
#endif

// @Todo: For now, <math.h> is included either way when using MSVC.
#ifdef ORH_NO_STD_MATH
#    ifdef _MSC_VER
#include <math.h>
f32 _pow(f32 x, f32 y)     
{
    f32 result = powf(x, y); 
    return result;
}
f32 _mod(f32 x, f32 y)     
{
    f32 result = fmodf(x, y); 
    return result;
}
f32 _sqrt(f32 x)           
{
    f32 result = sqrtf(x); 
    return result;
}
f32 _sin(f32 radians)      
{
    f32 result = sinf(radians); 
    return result;
}
f32 _cos(f32 radians)      
{
    f32 result = cosf(radians); 
    return result;
}
f32 _tan(f32 radians)      
{
    f32 result = tanf(radians); 
    return result;
}
f32 _sin_turns(f32 turns)
{
    return _sin(turns * TURNS_TO_RADS);
}
f32 _cos_turns(f32 turns)
{
    return _cos(turns * TURNS_TO_RADS);
}
f32 _tan_turns(f32 turns)
{
    return _tan(turns * TURNS_TO_RADS);
}
f32 _arcsin(f32 x)         
{
    f32 result = asinf(x); 
    return result;
}
f32 _arccos(f32 x)         
{
    f32 result = acosf(x); 
    return result;
}
f32 _arctan(f32 x)         
{
    f32 result = atanf(x); 
    return result;
}
f32 _arctan2(f32 y, f32 x) 
{
    f32 result = atan2f(y, x); 
    return result;
}
f32 _arcsin_turns(f32 x)
{
    return _arcsin(x) * RADS_TO_TURNS;
}
f32 _arccos_turns(f32 x)
{
    return _arccos(x) * RADS_TO_TURNS;
}
f32 _arctan_turns(f32 x)
{
    return _arctan(x) * RADS_TO_TURNS;
}
f32 _arctan2_turns(f32 y, f32 x);
{
    return _arctan2(y, x) * RADS_TO_TURNS;
}
f32 _round(f32 x)          
{
    f32 result = roundf(x); 
    return result;
}
f32 _floor(f32 x)          
{
    f32 result = floorf(x); 
    return result;
}
f32 _ceil(f32 x)           
{
    f32 result = ceilf(x); 
    return result;
}
#    else
f32 _pow(f32 x, f32 y)     
{
    f32 result = __builtin_powf(x, y); 
    return result;
}
f32 _mod(f32 x, f32 y)     
{
    f32 result = __builtin_fmodf(x, y); 
    return result;
}
f32 _sqrt(f32 x)           
{
    f32 result = __builtin_sqrtf(x); 
    return result;
}
f32 _sin(f32 radians)      
{
    f32 result = __builtin_sinf(radians); 
    return result;
}
f32 _cos(f32 radians)      
{
    f32 result = __builtin_cosf(radians); 
    return result;
}
f32 _tan(f32 radians)      
{
    f32 result = __builtin_tanf(radians); 
    return result;
}
f32 _sin_turns(f32 turns)
{
    return _sin(turns * TURNS_TO_RADS);
}
f32 _cos_turns(f32 turns)
{
    return _cos(turns * TURNS_TO_RADS);
}
f32 _tan_turns(f32 turns)
{
    return _tan(turns * TURNS_TO_RADS);
}
f32 _arcsin(f32 x)         
{
    f32 result = __builtin_asinf(x); 
    return result;
}
f32 _arccos(f32 x)         
{
    f32 result = __builtin_acosf(x); 
    return result;
}
f32 _arctan(f32 x)
{
    f32 result = __builtin_atanf(x); 
    return result;
}
f32 _arctan2(f32 y, f32 x)
{
    f32 result = __builtin_atan2f(y, x); 
    return result;
}
f32 _arcsin_turns(f32 x)
{
    return _arcsin(x) * RADS_TO_TURNS;
}
f32 _arccos_turns(f32 x)
{
    return _arccos(x) * RADS_TO_TURNS;
}
f32 _arctan_turns(f32 x)
{
    return _arctan(x) * RADS_TO_TURNS;
}
f32 _arctan2_turns(f32 y, f32 x)
{
    return _arctan2(y, x) * RADS_TO_TURNS;
}
f32 _round(f32 x)          
{
    f32 result = __builtin_roundf(x); 
    return result;
}
f32 _floor(f32 x)          
{
    f32 result = __builtin_floorf(x); 
    return result;
}
f32 _ceil(f32 x)           
{
    f32 result = __builtin_ceilf(x); 
    return result;
}
#    endif
#else
f32 _pow(f32 x, f32 y)     
{
    f32 result = powf(x, y); 
    return result;
}
f32 _mod(f32 x, f32 y)     
{
    f32 result = fmodf(x, y); 
    return result;
}
f32 _sqrt(f32 x)           
{
    f32 result = sqrtf(x); 
    return result;
}
f32 _sin(f32 radians)      
{
    f32 result = sinf(radians); 
    return result;
}
f32 _cos(f32 radians)      
{
    f32 result = cosf(radians); 
    return result;
}
f32 _tan(f32 radians)      
{
    f32 result = tanf(radians); 
    return result;
}
f32 _sin_turns(f32 turns)
{
    return _sin(turns * TURNS_TO_RADS);
}
f32 _cos_turns(f32 turns)
{
    return _cos(turns * TURNS_TO_RADS);
}
f32 _tan_turns(f32 turns)
{
    return _tan(turns * TURNS_TO_RADS);
}
f32 _arcsin(f32 x)         
{
    f32 result = asinf(x); 
    return result;
}
f32 _arccos(f32 x)         
{
    f32 result = acosf(x); 
    return result;
}
f32 _arctan(f32 x)
{
    f32 result = atanf(x); 
    return result;
}
f32 _arctan2(f32 y, f32 x)
{
    f32 result = atan2f(y, x); 
    return result;
}
f32 _arcsin_turns(f32 x)
{
    return _arcsin(x) * RADS_TO_TURNS;
}
f32 _arccos_turns(f32 x)
{
    return _arccos(x) * RADS_TO_TURNS;
}
f32 _arctan_turns(f32 x)
{
    return _arctan(x) * RADS_TO_TURNS;
}
f32 _arctan2_turns(f32 y, f32 x)
{
    return _arctan2(y, x) * RADS_TO_TURNS;
}
f32 _round(f32 x)          
{
    f32 result = roundf(x); 
    return result;
}
f32 _floor(f32 x)          
{
    f32 result = floorf(x); 
    return result;
}
f32 _ceil(f32 x)           
{
    f32 result = ceilf(x); 
    return result;
}
#endif

V2 clamp(V2 min, V2 x, V2 max)
{
    V2 result = {};
    result.x = CLAMP(min.x, x.x, max.x);
    result.y = CLAMP(min.y, x.y, max.y);
    return result;
}
V2 round(V2 v)
{
    V2 result = {};
    result.x  = _round(v.x);
    result.y  = _round(v.y);
    return result;
}
V2 floor(V2 v)
{
    V2 result = {};
    result.x  = _floor(v.x);
    result.y  = _floor(v.y);
    return result;
}
V2 ceil(V2 v)
{
    V2 result = {};
    result.x  = _ceil(v.x);
    result.y  = _ceil(v.y);
    return result;
}

V3 clamp(V3 min, V3 x, V3 max)
{
    V3 result = {};
    result.x = CLAMP(min.x, x.x, max.x);
    result.y = CLAMP(min.y, x.y, max.y);
    result.z = CLAMP(min.z, x.z, max.z);
    return result;
}
V3 round(V3 v)
{
    V3 result = {};
    result.x  = _round(v.x);
    result.y  = _round(v.y);
    result.z  = _round(v.z);
    return result;
}
V3 floor(V3 v)
{
    V3 result = {};
    result.x  = _floor(v.x);
    result.y  = _floor(v.y);
    result.z  = _floor(v.z);
    return result;
}
V3 ceil(V3 v)
{
    V3 result = {};
    result.x  = _ceil(v.x);
    result.y  = _ceil(v.y);
    result.z  = _ceil(v.z);
    return result;
}

f32 frac(f32 x)
{
    s32 i = (s32) x;
    
    f32 result = x - (f32)i;
    return result;
}
V3 frac(V3 v)
{
    V3 result = {};
    result.x  = frac(v.x);
    result.y  = frac(v.y);
    result.z  = frac(v.z);
    return result;
}

f32 safe_div(f32 x, f32 y, f32 n) 
{
    f32 result = y == 0.0f ? n : x/y; 
    return result;
}
f32 safe_div0(f32 x, f32 y)       
{
    f32 result = safe_div(x, y, 0.0f); 
    return result;
}
f32 safe_div1(f32 x, f32 y)       
{
    f32 result = safe_div(x, y, 1.0f); 
    return result;
}

V2 v2(f32 x, f32 y)
{
    V2 v = {x, y}; 
    return v; 
}
V2 v2(f32 c)
{
    V2 v = {c, c}; 
    return v; 
}

V2u v2u(u32 x, u32 y)
{
    V2u v = {x, y}; 
    return v; 
}
V2u v2u(u32 c)
{
    V2u v = {c, c}; 
    return v; 
}

V2s v2s(s32 x, s32 y)
{
    V2s v = {x, y}; 
    return v; 
}
V2s v2s(s32 c)
{
    V2s v = {c, c}; 
    return v; 
}


V3 v3(f32 x, f32 y, f32 z)
{
    V3 v = {x, y, z}; 
    return v; 
}
V3 v3(f32 c)
{
    V3 v = {c, c, c}; 
    return v; 
}
V3 v3(V2 xy, f32 z)
{
    V3 v; v.xy = xy; v.z = z; 
    return v; 
}

V4 v4(f32 x, f32 y, f32 z, f32 w)
{
    V4 v = {x, y, z, w}; 
    return v; 
}
V4 v4(f32 c)
{
    V4 v = {c, c, c, c}; 
    return v; 
}
V4 v4(V3 xyz, f32 w)
{
    V4 v; v.xyz = xyz; v.w = w; 
    return v; 
}
V4 v4(V2 xy, V2 zw)
{
    V4 v; v.xy = xy; v.zw = zw;
    return v;
}

V2 hadamard_mul(V2 a, V2 b)
{
    V2 v = {a.x*b.x, a.y*b.y}; 
    return v; 
}
V2 hadamard_div(V2 a, V2 b)
{
    V2 v = {a.x/b.x, a.y/b.y}; 
    return v; 
}
V3 hadamard_mul(V3 a, V3 b)
{
    V3 v = {a.x*b.x, a.y*b.y, a.z*b.z}; 
    return v; 
}
V3 hadamard_div(V3 a, V3 b)
{
    V3 v = {a.x/b.x, a.y/b.y, a.z/b.z}; 
    return v; 
}
V4 hadamard_mul(V4 a, V4 b)
{
    V4 v = {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w}; 
    return v; 
}
V4 hadamard_div(V4 a, V4 b)
{
    V4 v = {a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w}; 
    return v; 
}


f32 dot(V2 a, V2 b) 
{ 
    f32 result = a.x*b.x + a.y*b.y;
    return result;
}
s32 dot(V2s a, V2s b) 
{ 
    s32 result = a.x*b.x + a.y*b.y; 
    return result;
}
f32 dot(V3 a, V3 b) 
{ 
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z; 
    return result;
}
f32 dot(V4 a, V4 b) 
{ 
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; 
    return result;
}

V2 perp(V2 v)
{
    // Counter-Clockwise.
    
    V2 result = {-v.y, v.x};
    return result;
}
V3 cross(V3 a, V3 b) 
{
    V3 result =
    {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
    return result;
}

f32 length2(V2 v) 
{
    f32 result = dot(v, v); 
    return result;
}
f32 length2(V3 v) 
{
    f32 result = dot(v, v); 
    return result;
}
f32 length2(V4 v) 
{
    f32 result = dot(v, v); 
    return result;
}

f32 length(V2 v) 
{
    f32 result = _sqrt(length2(v)); 
    return result;
}
f32 length(V3 v) 
{
    f32 result = _sqrt(length2(v)); 
    return result;
}
f32 length(V4 v) 
{
    f32 result = _sqrt(length2(v)); 
    return result;
}

V2 min_v2(V2 a, V2 b)
{
    V2 result = 
    {
        MIN(a.x, b.x),
        MIN(a.y, b.y),
    };
    return result;
}
V3 min_v3(V3 a, V3 b)
{
    V3 result = 
    {
        MIN(a.x, b.x),
        MIN(a.y, b.y),
        MIN(a.z, b.z),
    };
    return result;
}
V4 min_v4(V4 a, V4 b)
{
    V4 result = 
    {
        MIN(a.x, b.x),
        MIN(a.y, b.y),
        MIN(a.z, b.z),
        MIN(a.w, b.w),
    };
    return result;
}

V2 max_v2(V2 a, V2 b)
{
    V2 result = 
    {
        MAX(a.x, b.x),
        MAX(a.y, b.y),
    };
    return result;
}
V3 max_v3(V3 a, V3 b)
{
    V3 result = 
    {
        MAX(a.x, b.x),
        MAX(a.y, b.y),
        MAX(a.z, b.z),
    };
    return result;
}
V4 max_v4(V4 a, V4 b)
{
    V4 result = 
    {
        MAX(a.x, b.x),
        MAX(a.y, b.y),
        MAX(a.z, b.z),
        MAX(a.w, b.w),
    };
    return result;
}

V2 normalize(V2 v) 
{
    V2 result = (v * (1.0f / length(v))); 
    return result;
}
V3 normalize(V3 v) 
{
    V3 result = (v * (1.0f / length(v))); 
    return result;
}
V4 normalize(V4 v) 
{
    V4 result = (v * (1.0f / length(v))); 
    return result;
}

V2 normalize0(V2 v) 
{ 
    f32 len = length(v); 
    return len > 0 ? v/len : v2(0.0f); 
}
V3 normalize0(V3 v) 
{ 
    f32 len = length(v); 
    return len > 0 ? v/len : v3(0.0f); 
}
V4 normalize0(V4 v) 
{ 
    f32 len = length(v);
    return len > 0 ? v/len : v4(0.0f); 
}

V3 reflect(V3 incident, V3 normal)
{
    V3 result = incident - 2.0f * dot(incident, normal) * normal;
    return result;
}

f32 dot(Quaternion a, Quaternion b) 
{
    f32 result = dot(a.xyz, b.xyz) + a.w*b.w; 
    return result;
}
f32 length(Quaternion q)            
{
    f32 result = _sqrt(dot(q, q)); 
    return result;
}
Quaternion normalize(Quaternion q)  
{
    Quaternion result = (q * (1.0f / length(q))); 
    return result;
}

Quaternion quaternion(f32 x, f32 y, f32 z, f32 w) 
{ 
    Quaternion q = {x, y, z, w}; 
    return q; 
}
Quaternion quaternion(V3 v, f32 w)
{ 
    Quaternion q; q.xyz = v; q.w = w; 
    return q; 
}
Quaternion quaternion_identity()
{
    Quaternion result = {0.0f, 0.0f, 0.0f, 1.0f};
    return result;
}
Quaternion quaternion_from_axis_angle(V3 axis, f32 angle)
{
    // @Note: Rotation around _axis_ by _angle_ radians.
    
    Quaternion result;
    result.v = axis * _sin(angle*0.5f);
    result.w =        _cos(angle*0.5f);
    return result;
}
Quaternion quaternion_from_axis_angle_turns(V3 axis, f32 angle_turns)
{
    Quaternion result = quaternion_from_axis_angle(axis, angle_turns * TURNS_TO_RADS);
    return result;
}
Quaternion quaternion_conjugate(Quaternion q) 
{
    Quaternion result;
    result.v = -q.v;
    result.w =  q.w;
    return result;
}
Quaternion quaternion_inverse(Quaternion q) 
{
    Quaternion result = quaternion_conjugate(q)/dot(q, q);
    return result;
}
V3 quaternion_get_axis(Quaternion q)
{
    Quaternion n = normalize(q);
    V3 result    = n.v / _sin(_arccos(q.w));
    return result;
}
f32 quaternion_get_angle(Quaternion q)
{
    f32 w      = q.w  / length(q);
    f32 result = 2.0f * _arccos(w);
    return result;
}
f32 quaternion_get_angle_turns(Quaternion q)
{
    f32 result = quaternion_get_angle(q) * RADS_TO_TURNS;
    return result;
}

M4x4 m4x4_identity() 
{
    M4x4 result = {};
    result._11 = 1.0f;
    result._22 = 1.0f;
    result._33 = 1.0f;
    result._44 = 1.0f;
    return result;
}
M4x4 m4x4_from_quaternion(Quaternion q)
{
    // Taken from:
    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
    
    /*     
        1 - 2*qy2 - 2*qz2	2*qx*qy - 2*qz*qw	2*qx*qz + 2*qy*qw;
        2*qx*qy + 2*qz*qw	1 - 2*qx2 - 2*qz2	2*qy*qz - 2*qx*qw;
        2*qx*qz - 2*qy*qw	2*qy*qz + 2*qx*qw	1 - 2*qx2 - 2*qy2;
         */
    
    M4x4 result = m4x4_identity();
    
    q = normalize(q);
    f32 qyy = q.y * q.y; f32 qzz = q.z * q.z; f32 qxy = q.x * q.y; 
    f32 qxz = q.x * q.z; f32 qyw = q.y * q.w; f32 qzw = q.z * q.w; 
    f32 qxx = q.x * q.x; f32 qyz = q.y * q.z; f32 qxw = q.x * q.w; 
    
    result._11 = 1.0f - 2.0f * (qyy + qzz);
    result._21 =        2.0f * (qxy + qzw);
    result._31 =        2.0f * (qxz - qyw);
    
    result._12 =        2.0f * (qxy - qzw);
    result._22 = 1.0f - 2.0f * (qxx + qzz);
    result._32 =        2.0f * (qyz + qxw);
    
    result._13 =        2.0f * (qxz + qyw);
    result._23 =        2.0f * (qyz - qxw);
    result._33 = 1.0f - 2.0f * (qxx + qyy);
    
    return result;
}
M4x4 transpose(M4x4 m)
{
    M4x4 result = 
    {
        m._11, m._21, m._31, m._41,
        m._12, m._22, m._32, m._42,
        m._13, m._23, m._33, m._43,
        m._14, m._24, m._34, m._44
    };
    return result;
}
V4 transform(M4x4 m, V4 v)
{
    V4 result = 
    {
        m._11*v.x + m._12*v.y + m._13*v.z + m._14*v.w,
        m._21*v.x + m._22*v.y + m._23*v.z + m._24*v.w,
        m._31*v.x + m._32*v.y + m._33*v.z + m._34*v.w,
        m._41*v.x + m._42*v.y + m._43*v.z + m._44*v.w
    };
    return result;
}
V3 get_column(M4x4 m, u32 c) 
{
    V3 result = {m.II[0][c], m.II[1][c], m.II[2][c]};
    return result;
}
V3 get_row(M4x4 m, u32 r)
{
    V3 result = {m.II[r][0], m.II[r][1], m.II[r][2]};
    return result;
}
V3 get_translation(M4x4 m) 
{
    V3 result = get_column(m, 3); 
    return result;
}
V3 get_scale(M4x4 m)
{
    f32 sx = length(get_column(m, 0));
    f32 sy = length(get_column(m, 1));
    f32 sz = length(get_column(m, 2));
    return {sx, sy, sz};
}
M4x4 get_rotaion(M4x4 m, V3 scale)
{
    V3 c0 = get_column(m, 0) / scale.x;
    V3 c1 = get_column(m, 1) / scale.y;
    V3 c2 = get_column(m, 2) / scale.z;
    
    M4x4 result = 
    {
        c0.x, c1.x, c2.x, 0.0f,
        c0.y, c1.y, c2.y, 0.0f,
        c0.z, c1.z, c2.z, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    return result;
}

f32 lerp(f32 a, f32 t, f32 b) 
{
    f32 result = a*(1.0f - t) + b*t; 
    return result;
}
V2 lerp(V2 a, f32 t, V2 b) 
{
    V2 result = a*(1.0f - t) + b*t; 
    return result;
}
V3 lerp(V3 a, f32 t, V3 b) 
{
    V3 result = a*(1.0f - t) + b*t; 
    return result;
}
V4 lerp(V4 a, f32 t, V4 b) 
{
    V4 result = a*(1.0f - t) + b*t; 
    return result;
}

Quaternion lerp(Quaternion a, f32 t, Quaternion b)
{
    Quaternion q;
    q.xyzw = lerp(a.xyzw, t, b.xyzw);
    return q;
}
Quaternion nlerp(Quaternion a, f32 t, Quaternion b)
{
    return normalize(lerp(a, t, b));
}
Quaternion slerp(Quaternion a, f32 t, Quaternion b)
{
    // Quaternion slerp according to wiki:
    // result = (b * q0^-1)^t * q0
    
    // Geometric slerp according to wiki, and help from glm to resolve edge cases:
    Quaternion result;
    
    f32 cos_theta = dot(a, b);
    if (cos_theta < 0.0f) {
        b         = -b;
        cos_theta = -cos_theta;
    }
    
    // When cos_theta is 1.0f, sin_theta is 0.0f. 
    // Avoid the slerp formula when we get close to 1.0f and do a lerp() instead.
    if (cos_theta > 1.0f - 1.192092896e-07F) {
        result = lerp(a, t, b);
    } else {
        f32 theta = _arccos(cos_theta);
        result    = (_sin((1-t)*theta)*a + _sin(t*theta)*b) / _sin(theta);
    }
    
    return result;
}

f32 smooth_step(f32 a, f32 t, f32 b) 
{ 
    float x = CLAMP01((t - a) / (b - a));
    return SQUARE(x)*(3.0f - 2.0f*x);
}
V2 smooth_step(V2 a, f32 t, V2 b)
{
    V2 result = {};
    result.x  = smooth_step(a.x, t, b.x);
    result.y  = smooth_step(a.y, t, b.y);
    return result;
}
V3 smooth_step(V3 a, f32 t, V3 b)
{
    V3 result = {};
    result.xy = smooth_step(a.xy, t, b.xy);
    result.z  = smooth_step(a.z, t, b.z);
    return result;
}
V4 smooth_step(V4 a, f32 t, V4 b)
{
    V4 result  = {};
    result.xyz = smooth_step(a.xyz, t, b.xyz);
    result.w   = smooth_step(a.w, t, b.w);
    return result;
}

f32 smoother_step(f32 a, f32 t, f32 b) 
{
    float x = CLAMP01((t - a) / (b - a));
    return CUBE(x)*(x*(6.0f*x - 15.0f) + 10.0f);
}
V2 smoother_step(V2 a, f32 t, V2 b)
{
    V2 result = {};
    result.x  = smoother_step(a.x, t, b.x);
    result.y  = smoother_step(a.y, t, b.y);
    return result;
}
V3 smoother_step(V3 a, f32 t, V3 b)
{
    V3 result = {};
    result.xy = smoother_step(a.xy, t, b.xy);
    result.z  = smoother_step(a.z, t, b.z);
    return result;
}
V4 smoother_step(V4 a, f32 t, V4 b)
{
    V4 result  = {};
    result.xyz = smoother_step(a.xyz, t, b.xyz);
    result.w   = smoother_step(a.w, t, b.w);
    return result;
}

f32 move_towards(f32 current, f32 target, f32 max_distance)
{
    f32 delta = target - current;
    if (ABS(delta) <= max_distance)
        return target;
    
    return current + SIGN(delta) * max_distance;
}
V2 move_towards(V2 current, V2 target, f32 max_distance)
{
    V2 delta = target - current;
    f32 distance2 = length2(delta);
    
    if (distance2 <= SQUARE(max_distance)) 
        return target;
    
    f32 ratio = max_distance / _sqrt(distance2);
    return current + (delta * ratio);
}
V3 move_towards(V3 current, V3 target, f32 max_distance)
{
    V3 delta = target - current;
    f32 distance2 = length2(delta);
    
    if (distance2 <= SQUARE(max_distance)) 
        return target;
    
    f32 ratio = max_distance / _sqrt(distance2);
    return current + (delta * ratio);
}
V2 rotate_point_around_pivot(V2 point, V2 pivot, Quaternion q)
{
    V2 result = rotate_point_around_pivot(v3(point, 0), v3(pivot, 0), q).xy;
    return result;
}
V3 rotate_point_around_pivot(V3 point, V3 pivot, Quaternion q)
{
    V3 result = (q * (point - pivot)) + pivot;
    return result;
}

Range range(f32 min, f32 max) 
{ 
    Range result = {min, max}; 
    return result; 
}
Rect2 rect2(V2  min, V2  max) 
{ 
    Rect2 result = {min, max}; 
    return result; 
}
Rect3 rect3(V3  min, V3  max)
{ 
    Rect3 result = {min, max}; 
    return result; 
}
Rect2 rect2_from_center(V2 center, V2 half_size)
{
    Rect2 result = { center - half_size, center + half_size };
    return result;
}
Rect3 rect3_from_center(V3 center, V3 half_size)
{
    Rect3 result = { center - half_size, center + half_size };
    return result;
}

V2 get_size(Rect2 r)
{
    V2 result = r.max - r.min;
    return result;
}
V2 get_center(Rect2 r)
{
    V2 size   = r.max - r.min;
    V2 result = r.min + (size * 0.5f);
    return result;
}
V3 get_size(Rect3 r)
{
    V3 result = r.max - r.min;
    return result;
}
V3 get_center(Rect3 r)
{
    V3 size   = r.max - r.min;
    V3 result = r.min + (size * 0.5f);
    return result;
}

//
// @Note: From Unity!
//
f32 repeat(f32 x, f32 max_y)
{
    // Loops the returned value (y a.k.a. result), so that it is never larger than max_y and never smaller than 0.
    //
    //    y
    //    |   /  /  /  /
    //    |  /  /  /  /
    //   x|_/__/__/__/_
    //
    f32 result = CLAMP(0.0f, x - _floor(x / max_y) * max_y, max_y);
    return result;
}
f32 ping_pong(f32 x, f32 max_y)
{
    // PingPongs the returned value (y a.k.a. result), so that it is never larger than max_y and never smaller than 0.
    //
    //   y
    //   |   /\    /\    /\
    //   |  /  \  /  \  /  \
    //  x|_/____\/____\/____\_
    //
    x     = repeat(x, max_y * 2.0f);
    f32 result = max_y - ABS(x - max_y);
    return result;
}

V3 bezier2(V3 p0, V3 p1, V3 p2, f32 t)
{
    V3 result = lerp(lerp(p0, t, p1), t, lerp(p1, t, p2));
    return result;
}

M4x4_Inverse look_at(V3 pos, V3 at, V3 up)
{
    V3 zaxis  = normalize(at - pos);
    V3 xaxis  = normalize(cross(zaxis, up));
    V3 yaxis  = cross(xaxis, zaxis);
    V3 t      = 
    {
        -dot(xaxis, pos), 
        -dot(yaxis, pos), 
        dot(zaxis, pos)
    };
    
    V3 xaxisI =  xaxis/length2(xaxis);
    V3 yaxisI =  yaxis/length2(yaxis);
    V3 zaxisI = -zaxis/length2(-zaxis);
    V3 tI     = 
    {
        xaxisI.x*t.x + yaxisI.x*t.y + -zaxis.x*t.z,
        xaxisI.y*t.x + yaxisI.y*t.y + -zaxis.y*t.z,
        xaxisI.z*t.x + yaxisI.z*t.y + -zaxis.z*t.z
    };
    
    M4x4_Inverse result =
    {
        {{ // The transform itself.
                { xaxis.x,  xaxis.y,  xaxis.z,  t.x},
                { yaxis.x,  yaxis.y,  yaxis.z,  t.y},
                {-zaxis.x, -zaxis.y, -zaxis.z,  t.z},
                {    0.0f,     0.0f,     0.0f, 1.0f}
            }},
        
        {{ // The inverse.
                { xaxisI.x,  yaxisI.x, zaxisI.x,  -tI.x},
                { xaxisI.y,  yaxisI.y, zaxisI.y,  -tI.y},
                { xaxisI.z,  yaxisI.z, zaxisI.z,  -tI.z},
                {        0,         0,        0,      1}
            }}
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse perspective(f32 fov, f32 aspect, f32 n, f32 f)
{
    ASSERT(aspect != 0);
    
    f32 t = _tan(fov * 0.5f);
    f32 x = 1.0f / (aspect * t);
    f32 y = 1.0f / t;
    f32 z = -(f + n) / (f - n);
    f32 e = -(2.0f * f * n) / (f - n);
    
    M4x4_Inverse result =
    {
        {{ // The transform itself.
                {   x, 0.0f,  0.0f, 0.0f},
                {0.0f,    y,  0.0f, 0.0f},
                {0.0f, 0.0f,     z,    e},
                {0.0f, 0.0f, -1.0f, 0.0f},
            }},
        
        {{ // Its inverse.
                {1.0f/x,   0.0f,    0.0f,  0.0f},
                {  0.0f, 1.0f/y,    0.0f,  0.0f},
                {  0.0f,   0.0f,    0.0f, -1.0f},
                {  0.0f,   0.0f,  1.0f/e,   z/e},
            }},
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse infinite_perspective(f32 fov, f32 aspect, f32 n)
{
    ASSERT(aspect != 0);
    
    f32 range  = _tan(fov * 0.5f) * n;
    f32 left   = -range * aspect;
    f32 rigth  =  range * aspect;
    f32 bottom = -range;
    f32 top    =  range;
    
    f32 m = (2.0f * n);
    f32 x =  m / (rigth - left);
    f32 y =  m / (top - bottom);
    f32 e = -m;
    M4x4_Inverse result =
    {
        {{ // The transform itself.
                {   x, 0.0f,  0.0f, 0.0f},
                {0.0f,    y,  0.0f, 0.0f},
                {0.0f, 0.0f, -1.0f,    e},
                {0.0f, 0.0f, -1.0f, 0.0f}
            }},
        
        {{ // Its inverse.
                {1.0f/x,   0.0f,    0.0f,    0.0f},
                {  0.0f, 1.0f/y,    0.0f,    0.0f},
                {  0.0f,   0.0f,    0.0f,   -1.0f},
                {  0.0f,   0.0f,  1.0f/e, -1.0f/e},
            }},
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse orthographic_3d(f32 left, f32 right, f32 bottom, f32 top, f32 n, f32 f, b32 normalized_z)
{
    // @Note: normalized_z should be true for D3D11 and false for OpenGL.
    ASSERT(left != right);
    ASSERT(bottom != top);
    
    f32 tz_ = -(n)  / (f - n);
    f32 z_  = -1.0f / (f - n);
    if (!normalized_z) {
        tz_ = -(f + n) / (f - n);
        z_  = -2.0f    / (f - n);
    }
    
    f32 tx = -(right + left) / (right - left);
    f32 ty = -(top + bottom) / (top - bottom);
    f32 tz = tz_;
    f32 x  =  2.0f / (right - left);
    f32 y  =  2.0f / (top - bottom);
    f32 z  = z_;
    
    M4x4_Inverse result = 
    {
        {{ // The transform itself.
                {   x, 0.0f, 0.0f, tx},
                {0.0f,    y, 0.0f, ty},
                {0.0f, 0.0f,    z, tz},
                {0.0f, 0.0f, 0.0f,  1}
            }},
        {{ // Its inverse.
                {1.0f/x,   0.0f,   0.0f, -tx/x},
                {  0.0f, 1.0f/y,   0.0f, -ty/y},
                {  0.0f,   0.0f, 1.0f/z, -tz/z},
                {  0.0f,   0.0f,   0.0f,     1}
            }}
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse orthographic_2d(f32 left, f32 right, f32 bottom, f32 top)
{
    ASSERT(left != right);
    ASSERT(bottom != top);
    
    f32 tx = -(right + left) / (right - left);
    f32 ty = -(top + bottom) / (top - bottom);
    f32 x  =  2.0f / (right - left);
    f32 y  =  2.0f / (top - bottom);
    
    M4x4_Inverse result = 
    {
        {{ // The transform itself.
                {   x, 0.0f, 0.0f, tx},
                {0.0f,    y, 0.0f, ty},
                {0.0f, 0.0f,-1.0f,  0},
                {0.0f, 0.0f, 0.0f,  1}
            }},
        {{ // Its inverse.
                {1.0f/x,   0.0f,   0.0f, -tx/x},
                {  0.0f, 1.0f/y,   0.0f, -ty/y},
                {  0.0f,   0.0f,  -1.0f,     0},
                {  0.0f,   0.0f,   0.0f,     1}
            }}
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}

void calculate_tangents(V3 normal, V3 *tangent_out, V3 *bitangent_out)
{
    V3 a = cross(normal, v3(1.0f, 0.0f, 0.0f));
    V3 b = cross(normal, v3(0.0f, 1.0f, 0.0f));
    
    if (length2(a) > length2(b)) *tangent_out = normalize(a);
    else                         *tangent_out = normalize(b);
    
    *bitangent_out = normalize(cross(*tangent_out, normal));
}

#if defined(__GCC__) || defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(__clang__)
#    pragma clang diagnostic pop
#endif

/////////////////////////////////////////
//
// PRNG Implementation
//
// @Note: From Matrins: https://gist.github.com/mmozeiko/1561361cd4105749f80bb0b9223e9db8
#define PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL
Random_PCG random_seed(u64 seed)
{
    Random_PCG result;
    result.state  = 0ULL;
    random_next(&result);
    result.state += seed;
    random_next(&result);
    
    return result;
}
u32 random_next(Random_PCG *rng)
{
    u64 state  = rng->state;
    rng->state = state * PCG_DEFAULT_MULTIPLIER_64 + PCG_DEFAULT_INCREMENT_64;
    
    // XSH-RR
    u32 value = (u32)((state ^ (state >> 18)) >> 27);
    s32 rot   = state >> 59;
    return rot ? (value >> rot) | (value << (32 - rot)) : value;
}
f32 random_nextf(Random_PCG *rng)
{
    // @Note: returns float in [0, 1) interval.
    
    u32 x = random_next(rng);
    return (f32)(s32)(x >> 8) * 0x1.0p-24f;
}
u32 random_range(Random_PCG *rng, u32 min, u32 max)
{
    // @Note: Returns value in [min, max) interval.
    
    u32 bound     = max - min;
    u32 threshold = -(s32)bound % bound;
    
    for (;;)
    {
        u32 r = random_next(rng);
        if (r >= threshold)
        {
            return min + (r % bound);
        }
    }
}
V2 random_range_v2(Random_PCG *rng, V2 min, V2 max)
{
    // @Note: Returns value in [min, max) interval.
    
    V2 result;
    result.x = lerp(min.x, random_nextf(rng), max.x);
    result.y = lerp(min.y, random_nextf(rng), max.y);
    return result;
}
V3 random_range_v3(Random_PCG *rng, V3 min, V3 max)
{
    // @Note: Returns value in [min, max) interval.
    
    V3 result;
    result.x = lerp(min.x, random_nextf(rng), max.x);
    result.y = lerp(min.y, random_nextf(rng), max.y);
    result.z = lerp(min.z, random_nextf(rng), max.z);
    return result;
}

/////////////////////////////////////////
//
// Memory Arena Implementation
//
void * memory_copy(void *dst, void *src, u64 size)
{
    u8 *__dst = (u8 *)dst;
    u8 *__src = (u8 *)src;
    
    while (size--)
        *__dst++ = *__src++;
    
    return dst;
}
void * memory_set(void *dst, s32 val, u64 size)
{
    u8 *__dst =  (u8 *)dst;
    u8  __val = *(u8 *)&val;
    
    while (size--)
        *__dst++ = __val;
    
    return dst;
}

Arena* arena_init(u64 max_size)
{
    Arena *result = 0;
    if (max_size > ARENA_COMMIT_SIZE) {
        void *memory = os->reserve(max_size);
        if (os->commit(memory, ARENA_COMMIT_SIZE)) {
            result              = (Arena *)memory;
            result->max         = max_size;
            result->used        = ALIGN_UP(sizeof(Arena), 64);
            result->commit_used = ARENA_COMMIT_SIZE;
        }
    }
    ASSERT(result != 0);
    return result;
}
void arena_free(Arena *arena)
{
    os->release(arena);
}
void* arena_push(Arena *arena, u64 size)
{
    void *result = 0;
    u64 s = arena->used + size; 
    if (s < arena->max) {
        if (s > arena->commit_used) {
            // Commit more pages.
            u64 commit_size     = (size + (ARENA_COMMIT_SIZE-1));
            commit_size        -= (commit_size % ARENA_COMMIT_SIZE);
            if (os->commit(((u8*)arena) + arena->commit_used, commit_size))
                arena->commit_used += commit_size;
        }
        
        if (s <= arena->commit_used) {
            result      = ((u8*)arena) + arena->used;
            arena->used = s;
        }
    }
    ASSERT(result != 0);
    return result;
}
void* arena_push_zero(Arena *arena, u64 size)
{
    void *result = arena_push(arena, size);
    MEMORY_ZERO(result, size);
    return result;
}
void arena_pop(Arena *arena, u64 size)
{
    // @Todo: Decommit memory.
    
    // @Note: Make sure we don't clear arena details/header by accident.
    u64 header_size = ALIGN_UP(sizeof(Arena), 64);
    size = CLAMP_UPPER(arena->used - header_size, size);
    arena->used -= size;
}
void arena_reset(Arena *arena)
{
    arena_pop(arena, arena->used);
}
Arena_Temp arena_temp_begin(Arena *arena)
{
    Arena_Temp result = {arena, arena->used};
    return result;
}
void arena_temp_end(Arena_Temp temp)
{
    if (temp.arena->used >= temp.used)
        temp.arena->used = temp.used;
}

threadvar Arena *scratch_pool[ARENA_SCRATCH_COUNT] = {};
Arena_Temp get_scratch(Arena **conflict_array, s32 count)
{
    // Initialize arenas on first visit.
    if (scratch_pool[0] == 0) {
        for (s32 i = 0; i < ARENA_SCRATCH_COUNT; i++)
            scratch_pool[i] = arena_init();
    }
    
    // Get non-conflicting arena.
    Arena_Temp result = {};
    for (s32 i = 0; i < ARENA_SCRATCH_COUNT; i++) {
        b32 is_used = false;
        for (s32 j = 0; j < count; j++) {
            if (scratch_pool[i] == conflict_array[j]) {
                is_used = true;
                break;
            }
        }
        
        if (!is_used) {
            result = arena_temp_begin(scratch_pool[i]);
            break;
        }
    }
    
    return result;
}

/////////////////////////////////////////
//
// String8 Implementation
//
String8 string(u8 *data, u64 count)
{ 
    String8 result = {data, count};
    return result;
}
String8 string(const char *c_string)
{
    String8 result;
    result.data  = (u8 *) c_string;
    result.count = string_length(c_string);
    return result;
}
u64 string_length(const char *c_string)
{
    u64 result = 0;
    while (*c_string++) result++;
    return result;
}
String8 string_copy(String8 s, Arena *arena /*= 0*/)
{
    arena = (arena? arena : os->permanent_arena);
    
    String8 result;
    result.count = s.count;
    result.data  = PUSH_ARRAY(arena, u8, result.count + 1);
    memory_copy(result.data, s.data, result.count);
    result.data[result.count] = 0;
    return result;
}
String8 string_cat(String8 a, String8 b, Arena *arena /*= 0*/)
{
    arena = (arena? arena : os->permanent_arena);
    
    String8 result;
    result.count = a.count + b.count;
    result.data  = PUSH_ARRAY(arena, u8, result.count + 1);
    memory_copy(result.data, a.data, a.count);
    memory_copy(result.data + a.count, b.data, b.count);
    result.data[result.count] = 0;
    return result;
}
b32 string_empty(String8 s)
{
    b32 result = (!s.data || !s.count);
    return result;
}
b32 string_match(String8 a, String8 b)
{
    if (a.count != b.count) return false;
    for(u64 i = 0; i < a.count; i++)
    {
        if (a.data[i] != b.data[i]) return false;
    }
    return true;
}

/////////////////////////////////////////
//
// String Helper Functions Implementation
//
void advance(String8 *s, u64 count)
{
    count     = CLAMP_UPPER(s->count, count);
    s->data  += count;
    s->count -= count;
}
u32 get_hash(String8 s)
{
    // djb2 hash from http://www.cse.yorku.ca/~oz/hash.html
    
    u32 hash = 5381;
    
    for(s32 i = 0; i < s.count; i++)
    {
        /* hash * 33 + s[i] */
        hash = ((hash << 5) + hash) + *(s.data + i);
    }
    
    return hash;
}
b32 is_spacing(char c)
{
    b32 result = ((c == ' ')  ||
                  (c == '\t') ||
                  (c == '\v') ||
                  (c == '\f'));
    return result;
}
b32 is_end_of_line(char c)
{
    b32 result = ((c == '\n') ||
                  (c == '\r'));
    return result;
}
b32 is_whitespace(char c)
{
    b32 result = (is_spacing(c) || is_end_of_line(c));
    return result;
}
b32 is_alpha(char c)
{
    b32 result = (((c >= 'A') && (c <= 'Z')) ||
                  ((c >= 'a') && (c <= 'z')));
    return result;
}
b32 is_numeric(char c)
{
    b32 result = (((c >= '0') && (c <= '9')));
    return result;
}
b32 is_alphanumeric(char c)
{
    b32 result = (is_alpha(c) || is_numeric(c));
    return result;
}
b32 is_file_separator(char c)
{
    b32 result = ((c == '\\') || (c == '/'));
    return result;
}

/////////////////////////////////////////
//
// String Format Implementation
//
GLOBAL char decimal_digits[]   = "0123456789";
GLOBAL char lower_hex_digits[] = "0123456789abcdef";
GLOBAL char upper_hex_digits[] = "0123456789ABCDEF";

void put_char(String8 *dest, char c)
{
    if (dest->count) {
        dest->count--;
        *dest->data++ = c;
    }
}
void put_c_string(String8 *dest, const char *c_string)
{
    while (*c_string) put_char(dest, *c_string++);
    // @Todo: Null-terminate!
}
void u64_to_ascii(String8 *dest, u64 value, u32 base, char *digits)
{
    ASSERT(base != 0);
    
    u8 *start = dest->data;
    do
    {
        put_char(dest, digits[value % base]);
        value /= base;
    } while (value != 0);
    u8 *end  = dest->data;
    
    // Reverse.
    while (start < end) {
        end--;
        SWAP(*end, *start, char);
        start++;
    }
}
void f64_to_ascii(String8 *dest, f64 value, u32 precision)
{
    if (value < 0) {
        put_char(dest, '-');
        value = -value;
    }
    
    u64 integer_part = (u64) value;
    value           -= (f64) integer_part;
    
    u64_to_ascii(dest, integer_part, 10, decimal_digits);
    put_char(dest, '.');
    
    for(u32 precision_index = 0;
        precision_index < precision;
        precision_index++)
    {
        value       *= 10.0f;
        integer_part = (u64) value;
        value       -= (f64) integer_part;
        put_char(dest, decimal_digits[integer_part]);
    }
}
u64 ascii_to_u64(char **at)
{
    u64 result = 0;
    char *tmp = *at;
    while (is_numeric(*tmp)) {
        result *= 10;
        result += *tmp - '0';
        tmp++;
    }
    *at = tmp;
    return result;
}
u64 string_format_list(char *dest_start, u64 dest_count, const char *format, va_list arg_list)
{
    if (!dest_count) return 0;
    
    String8 dest_buffer = string((u8*)dest_start, dest_count);
    
    char *at = (char *) format;
    while (*at) {
        if (*at == '%') {
            at++;
            
            b32 use_precision = false;
            u32 precision     = 0;
            
            // Handle precision specifier.
            if (*at == '.') {
                at++;
                
                if (is_numeric(*at)) {
                    use_precision = true;
                    precision     = (u32) ascii_to_u64(&at); // at is advanced inside the function.
                } else ASSERT(!"Invalid precision specifier!");
            }
            if (!use_precision) precision = 6;
            
            switch (*at)  {
                case 'b': {
                    b32 b = (b32) va_arg(arg_list, b32);
                    if (b) put_c_string(&dest_buffer, "True");
                    else   put_c_string(&dest_buffer, "False");
                } break;
                
                case 'c': {
                    char c = (char) va_arg(arg_list, s32);
                    put_char(&dest_buffer, c);
                } break;
                
                case 's': {
                    char *c_str = va_arg(arg_list, char *);
                    put_c_string(&dest_buffer, c_str);
                } break;
                
                case 'S': {
                    String8 s = va_arg(arg_list, String8);
                    for(u64 i = 0; i < s.count; i++) put_char(&dest_buffer, s.data[i]);
                    // @Todo: Null-terminate!
                } break;
                
                case 'i':
                case 'd': {
                    s64 value = (s64) va_arg(arg_list, s32);
                    
                    if (value < 0)  {
                        put_char(&dest_buffer, '-');
                        value = -value;
                    }
                    
                    u64_to_ascii(&dest_buffer, (u64)value, 10, decimal_digits);
                } break;
                
                case 'u': {
                    u64 value = (u64) va_arg(arg_list, u32);
                    u64_to_ascii(&dest_buffer, value, 10, decimal_digits);
                } break;
                
                case 'm': {
                    umm value = va_arg(arg_list, umm);
                    const char *suffix = "B";
                    
                    // Round up.
                    if (value >= KILOBYTES(1)) {
                        suffix = "KB";
                        value = (value + KILOBYTES(1) - 1) / KILOBYTES(1);
                    } else if (value >= MEGABYTES(1)) {
                        suffix = "MB";
                        value = (value + MEGABYTES(1) - 1) / MEGABYTES(1);
                    } else if (value >= GIGABYTES(1)) {
                        suffix = "GB";
                        value = (value + GIGABYTES(1) - 1) / GIGABYTES(1);
                    }
                    
                    u64_to_ascii(&dest_buffer, value, 10, decimal_digits);
                    put_c_string(&dest_buffer, suffix);
                } break;
                
                case 'f': {
                    f64 value = va_arg(arg_list, f64);
                    f64_to_ascii(&dest_buffer, value, precision);
                } break;
                
                case 'v': {
                    if ((at[1] != '2') && 
                        (at[1] != '3') && 
                        (at[1] != '4'))
                        ASSERT(!"Unrecognized vector specifier!");
                    
                    s32 vector_size = (s32)(at[1] - '0');
                    V4 *v = 0;
                    
                    if (vector_size == 2) {
                        V2 t = va_arg(arg_list, V2);
                        v    = (V4 *) &t;
                    } else if (vector_size == 3) {
                        V3 t = va_arg(arg_list, V3);
                        v    = (V4 *) &t;
                    } else if (vector_size == 4) {
                        V4 t = va_arg(arg_list, V4);
                        v    = (V4 *) &t;
                    }
                    
                    put_char(&dest_buffer, '[');
                    for(s32 i = 0; i < vector_size; i++)
                    {
                        f64_to_ascii(&dest_buffer, v->I[i], precision);
                        if (i < (vector_size - 1)) put_c_string(&dest_buffer, ", ");
                    }
                    put_char(&dest_buffer, ']');
                    
                    at++;
                } break;
                
                case '%': {
                    put_char(&dest_buffer, '%');
                } break;
                
                default:
                {
                    ASSERT(!"Unrecognized format specifier!");
                } break;
            }
            
            if (*at) at++;
        } else {
            put_char(&dest_buffer, *at++);
        }
    }
    
    u64 result = dest_buffer.data - (u8*)dest_start;
    
    if (dest_buffer.count) 
        dest_buffer.data[0] = 0;
    else {
        dest_buffer.data[-1] = 0;
        result--;
    }
    
    return result;
}
u64 string_format(char *dest_start, u64 dest_count, const char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    u64 result = string_format_list(dest_start, dest_count, format, arg_list);
    va_end(arg_list);
    
    return result;
}
String8 sprint(const char *format, ...)
{
    char temp[4096];
    String8 s = {};
    
    va_list arg_list;
    va_start(arg_list, format);
    s.data  = (u8 *) temp;
    s.count = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    String8 result = string_copy(s, os->permanent_arena);
    return result;
}
String8 sprint(Arena *arena, const char *format, ...)
{
    char temp[4096];
    String8 s = {};
    
    va_list arg_list;
    va_start(arg_list, format);
    s.data  = (u8 *) temp;
    s.count = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    String8 result = string_copy(s, arena);
    return result;
}
void print(const char *format, ...)
{
    char temp[4096];
    String8 s = {};
    
    va_list arg_list;
    va_start(arg_list, format);
    s.data  = (u8 *) temp;
    s.count = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    os->print_to_console(s);
}

/////////////////////////////////////////
//
// String Builder Implementation
//
String_Builder sb_init(u64 capacity /*= SB_BLOCK_SIZE*/)
{
    String_Builder builder = {};
    builder.arena    = arena_init();
    builder.capacity = capacity;
    sb_reset(&builder);
    return builder;
}
void sb_free(String_Builder *builder)
{
    arena_free(builder->arena);
}
void sb_reset(String_Builder *builder)
{
    arena_reset(builder->arena);
    builder->length = 0;
    builder->start  = PUSH_ARRAY(builder->arena, u8, builder->capacity);
    builder->buffer = {builder->start, builder->capacity};
}
void sb_append(String_Builder *builder, void *data, u64 size)
{
    if ((builder->length + size) > builder->capacity) {
        builder->capacity += (size + (SB_BLOCK_SIZE-1));
        builder->capacity -= (builder->capacity % SB_BLOCK_SIZE);
        u64 len = builder->length;
        sb_reset(builder);
        builder->length = len;
        advance(&builder->buffer, builder->length);
    }
    
    memory_copy(builder->buffer.data, data, size);
    advance(&builder->buffer, size);
    
    builder->length = builder->buffer.data - builder->start;
}
void sb_appendf(String_Builder *builder, char *format, ...)
{
    char temp[4096];
    
    va_list arg_list;
    va_start(arg_list, format);
    u64 size = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    sb_append(builder, temp, size);
}
String8 sb_to_string(String_Builder *builder, Arena *arena /*= 0*/)
{
    if (builder->buffer.count)
        builder->buffer.data[0] = 0;
    else {
        builder->buffer.data[-1] = 0;
        builder->length--;
    }
    
    String8 result = string_copy(string(builder->start, builder->length), arena);
    return result;
}

/////////////////////////////////////////
//
// Dynamic Array Implementation
//
// In the header part of this file.

/////////////////////////////////////////
//
// Hash Table Implementation
//
// In the header part of this file.

/////////////////////////////////////////
//
// OS Implementation
//
OS_State *os = 0;

b32 key_pressed(s32 key, b32 capture /* = false */)
{
    b32 result = os->pressed[key];
    if (result && capture)
        os->pressed[key] = false;
    return result;
}
b32 key_held(s32 key, b32 capture /* = false */)
{
    b32 result = os->held[key];
    if (result && capture)
        os->pressed[key] = false;
    return result;
}
b32 key_released(s32 key, b32 capture /* = false */)
{
    b32 result = os->released[key];
    if (result && capture)
        os->released[key] = false;
    return result;
}
Rect2 aspect_ratio_fit(V2u render_dim, V2u window_dim)
{
    // @Note: From Handmade Hero E.322.
    
    Rect2 result = {};
    f32 rw = (f32)render_dim.width;
    f32 rh = (f32)render_dim.height;
    f32 ww = (f32)window_dim.width;
    f32 wh = (f32)window_dim.height;
    
    if ((rw > 0) && (rh > 0) && (ww > 0) && (wh > 0)) {
        f32 optimal_ww = wh * (rw / rh);
        f32 optimal_wh = ww * (rh / rw);
        
        if (optimal_ww > ww) {
            // Width-constrained display (top and bottom black bars).
            result.min.x = 0;
            result.max.x = ww;
            
            f32 empty_space = wh - optimal_wh;
            f32 half_empty  = _round(0.5f*empty_space);
            f32 used_height = _round(optimal_wh);
            
            result.min.y = half_empty; 
            result.max.y = result.min.y + used_height;
        } else {
            // Height-constrained display (left and right black bars).
            result.min.y = 0;
            result.max.y = wh;
            
            f32 empty_space = ww - optimal_ww;
            f32 half_empty  = _round(0.5f*empty_space);
            f32 used_height = _round(optimal_ww);
            
            result.min.x = half_empty; 
            result.max.x = result.min.x + used_height;
        }
    }
    
    return result;
}
V3 unproject(V3 camera_position, f32 Zworld_distance_from_camera,
             V3 mouse_ndc, M4x4_Inverse view, M4x4_Inverse proj)
{
    // @Note: Handmade Hero EP.373 and EP.374
    
    V3 camera_zaxis = get_row(view.forward, 2);
    V3 new_p        = camera_position - Zworld_distance_from_camera*camera_zaxis;
    V4 probe_z      = v4(new_p, 1.0f);
    
    // Get probe_z in clip space.
    probe_z = proj.forward*view.forward*probe_z;
    
    // Undo the perspective divide.
    mouse_ndc.x *= probe_z.w;
    mouse_ndc.y *= probe_z.w;
    
    V4 mouse_clip = v4(mouse_ndc.x, mouse_ndc.y, probe_z.z, probe_z.w);
    V3 result     = (view.inverse*proj.inverse*mouse_clip).xyz;
    
    return result;
}

#endif // ORH_IMPLEMENTATION