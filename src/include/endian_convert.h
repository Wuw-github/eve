#pragma once
#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>

#include <type_traits>

namespace sylar
{
    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value);
    }

    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);
    }

    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value);
    }

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
    template <class T>
    T toBigEndian(T t)
    {
        return t;
    }
    template <class T>
    T toLittleEndian(T t)
    {
        return byteswap(t);
    }
#else
    template <class T>
    T toBigEndian(T t)
    {
        return byteswap(t);
    }
    template <class T>
    T toLittleEndian(T t)
    {
        return t;
    }
#endif

} // namespace sylar