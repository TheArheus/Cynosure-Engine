
#pragma once
#include <math.h>

#include <immintrin.h>

#if __cplusplus == 202002L
template<typename T>
constexpr T Pi = T(3.1415926535897932384626433832795028841971693993751058209749445923078164062);
#else
constexpr auto Pi (3.1415926535897932384626433832795028841971693993751058209749445923078164062);
#endif

template<typename T>
T Radians(T V)
{
#if __cplusplus == 202002L
	return V * Pi<T> / T(180);
#else
	return V * Pi / T(180);
#endif
}

template<typename T>
T Degrees(T V)
{
#if __cplusplus == 202002L
	return V * T(180) / Pi<T>;
#else
	return V * T(180) / Pi;
#endif
}


inline uint16_t
EncodeHalf(float x)
{
    uint32_t v = *(uint32_t*)&x;

    uint32_t s = (v & 0x8000'0000) >> 31;
    uint32_t e = (v & 0x7f80'0000) >> 23;
    uint32_t m = (v & 0x007f'ffff);

    if(e >= 255)	 return (s << 15) | 0x7c00;
    if(e > (127+15)) return (s << 15) | 0x7c00;
    if(e < (127-14)) return (s << 15) | 0x0 | (m >> 13);

    e = e - 127 + 15;
    m = m / float(1 << 23) * float(1 << 10);

    return (s << 15) | (e << 10) | (m);
}

inline float
DecodeHalf(uint16_t Val)
{
    float Res = 0;

    bool Sign =  Val & 0x8000;
    uint8_t   Exp  = (Val & 0x7C00) >> 10;
    uint16_t  Mant =  Val & 0x03FF;

    Res = powf(-1, Sign) * (1 << (Exp - 15)) * (1 + float(Mant) / (1 << 10));

    return Res;
}

template<typename vec_t, unsigned int a, unsigned int b>
struct swizzle_2d
{
    typename vec_t::type E[2];
    swizzle_2d() = default;
    swizzle_2d(const vec_t& Vec)
    {
        E[a] = Vec.x;
        E[b] = Vec.y;
    }
    vec_t operator=(const vec_t& Vec)
    {
        return vec_t(E[a] = Vec.x, E[b] = Vec.y);
    }
    operator vec_t()
    {
        return vec_t(E[a], E[b]);
    }
#if __cplusplus == 202002L
    template<template<typename T> class vec_c>
        requires std::is_same_v<typename vec_t::type, uint16_t>
    operator vec_c<float>()
    {
        return vec_c<float>(DecodeHalf(E[a]), DecodeHalf(E[b]));
    }
#else
#endif
    template<class vec_c>
    operator vec_c()
    {
        return vec_c(E[a], E[b]);
    }
};

template<typename vec_t, unsigned int a, unsigned int b, unsigned int c>
struct swizzle_3d
{
    typename vec_t::type E[3];
    swizzle_3d() = default;
    swizzle_3d(const vec_t& Vec)
    {
        E[a] = Vec.x;
        E[b] = Vec.y;
        E[c] = Vec.z;
    }
    vec_t operator=(const vec_t& Vec)
    {
        return vec_t(E[a] = Vec.x, E[b] = Vec.y, E[c] = Vec.z);
    }
    operator vec_t()
    {
        return vec_t(E[a], E[b], E[c]);
    }
#if __cplusplus == 202002L
    template<template<typename T> class vec_c>
        requires std::is_same_v<typename vec_t::type, uint16_t>
    operator vec_c<float>()
    {
        return vec_c<float>(DecodeHalf(E[a]), DecodeHalf(E[b]), DecodeHalf(E[c]));
    }
#else
#endif
    template<class vec_c>
    operator vec_c()
    {
        return vec_c(E[a], E[b], E[c]);
    }
};

template<typename vec_t, unsigned int a, unsigned int b, unsigned int c, unsigned int d>
struct swizzle_4d
{
    typename vec_t::type E[4];
    swizzle_4d() = default;
    swizzle_4d(const vec_t& Vec)
    {
        E[a] = Vec.x;
        E[b] = Vec.y;
        E[c] = Vec.z;
        E[d] = Vec.w;
    }
    vec_t operator=(const vec_t& Vec)
    {
        return vec_t(E[a] = Vec.x, E[b] = Vec.y, E[c] = Vec.z, E[d] = Vec.w);
    }
    operator vec_t()
    {
        return vec_t(E[a], E[b], E[c], E[d]);
    }
#if __cplusplus == 202002L
    template<template<typename T> class vec_c>
        requires std::is_same_v<typename vec_t::type, uint16_t>
    operator vec_c<float>()
    {
        return vec_c<float>(DecodeHalf(E[a]), DecodeHalf(E[b]), DecodeHalf(E[c]), DecodeHalf(E[d]));
    }
#else
#endif
    template<class vec_c>
    operator vec_c()
    {
        return vec_c(E[a], E[b], E[c], E[d]);
    }
};

template<typename T>
struct v2
{
    using type = T;
    union
    {
        struct
        {
            T x, y;
        };
        T E[2];
        swizzle_2d<v2<T>, 0, 0> xx;
        swizzle_2d<v2<T>, 1, 1> yy;
        swizzle_2d<v2<T>, 0, 1> xy;
    };

    v2() = default;

    v2(T V) : x(V), y(V) {};
    v2(T _x, T _y) : x(_x), y(_y){};

    T& operator[](uint32_t Idx)
    {
        return E[Idx];
    }

    v2 operator+(const v2& rhs)
    {
        v2 Result = {};
        Result.x = this->x + rhs.x;
        Result.y = this->y + rhs.y;
        return Result;
    }

    v2 operator+(const float& rhs)
    {
        v2 Result = {};
        Result.x = this->x + rhs;
        Result.y = this->y + rhs;
        return Result;
    }

    v2& operator+=(const v2& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    v2& operator+=(const float& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

	v2 operator-()
	{
        v2 Result = {};
        Result.x = -this->x;
        Result.y = -this->y;
        return Result;
	}

    v2 operator-(const v2& rhs)
    {
        v2 Result = {};
        Result.x = this->x - rhs.x;
        Result.y = this->y - rhs.y;
        return Result;
    }

    v2 operator-(const float& rhs)
    {
        v2 Result = {};
        Result.x = this->x - rhs;
        Result.y = this->y - rhs;
        return Result;
    }

    v2& operator-=(const v2& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    v2& operator-=(const float& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    v2 operator*(const v2& rhs)
    {
        v2 Result = {};
        Result.x = this->x * rhs.x;
        Result.y = this->y * rhs.y;
        return Result;
    }

    v2 operator*(const float& rhs)
    {
        v2 Result = {};
        Result.x = this->x * rhs;
        Result.y = this->y * rhs;
        return Result;
    }

    v2& operator*=(const v2& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    v2& operator*=(const float& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    v2 operator/(const v2& rhs)
    {
        v2 Result = {};
        Result.x = this->x / rhs.x;
        Result.y = this->y / rhs.y;
        return Result;
    }

    v2 operator/(const float& rhs)
    {
        v2 Result = {};
        Result.x = this->x / rhs;
        Result.y = this->y / rhs;
        return Result;
    }

    v2& operator/=(const v2& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    v2& operator/=(const float& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    bool operator==(const v2& rhs) const
    {
        return (this->x == rhs.x) && (this->y == rhs.y);
    }

    float Dot(const v2& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y;
    }

    float LengthSq()
    {
        return Dot(*this);
    }

    float Length()
    {
        return sqrtf(LengthSq());
    }

    v2 Normalize()
    {
        v2 Result = *this / Length();
        return Result;
    }
};

struct vech2 : v2<uint16_t>
{
    using v2::v2;
    vech2(const v2<float>& V)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
    }
};

template<typename T>
struct v4;
template<typename T>
struct v3
{
    using type = T;
    union
    {
        struct
        {
            T x, y, z;
        };
        struct
        {
            T r, g, b;
        };
        T E[3];
        swizzle_2d<v2<T>, 0, 1>    xy;
        swizzle_2d<v2<T>, 0, 0>    xx;
        swizzle_2d<v2<T>, 1, 1>    yy;
        swizzle_3d<v3<T>, 0, 0, 0> xxx;
        swizzle_3d<v3<T>, 1, 1, 1> yyy;
        swizzle_3d<v3<T>, 2, 2, 2> zzz;
        swizzle_3d<v3<T>, 0, 1, 2> xyz;
    };

    v3() = default;

    v3(T V) : x(V), y(V), z(V) {};
    v3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {};
	v3(const v4<T>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {};

    T& operator[](uint32_t Idx)
    {
        return E[Idx];
    }

    v3 operator+(const v3& rhs)
    {
        v3 Result = {};
        Result.x = this->x + rhs.x;
        Result.y = this->y + rhs.y;
        Result.z = this->z + rhs.z;
        return Result;
    }

    v3 operator+(const float& rhs)
    {
        v3 Result = {};
        Result.x = this->x + rhs;
        Result.y = this->y + rhs;
        Result.z = this->z + rhs;
        return Result;
    }

    v3& operator+=(const v3& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    v3& operator+=(const float& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

	v3 operator-()
	{
        v3 Result = {};
        Result.x = -this->x;
        Result.y = -this->y;
        Result.z = -this->z;
        return Result;
	}

    v3 operator-(const v3& rhs)
    {
        v3 Result = {};
        Result.x = this->x - rhs.x;
        Result.y = this->y - rhs.y;
        Result.z = this->z - rhs.z;
        return Result;
    }

    v3 operator-(const float& rhs)
    {
        v3 Result = {};
        Result.x = this->x - rhs;
        Result.y = this->y - rhs;
        Result.z = this->z - rhs;
        return Result;
    }

    v3& operator-=(const v3& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    v3& operator-=(const float& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    v3 operator*(const v3& rhs)
    {
        v3 Result = {};
        Result.x = this->x * rhs.x;
        Result.y = this->y * rhs.y;
        Result.z = this->z * rhs.z;
        return Result;
    }

    v3 operator*(const float& rhs)
    {
        v3 Result = {};
        Result.x = this->x * rhs;
        Result.y = this->y * rhs;
        Result.z = this->z * rhs;
        return Result;
    }

    v3& operator*=(const v3& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    v3& operator*=(const float& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    v3 operator/(const v3& rhs)
    {
        v3 Result = {};
        Result.x = this->x / rhs.x;
        Result.y = this->y / rhs.y;
        Result.z = this->z / rhs.z;
        return Result;
    }

    v3 operator/(const float& rhs)
    {
        v3 Result = {};
        Result.x = this->x / rhs;
        Result.y = this->y / rhs;
        Result.z = this->z / rhs;
        return Result;
    }

    v3& operator/=(const v3& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    v3& operator/=(const float& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

	v3& operator=(const v4<T>& rhs)
	{
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        return *this;
	}

    bool operator==(const v3& rhs) const
    {
        return (this->x == rhs.x) && (this->y == rhs.y) && (this->z == rhs.z);
    }

    float Dot(const v3& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
    }

    float LengthSq()
    {
        return Dot(*this);
    }

    float Length()
    {
        return sqrtf(LengthSq());
    }

    v3 Normalize()
    {
        v3 Result = *this / Length();
        return Result;
    }
};

struct vech3 : v3<uint16_t>
{
    using v3::v3;
    vech3(const v3<float>& V)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
        z = EncodeHalf(V.z);
    }
    vech3(const v3<uint16_t>& V)
    {
        x = V.x;
        y = V.y;
        z = V.z;
    }
    vech3(uint16_t _x, uint16_t _y, uint16_t _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }
};

template<typename T>
struct v4
{
    using type = T;
    union
    {
        struct
        {
            T x, y, z, w;
        };
        struct
        {
            T r, g, b, a;
        };
        T E[4];
        swizzle_2d<v2<T>, 0, 1>       xy;
        swizzle_2d<v2<T>, 0, 0>       xx;
        swizzle_2d<v2<T>, 1, 1>       yy;
        swizzle_3d<v3<T>, 0, 0, 0>    xxx;
        swizzle_3d<v3<T>, 1, 1, 1>    yyy;
        swizzle_3d<v3<T>, 2, 2, 2>    zzz;
        swizzle_3d<v3<T>, 0, 1, 2>    xyz;
        swizzle_4d<v4<T>, 0, 1, 2, 3> xyzw;
        swizzle_4d<v4<T>, 0, 0, 0, 0> xxxx;
        swizzle_4d<v4<T>, 1, 1, 1, 1> yyyy;
        swizzle_4d<v4<T>, 2, 2, 2, 2> zzzz;
        swizzle_4d<v4<T>, 3, 3, 3, 3> wwww;
    };

    v4() = default;

    v4(T V) : x(V), y(V), z(V), w(V) {};
    v4(v2<T> V) : x(V.x), y(V.y), z(0), w(0) {};
    v4(v3<T> V) : x(V.x), y(V.y), z(V.z), w(0) {};
    v4(v3<T> V, T _w) : x(V.x), y(V.y), z(V.z), w(_w) {};
    //v4(const v4<T>& V) : x(V.x), y(V.y), z(V.z), w(V.w) {};
    template<typename U>
    v4(U _x, U _y, U _z, U _w) : x(_x), y(_y), z(_z), w(_w) {};
    v4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {};

    T& operator[](uint32_t Idx)
    {
        return E[Idx];
    }

    v4 operator+(const v4& rhs)
    {
        v4 Result = {};
        Result.x = this->x + rhs.x;
        Result.y = this->y + rhs.y;
        Result.z = this->z + rhs.z;
        Result.w = this->w + rhs.w;
        return Result;
    }

    v4 operator+(const float& rhs)
    {
        v4 Result = {};
        Result.x = this->x + rhs;
        Result.y = this->y + rhs;
        Result.z = this->z + rhs;
        Result.w = this->w + rhs;
        return Result;
    }

    v4& operator+=(const v4& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    v4& operator+=(const float& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

	v4 operator-()
	{
        v4 Result = {};
        Result.x = -this->x;
        Result.y = -this->y;
        Result.z = -this->z;
        Result.w = -this->w;
        return Result;
	}

    v4 operator-(const v4& rhs)
    {
        v4 Result = {};
        Result.x = this->x - rhs.x;
        Result.y = this->y - rhs.y;
        Result.z = this->z - rhs.z;
        Result.w = this->w - rhs.w;
        return Result;
    }

    v4 operator-(const float& rhs)
    {
        v4 Result = {};
        Result.x = this->x - rhs;
        Result.y = this->y - rhs;
        Result.z = this->z - rhs;
        Result.w = this->w - rhs;
        return Result;
    }

    v4& operator-=(const v4& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    v4& operator-=(const float& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    v4 operator*(const v4& rhs)
    {
        v4 Result = {};
        Result.x = this->x * rhs.x;
        Result.y = this->y * rhs.y;
        Result.z = this->z * rhs.z;
        Result.w = this->w * rhs.w;
        return Result;
    }

    v4 operator*(const float& rhs)
    {
        v4 Result = {};
        Result.x = this->x * rhs;
        Result.y = this->y * rhs;
        Result.z = this->z * rhs;
        Result.w = this->w * rhs;
        return Result;
    }

    v4& operator*=(const v4& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    v4& operator*=(const float& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    v4 operator/(const v4& rhs)
    {
        v4 Result = {};
        Result.x = this->x / rhs.x;
        Result.y = this->y / rhs.y;
        Result.z = this->z / rhs.z;
        Result.w = this->w / rhs.w;
        return Result;
    }

    v4 operator/(const float& rhs)
    {
        v4 Result = {};
        Result.x = this->x / rhs;
        Result.y = this->y / rhs;
        Result.z = this->z / rhs;
        Result.w = this->w / rhs;
        return Result;
    }

    v4& operator/=(const v4& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    v4& operator/=(const float& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    v4<T> operator=(const v4<T>& rhs)
    {
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        this->w = rhs.w;
        return *this;
    }

    template<typename U>
    v4 operator=(const v4<U>& rhs)
    {
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        this->w = rhs.w;
        return *this;
    }

    bool operator==(const v4& rhs) const
    {
        return (this->x == rhs.x) && (this->y == rhs.y) && (this->z == rhs.z) && (this->w == rhs.w);
    }

    float Dot(const v4& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
    }

    float LengthSq()
    {
        return Dot(*this);
    }

    float Length()
    {
        return sqrtf(LengthSq());
    }

    v4 Normalize()
    {
        v4 Result = *this / Length();
        return Result;
    }
};

struct vech4 : v4<uint16_t>
{
    using v4::v4;
    vech4(const v4<float>& V)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
        z = EncodeHalf(V.z);
        w = EncodeHalf(V.w);
    }
    vech4(const v3<float>& V, float W)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
        z = EncodeHalf(V.z);
        w = EncodeHalf(W);
    }
    vech4(float X, float Y, float Z, float W)
    {
        x = EncodeHalf(X);
        y = EncodeHalf(Y);
        z = EncodeHalf(Z);
        w = EncodeHalf(W);
    }
};

template<typename T>
inline v4<T>
operator*(const v4<T>& lhs, const v4<T>& rhs)
{
    v4<T> Result = {};
    Result.x = lhs.x * rhs.x;
    Result.y = lhs.y * rhs.y;
    Result.z = lhs.z * rhs.z;
    Result.w = lhs.w * rhs.w;
    return Result;
}

template<typename T>
inline v4<T>
operator*(const v4<T>& lhs, const float& rhs)
{
    v4<T> Result = {};
    Result.x = lhs.x * rhs;
    Result.y = lhs.y * rhs;
    Result.z = lhs.z * rhs;
    Result.w = lhs.w * rhs;
    return Result;
}

template<typename T>
inline v4<T>
operator*=(v4<T> lhs, const v4<T>& rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

template<typename T>
inline v4<T>
operator*=(v4<T> lhs, const float& rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

template<typename T>
inline float 
Dot(const v2<T>& lhs, const v2<T>& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y;
}

template<typename T>
inline float 
LengthSq(const v2<T>& v)
{
	return Dot(v, v);
}

template<typename T> 
inline float 
Length(const v2<T>& v)
{
	return sqrtf(LengthSq(v));
}

template<typename T>
inline v2<T>
Normalize(v2<T>& v)
{
	v2<T> Result = v / v.Length();
	return Result;
}

template<typename T>
inline float 
Dot(const v3<T>& lhs, const v3<T>& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

template<typename T>
inline float 
LengthSq(const v3<T>& v)
{
	return Dot(v, v);
}

template<typename T> 
inline float 
Length(const v3<T>& v)
{
	return sqrtf(LengthSq(v));
}

template<typename T>
inline v3<T>
Normalize(v3<T>& v)
{
	v3<T> Result = v / v.Length();
	return Result;
}

template<typename T>
inline float 
Dot(const v4<T>& lhs, const v4<T>& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

template<typename T>
inline float 
LengthSq(const v4<T>& v)
{
	return Dot(v, v);
}

template<typename T> 
inline float 
Length(const v4<T>& v)
{
	return sqrtf(LengthSq(v));
}

template<typename T>
inline v4<T>
Normalize(v4<T>& v)
{
	v4<T> Result = v / v.Length();
	return Result;
}

template<typename A, typename B>
inline auto
Min(A a, B b)
{
	return a < b ? a : b;
}

template<typename T>
inline v2<T>
Min(const v2<T>& a, const v2<T>& b)
{
	v2<T> Result = {};
	Result.x = Min(a.x, b.x);
	Result.y = Min(a.y, b.y);
	return Result;
}

template<typename T>
inline v3<T>
Min(const v3<T>& a, const v3<T>& b)
{
	v3<T> Result = {};
	Result.x = Min(a.x, b.x);
	Result.y = Min(a.y, b.y);
	Result.z = Min(a.z, b.z);
	return Result;
}

template<typename T>
inline v4<T>
Min(const v4<T>& a, const v4<T>& b)
{
	v4<T> Result = {};
	Result.x = Min(a.x, b.x);
	Result.y = Min(a.y, b.y);
	Result.z = Min(a.z, b.z);
	Result.w = Min(a.w, b.w);
	return Result;
}

template<typename A, typename B>
inline auto
Max(A a, B b)
{
	return a > b ? a : b;
}

template<typename T>
inline v2<T>
Max(const v2<T>& a, const v2<T>& b)
{
	v2<T> Result = {};
	Result.x = Max(a.x, b.x);
	Result.y = Max(a.y, b.y);
	return Result;
}

template<typename T>
inline v3<T>
Max(const v3<T>& a, const v3<T>& b)
{
	v3<T> Result = {};
	Result.x = Max(a.x, b.x);
	Result.y = Max(a.y, b.y);
	Result.z = Max(a.z, b.z);
	return Result;
}

template<typename T>
inline v4<T>
Max(const v4<T>& a, const v4<T>& b)
{
	v4<T> Result = {};
	Result.x = Max(a.x, b.x);
	Result.y = Max(a.y, b.y);
	Result.z = Max(a.z, b.z);
	Result.w = Max(a.w, b.w);
	return Result;
}

inline float
Lerp(float a, float t, float b)
{
    return (1.0f - t) * a + t * b;
}

template<typename T>
inline v2<T>
Lerp(v2<T> a, float t, v2<T> b)
{
    v2<T> Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);

    return Result;
}

template<typename T>
inline v3<T>
Lerp(v3<T> a, float t, v3<T> b)
{
    v3<T> Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);
    Result.z = Lerp(a.z, t, b.z);

    return Result;
}

template<typename T>
inline v4<T>
Lerp(v4<T> a, float t, v4<T> b)
{
    v4<T> Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);
    Result.z = Lerp(a.z, t, b.z);
    Result.w = Lerp(a.w, t, b.w);

    return Result;
}

template<typename T>
inline float
Cross(v2<T> A, v2<T> B)
{
    float Result = A.x * B.y - A.y * B.x;
    return Result;
}

template<typename T>
inline v3<T>
Cross(v3<T> A, v3<T> B)
{
    v3<T> Result = {};

    Result.x = (A.y * B.z - A.z * B.y);
    Result.y = (A.z * B.x - A.x * B.z);
    Result.z = (A.x * B.y - A.y * B.x);

    return Result;
}

using vec2 = v2<float>;
using vec3 = v3<float>;
using vec4 = v4<float>;

using ivec2 = v2<int32_t>;
using ivec3 = v3<int32_t>;
using ivec4 = v4<int32_t>;

using uvec2 = v2<uint32_t>;
using uvec3 = v3<uint32_t>;
using uvec4 = v4<uint32_t>;

inline float 
Dot(const vec2& lhs, const vec2& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y;
}

inline float 
LengthSq(const vec2& v)
{
	return Dot(v, v);
}

inline float 
Length(const vec2& v)
{
	return sqrtf(LengthSq(v));
}

inline vec2
Normalize(vec2& v)
{
	vec2 Result = v / v.Length();
	return Result;
}

inline float 
Dot(const vec3& lhs, const vec3& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline float 
LengthSq(const vec3& v)
{
	return Dot(v, v);
}

inline float 
Length(const vec3& v)
{
	return sqrtf(LengthSq(v));
}

inline vec3
Normalize(vec3& v)
{
	vec3 Result = v / v.Length();
	return Result;
}

inline float 
Dot(const vec4& lhs, const vec4& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

inline float 
LengthSq(const vec4& v)
{
	return Dot(v, v);
}

inline float 
Length(const vec4& v)
{
	return sqrtf(LengthSq(v));
}

inline vec4
Normalize(vec4& v)
{
	vec4 Result = v / v.Length();
	return Result;
}

inline vec2
Min(const vec2& a, const vec2& b)
{
	vec2 Result = {};
	Result.x = Min(a.x, b.x);
	Result.y = Min(a.y, b.y);
	return Result;
}

inline vec3
Min(const vec3& a, const vec3& b)
{
	vec3 Result = {};
	Result.x = Min(a.x, b.x);
	Result.y = Min(a.y, b.y);
	Result.z = Min(a.z, b.z);
	return Result;
}

inline vec4
Min(const vec4& a, const vec4& b)
{
	vec4 Result = {};
	Result.x = Min(a.x, b.x);
	Result.y = Min(a.y, b.y);
	Result.z = Min(a.z, b.z);
	Result.w = Min(a.w, b.w);
	return Result;
}

inline vec2
Max(const vec2& a, const vec2& b)
{
	vec2 Result = {};
	Result.x = Max(a.x, b.x);
	Result.y = Max(a.y, b.y);
	return Result;
}

inline vec3
Max(const vec3& a, const vec3& b)
{
	vec3 Result = {};
	Result.x = Max(a.x, b.x);
	Result.y = Max(a.y, b.y);
	Result.z = Max(a.z, b.z);
	return Result;
}

inline vec4
Max(const vec4& a, const vec4& b)
{
	vec4 Result = {};
	Result.x = Max(a.x, b.x);
	Result.y = Max(a.y, b.y);
	Result.z = Max(a.z, b.z);
	Result.w = Max(a.w, b.w);
	return Result;
}

inline vec2
Lerp(vec2 a, float t, vec2 b)
{
    vec2 Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);

    return Result;
}

inline vec3
Lerp(vec3 a, float t, vec3 b)
{
    vec3 Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);
    Result.z = Lerp(a.z, t, b.z);

    return Result;
}

inline vec4
Lerp(vec4 a, float t, vec4 b)
{
    vec4 Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);
    Result.z = Lerp(a.z, t, b.z);
    Result.w = Lerp(a.w, t, b.w);

    return Result;
}

inline float
Cross(vec2 A, vec2 B)
{
    float Result = A.x * B.y - A.y * B.x;
    return Result;
}

inline vec3
Cross(vec3 A, vec3 B)
{
    vec3 Result = {};

    Result.x = (A.y * B.z - A.z * B.y);
    Result.y = (A.z * B.x - A.x * B.z);
    Result.z = (A.x * B.y - A.y * B.x);

    return Result;
}

union mat3
{
    struct
    {
        float E11, E12, E13;
        float E21, E22, E23;
        float E31, E32, E33;
    };
    struct
    {
        vec3 Line0;
        vec3 Line1;
        vec3 Line2;
    };
    vec3 Lines[3];
    float E[3][3];
    float V[9];

    mat3 operator=(const mat3& rhs)
    {
        this->Line0 = rhs.Line0;
        this->Line1 = rhs.Line1;
        this->Line2 = rhs.Line2;
        return *this;
    }

	static mat3
	Identity()
	{
		mat3 Result =
		{
			1, 0, 0,
			0, 1, 0,
			0, 0, 1,
		};

		return Result;
	}

	mat3 Inverse()
	{
		float Det11 = E22 * E33 - E23 * E32;
		float Det12 = E21 * E33 - E23 * E31;
		float Det13 = E21 * E32 - E22 * E31;
		float Det21 = E12 * E33 - E13 * E32;
		float Det22 = E11 * E33 - E13 * E31;
		float Det23 = E11 * E32 - E12 * E31;
		float Det31 = E12 * E23 - E13 * E22;
		float Det32 = E11 * E23 - E13 * E21;
		float Det33 = E11 * E22 - E12 * E21;

		float Det = E11 * Det11 - E12 * Det12 + E13 * Det13;
		Det = 1.0f / Det;

		mat3 Result = 
		{
			 Det11 * Det, -Det21 * Det,  Det31 * Det,
			-Det12 * Det,  Det22 * Det, -Det32 * Det,
			 Det13 * Det, -Det23 * Det,  Det33 * Det,
		};

		return Result;
	}
};

inline vec3
operator*(const mat3 lhs, const vec3 rhs)
{
    vec3 Result = {};

    Result.x = lhs.E11 * rhs.x + lhs.E21 * rhs.y + lhs.E31 * rhs.z;
    Result.y = lhs.E12 * rhs.x + lhs.E22 * rhs.y + lhs.E32 * rhs.z;
    Result.z = lhs.E13 * rhs.x + lhs.E23 * rhs.y + lhs.E33 * rhs.z;

    return Result;
}

inline mat3 
Inverse(mat3 M)
{
	float Det11 = M.E22 * M.E33 - M.E23 * M.E32;
	float Det12 = M.E21 * M.E33 - M.E23 * M.E31;
	float Det13 = M.E21 * M.E32 - M.E22 * M.E31;

	float Det21 = M.E12 * M.E33 - M.E13 * M.E32;
	float Det22 = M.E11 * M.E33 - M.E13 * M.E31;
	float Det23 = M.E11 * M.E32 - M.E12 * M.E31;

	float Det31 = M.E12 * M.E23 - M.E13 * M.E22;
	float Det32 = M.E11 * M.E23 - M.E13 * M.E21;
	float Det33 = M.E11 * M.E22 - M.E12 * M.E21;

	float Det = M.E11 * Det11 - M.E12 * Det12 + M.E13 * Det13;
	Det = 1.0f / Det;

	mat3 Result = 
	{
		 Det11 * Det, -Det21 * Det,  Det31 * Det,
		-Det12 * Det,  Det22 * Det, -Det32 * Det,
		 Det13 * Det, -Det23 * Det,  Det33 * Det,
	};

	return Result;
}

union alignas(16) mat4
{
    struct
    {
        float E11, E12, E13, E14;
        float E21, E22, E23, E24;
        float E31, E32, E33, E34;
        float E41, E42, E43, E44;
    };
    struct
    {
        vec4 Line0;
        vec4 Line1;
        vec4 Line2;
        vec4 Line3;
    };
    vec4 Lines[4];
    float E[4][4];
    float V[16];
    __m128 I[4];

    mat4 operator=(const mat4& rhs)
    {
        this->Line0 = rhs.Line0;
        this->Line1 = rhs.Line1;
        this->Line2 = rhs.Line2;
        this->Line3 = rhs.Line3;
        return *this;
    }

    mat3 GetMat3()
    {
        mat3 Result =
        {
            E11, E12, E13,
            E21, E22, E23,
            E31, E32, E33,
        };

        return Result;
    }

	static mat4
	Identity()
	{
		mat4 Result =
		{
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1,
		};

		return Result;
	}

	mat4 Inverse()
	{
		float Sub00 = E33 * E44 - E34 * E43;
		float Sub01 = E32 * E44 - E34 * E42;
		float Sub02 = E32 * E43 - E33 * E42;
		float Sub03 = E31 * E44 - E34 * E41;
		float Sub04 = E31 * E43 - E33 * E41;
		float Sub05 = E31 * E42 - E32 * E41;
		float Sub06 = E23 * E44 - E24 * E43;
		float Sub07 = E22 * E44 - E24 * E42;
		float Sub08 = E22 * E43 - E23 * E42;
		float Sub09 = E21 * E44 - E24 * E41;
		float Sub10 = E21 * E43 - E23 * E41;
		float Sub11 = E21 * E42 - E22 * E41;
		float Sub12 = E23 * E34 - E24 * E33;
		float Sub13 = E22 * E34 - E24 * E32;
		float Sub14 = E22 * E33 - E23 * E32;
		float Sub15 = E21 * E34 - E24 * E31;
		float Sub16 = E21 * E33 - E23 * E31;
		float Sub17 = E21 * E32 - E22 * E31;

		float Det11 = E22 * Sub00 - E23 * Sub01 + E24 * Sub02;
		float Det12 = E21 * Sub00 - E23 * Sub03 + E24 * Sub04;
		float Det13 = E21 * Sub01 - E22 * Sub03 + E24 * Sub05;
		float Det14 = E21 * Sub02 - E22 * Sub04 + E23 * Sub05;

		float Det21 = -E12 * Sub00 + E13 * Sub01 - E14 * Sub02;
		float Det22 =  E11 * Sub00 - E13 * Sub03 + E14 * Sub04;
		float Det23 =  E11 * Sub01 - E12 * Sub03 + E14 * Sub05;
		float Det24 =  E11 * Sub02 - E12 * Sub04 + E13 * Sub05;

		float Det31 =  E12 * Sub06 - E13 * Sub07 + E14 * Sub08;
		float Det32 = -E11 * Sub06 + E13 * Sub09 - E14 * Sub10;
		float Det33 =  E11 * Sub07 - E12 * Sub09 + E14 * Sub11;
		float Det34 = -E11 * Sub08 + E12 * Sub10 - E13 * Sub11;

		float Det41 = -E12 * Sub12 + E13 * Sub13 - E14 * Sub14;
		float Det42 =  E11 * Sub12 - E13 * Sub15 + E14 * Sub16;
		float Det43 =  E11 * Sub13 - E12 * Sub15 + E14 * Sub17;
		float Det44 = -E11 * Sub14 + E12 * Sub16 - E13 * Sub17;

		float Det = E11 * Det11 - E12 * Det12 + E13 * Det13 - E14 * Det14;
		Det = 1.0f / Det;

		mat4 Result = 
		{
			Det11 * Det, Det21 * Det, Det31 * Det, Det41 * Det,
			Det12 * Det, Det22 * Det, Det32 * Det, Det42 * Det,
			Det13 * Det, Det23 * Det, Det33 * Det, Det43 * Det,
			Det14 * Det, Det24 * Det, Det34 * Det, Det44 * Det,
		};

		return Result;
	}
};

inline vec4
operator*(const mat4 lhs, const vec4 rhs)
{
    vec4 Result = {};

    Result.x = lhs.E11 * rhs.x + lhs.E21 * rhs.y + lhs.E31 * rhs.z + lhs.E41 * rhs.w;
    Result.y = lhs.E12 * rhs.x + lhs.E22 * rhs.y + lhs.E32 * rhs.z + lhs.E42 * rhs.w;
    Result.z = lhs.E13 * rhs.x + lhs.E23 * rhs.y + lhs.E33 * rhs.z + lhs.E43 * rhs.w;
    Result.w = lhs.E14 * rhs.x + lhs.E24 * rhs.y + lhs.E34 * rhs.z + lhs.E44 * rhs.w;

    return Result;
}

inline mat4 
Inverse(mat4 M)
{
	float Sub00 = M.E33 * M.E44 - M.E34 * M.E43;
	float Sub01 = M.E32 * M.E44 - M.E34 * M.E42;
	float Sub02 = M.E32 * M.E43 - M.E33 * M.E42;
	float Sub03 = M.E31 * M.E44 - M.E34 * M.E41;
	float Sub04 = M.E31 * M.E43 - M.E33 * M.E41;
	float Sub05 = M.E31 * M.E42 - M.E32 * M.E41;
	float Sub06 = M.E23 * M.E44 - M.E24 * M.E43;
	float Sub07 = M.E22 * M.E44 - M.E24 * M.E42;
	float Sub08 = M.E22 * M.E43 - M.E23 * M.E42;
	float Sub09 = M.E21 * M.E44 - M.E24 * M.E41;
	float Sub10 = M.E21 * M.E43 - M.E23 * M.E41;
	float Sub11 = M.E21 * M.E42 - M.E22 * M.E41;
	float Sub12 = M.E23 * M.E34 - M.E24 * M.E33;
	float Sub13 = M.E22 * M.E34 - M.E24 * M.E32;
	float Sub14 = M.E22 * M.E33 - M.E23 * M.E32;
	float Sub15 = M.E21 * M.E34 - M.E24 * M.E31;
	float Sub16 = M.E21 * M.E33 - M.E23 * M.E31;
	float Sub17 = M.E21 * M.E32 - M.E22 * M.E31;

	float Det11 = M.E22 * Sub00 - M.E23 * Sub01 + M.E24 * Sub02;
	float Det12 = M.E21 * Sub00 - M.E23 * Sub03 + M.E24 * Sub04;
	float Det13 = M.E21 * Sub01 - M.E22 * Sub03 + M.E24 * Sub05;
	float Det14 = M.E21 * Sub02 - M.E22 * Sub04 + M.E23 * Sub05;

	float Det21 = M.E12 * Sub00 - M.E13 * Sub01 + M.E14 * Sub02;
	float Det22 = M.E11 * Sub00 - M.E13 * Sub03 + M.E14 * Sub04;
	float Det23 = M.E11 * Sub01 - M.E12 * Sub03 + M.E14 * Sub05;
	float Det24 = M.E11 * Sub02 - M.E12 * Sub04 + M.E13 * Sub05;

	float Det31 = M.E12 * Sub06 - M.E13 * Sub07 + M.E14 * Sub08;
	float Det32 = M.E11 * Sub06 - M.E13 * Sub09 + M.E14 * Sub10;
	float Det33 = M.E11 * Sub07 - M.E12 * Sub09 + M.E14 * Sub11;
	float Det34 = M.E11 * Sub08 - M.E12 * Sub10 + M.E13 * Sub11;

	float Det41 = M.E12 * Sub12 - M.E13 * Sub13 + M.E14 * Sub14;
	float Det42 = M.E11 * Sub12 - M.E13 * Sub15 + M.E14 * Sub16;
	float Det43 = M.E11 * Sub13 - M.E12 * Sub15 + M.E14 * Sub17;
	float Det44 = M.E11 * Sub14 - M.E12 * Sub16 + M.E13 * Sub17;

	float Det = M.E11 * Det11 - M.E12 * Det12 + M.E13 * Det13 - M.E14 * Det14;
	Det = 1.0f / Det;

	mat4 Result = 
	{
		Det11 * Det, -Det21 * Det, Det31 * Det, -Det41 * Det,
		-Det12 * Det, Det22 * Det, -Det32 * Det, Det42 * Det,
		Det13 * Det, -Det23 * Det, Det33 * Det, -Det43 * Det,
		-Det14 * Det, Det24 * Det, -Det34 * Det, Det44 * Det,
	};

	return Result;
}

inline mat4
Scale(float V)
{
    mat4 Result = mat4::Identity();
    Result.E[0][0] = V;
    Result.E[1][1] = V;
    Result.E[2][2] = V;
    return Result;
}

inline mat4
Scale(vec3 V)
{
    mat4 Result = mat4::Identity();
    Result.E[0][0] = V.x;
    Result.E[1][1] = V.y;
    Result.E[2][2] = V.z;
    return Result;
}

inline mat4
RotateX(float A)
{
    float s = sinf(A);
    float c = cosf(A);
    mat4 Result =
    {
        1, 0,  0, 0,
        0, c, -s, 0,
        0, s,  c, 0,
        0, 0,  0, 1,
    };

    return Result;
}

inline mat4
RotateY(float A)
{
    float s = sinf(A);
    float c = cosf(A);
    mat4 Result =
    {
         c, 0, s, 0,
         0, 1, 0, 0,
        -s, 0, c, 0,
         0, 0, 0, 1,
    };

    return Result;
}

inline mat4
RotateZ(float A)
{
    float s = sinf(A);
    float c = cosf(A);
    mat4 Result =
    {
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1,
    };

    return Result;
}

inline mat4
Translate(float V)
{
    mat4 Result = mat4::Identity();
    Result.E[3][0] = V;
    Result.E[3][1] = V;
    Result.E[3][2] = V;
    return Result;
}

inline mat4
Translate(vec3 V)
{
    mat4 Result = mat4::Identity();
    Result.E[3][0] = V.x;
    Result.E[3][1] = V.y;
    Result.E[3][2] = V.z;
    return Result;
}

inline mat4
operator*(const mat4& lhs, const mat4& rhs)
{
    mat4 res = {};

    __m128 v0 = {};
    __m128 v1 = {};
    __m128 v2 = {};
    __m128 v3 = {};

    for(int idx = 0; idx < 4; ++idx)
    {
        v0 = _mm_set1_ps(lhs.V[0+idx*4]);
        v1 = _mm_set1_ps(lhs.V[1+idx*4]);
        v2 = _mm_set1_ps(lhs.V[2+idx*4]);
        v3 = _mm_set1_ps(lhs.V[3+idx*4]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[0], v0, res.I[idx]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[1], v1, res.I[idx]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[2], v2, res.I[idx]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[3], v3, res.I[idx]);
    }

    return res;
}

struct quat
{
    union
	{
		struct 
		{
			vec3 q;
			float w;
		};
		vec4 v;
	};

	quat() = default;

	quat(float x_, float y_, float z_, float w_){w = w_; q.x = x_; q.y = y_; q.z = z_;};
	quat(vec3 q_, float w_){w = w_; q = q_;};
	quat(v4<float> v_){v = v_;};
	quat(float angle, vec3 axis){w = cos(angle / 2), q = axis * vec3(sin(angle/2));};

	quat(const quat& Other){ v = Other.v; };
	quat& operator=(const quat& Other)
    {
		v = Other.v;
        return *this; 
    };

	quat Inverse()
	{
		return quat(-q.x, -q.y, -q.z, w);
	}

	quat Normalize()
	{
		v = v.Normalize();
		return *this;
	};

	mat4 ToRotateMatrix()
	{
		mat4 Result = 
		{
			v.x*v.x - v.y*v.y - v.z*v.z + v.w*v.w, 2*v.x*v.y - 2*v.z*v.w, 2*v.y*v.w + 2*v.x*v.z, 0,
			2*v.z*v.w + 2*v.x*v.y, -v.x*v.x + v.y*v.y - v.z*v.z + v.w*v.w, 2*v.x*v.w - 2*v.x*v.w, 0,
			2*v.x*v.z - 2*v.y*v.w, 2*v.x*v.w + 2*v.y*v.z, -v.x*v.x - v.y*v.y + v.z*v.z + v.w*v.w, 0,
			0, 0, 0, 1
		};
		return Result;
	}
};

quat operator+(quat lhs, quat rhs)
{
	return quat(lhs.q.x + rhs.q.x, lhs.q.y + rhs.q.y, lhs.q.z + rhs.q.z, lhs.w + rhs.w);
}

quat operator*(quat lhs, quat rhs)
{
	return quat(lhs.q * rhs.w + rhs.q * lhs.w + Cross(lhs.q, rhs.q), -lhs.q.Dot(rhs.q) + lhs.w * rhs.w);
}

inline mat4
PerspLH(float Fov, float Width, float Height, float NearZ, float FarZ)
{
    float a = Height / Width;
    float f = 1.0f / tanf(Fov * 0.5f);
    float l = FarZ / (FarZ - NearZ);
    mat4 Result =
    {
        f*a, 0,     0     , 0,
         0,  f,     0     , 0,
         0,  0,     l     , 1,
         0,  0, -l * NearZ, 0,
    };

    return Result;
}

inline mat4
PerspRH(float Fov, float Width, float Height, float NearZ, float FarZ)
{
    float a = Height / Width;
    float f = 1.0f / tanf(Fov * 0.5f);
    float l = FarZ / (NearZ - FarZ);
    mat4 Result =
    {
        f*a, 0,     0    ,  0,
         0,  f,     0    ,  0,
         0,  0,     l    , -1,
         0,  0, l * NearZ,  0,
    };

    return Result;
}

inline mat4
PerspInfFarZ(float Fov, float Width, float Height, float NearZ)
{
    float a = Height / Width;
    float f = 1.0f / tanf(Fov * 0.5f);
    mat4 Result =
    {
        f*a, 0,    0  ,  0,
         0,  f,    0  ,  0,
         0,  0, -  1  , -1,
         0,  0, -NearZ,  0,
    };

	return Result;
}

inline mat4
OrthoLH(float l, float r, float b, float t, float n, float f)
{
	float lr = 1.0f / (r - l);
	float bt = 1.0f / (t - b);
	float nf = 1.0f / (f - n);

	mat4 Result = mat4::Identity();
	Result.E[0][0] =  2.0f * lr;
	Result.E[1][1] =  2.0f * bt;
	Result.E[2][2] =  1.0f * nf;
	Result.E[3][0] = -(l + r) * lr;
	Result.E[3][1] = -(b + t) * bt;
	Result.E[3][2] = -n * nf;

	return Result;
}

inline mat4
OrthoRH(float l, float r, float b, float t, float n, float f)
{
	float lr = 1.0f / (r - l);
	float bt = 1.0f / (t - b);
	float nf = 1.0f / (f - n);

	mat4 Result = mat4::Identity();
	Result.E[0][0] =  2.0f * lr;
	Result.E[1][1] =  2.0f * bt;
	Result.E[2][2] = -1.0f * nf;
	Result.E[3][0] = -(l + r) * lr;
	Result.E[3][1] = -(b + t) * bt;
	Result.E[3][2] = -n * nf;

	return Result;
}

struct plane
{
    vec4 Pos;
    vec4 Norm;
};

inline void
GeneratePlanes(plane* Planes, mat4 Proj, float NearZ, float FarZ = 0)
{
    FarZ = (FarZ < NearZ) ? NearZ : FarZ;
    Planes[0].Pos = vec4(0);
    Planes[0].Norm = vec4(Proj.E14 + Proj.E11, Proj.E24 + Proj.E21, Proj.E34 + Proj.E31, Proj.E44 + Proj.E41).Normalize();

    Planes[1].Pos = vec4(0);
    Planes[1].Norm = vec4(Proj.E14 - Proj.E11, Proj.E24 - Proj.E21, Proj.E34 - Proj.E31, Proj.E44 - Proj.E41).Normalize();

    Planes[2].Pos = vec4(0);
    Planes[2].Norm = vec4(Proj.E14 - Proj.E12, Proj.E24 - Proj.E22, Proj.E34 - Proj.E32, Proj.E44 - Proj.E42).Normalize();

    Planes[3].Pos = vec4(0);
    Planes[3].Norm = vec4(Proj.E14 + Proj.E12, Proj.E24 + Proj.E22, Proj.E34 + Proj.E32, Proj.E44 + Proj.E42).Normalize();

    Planes[4].Pos = vec4(0, 0, NearZ, 0);
    Planes[4].Norm = vec4(Proj.E13, Proj.E23, Proj.E33, Proj.E43).Normalize();

    Planes[5].Pos = vec4(0, 0, FarZ , 0);
    Planes[5].Norm = vec4(Proj.E14 - Proj.E13, Proj.E24 - Proj.E23, Proj.E34 - Proj.E33, Proj.E44 - Proj.E43).Normalize();
}

inline mat4
LookAtLH(vec3 CamPos, vec3 Target, vec3 Up)
{
    vec3 z = (Target - CamPos).Normalize();
    vec3 x = Cross(Up, z).Normalize();
    vec3 y = Cross(z, x);

    mat4 Result =
    {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        -CamPos.Dot(x), -CamPos.Dot(y), -CamPos.Dot(z), 1,
    };

    return Result;
}

inline mat4
LookAtRH(vec3 CamPos, vec3 Target, vec3 Up)
{
    vec3 z = (Target - CamPos).Normalize(); // Forward
    vec3 x = Cross(z, Up).Normalize();		// Right
    vec3 y = Cross(x, z);					// Up

    mat4 Result =
    {
        x.x, y.x, -z.x, 0,
        x.y, y.y, -z.y, 0,
        x.z, y.z, -z.z, 0,
        -CamPos.Dot(x), -CamPos.Dot(y), CamPos.Dot(z), 1,
    };

    return Result;
}

namespace std
{
    // NOTE: got from stack overflow question
    inline void
    hash_combine(size_t& seed, size_t hash)
    {
        hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash;
    }

    template<typename T>
    struct hash<v2<T>>
    {
        size_t operator()(const v2<T>& v) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<T>{}(v.x));
            hash_combine(Result, hash<T>{}(v.y));
            return Result;
        }
    };

    template<typename T>
    struct hash<v3<T>>
    {
        size_t operator()(const v3<T>& v) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<T>{}(v.x));
            hash_combine(Result, hash<T>{}(v.y));
            hash_combine(Result, hash<T>{}(v.z));
            return Result;
        }
    };

    template<typename T>
    struct hash<v4<T>>
    {
        size_t operator()(const v4<T>& v) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<T>{}(v.x));
            hash_combine(Result, hash<T>{}(v.y));
            hash_combine(Result, hash<T>{}(v.z));
            hash_combine(Result, hash<T>{}(v.w));
            return Result;
        }
    };

    template<>
    struct hash<vech2>
    {
        size_t operator()(const v2<uint16_t>& v) const
        {
            size_t ValueToHash = 0;
            size_t Result = 0;

            ValueToHash = (v.x << 16) | (v.y);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            return Result;
        }
    };

    template<>
    struct hash<vech3>
    {
        size_t operator()(const v3<uint16_t>& v) const
        {
            size_t ValueToHash = 0;
            size_t Result = 0;

            ValueToHash = (v.x << 16) | (v.y);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            ValueToHash = v.z;
            hash_combine(Result, hash<uint16_t>()(ValueToHash));
            return Result;
        }
    };

    template<>
    struct hash<vech4>
    {
        size_t operator()(const v4<uint16_t>& v) const
        {
            size_t ValueToHash = 0;
            size_t Result = 0;

            ValueToHash = (v.x << 16) | (v.y);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            ValueToHash = (v.z << 16) | (v.w);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            return Result;
        }
    };
}

inline uint64_t
AlignUp(uint64_t Size, uint64_t Align = 4)
{
    return (Size + (Align - 1)) & ~(Align - 1);
}

inline uint64_t
AlignDown(uint64_t Size, uint64_t Align = 4)
{
    return Size & ~(Align - 1);
}
