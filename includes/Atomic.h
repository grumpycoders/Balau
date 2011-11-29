#pragma once

namespace Balau {

namespace Atomic {

#if (__GNUC__ >= 5) || ((__GNUC__ == 4) && ((__GNUC_MINOR__ >= 1)))
// gcc version of the atomic operations
template <class T> T Or(volatile T * ptr, T mask) { return __sync_or_and_fetch(ptr, mask); }
template <class T> T And(volatile T * ptr, T mask) { return __sync_and_and_fetch(ptr, mask); }
template <class T> T Xor(volatile T * ptr, T mask) { return __sync_xor_and_fetch(ptr, mask); }
template <class T> T Nand(volatile T * ptr, T mask) { return __sync_nand_and_fetch(ptr, mask); }
template <class T> T Increment(volatile T * ptr, T delta = 1) { return __sync_add_and_fetch(ptr, delta); }
template <class T> T Decrement(volatile T * ptr, T delta = 1) { return __sync_sub_and_fetch(ptr, delta); }

namespace Prefetch {
template <class T> T Or(volatile T * ptr, T mask) { return __sync_fetch_and_or(ptr, mask); }
template <class T> T And(volatile T * ptr, T mask) { return __sync_fetch_and_and(ptr, mask); }
template <class T> T Xor(volatile T * ptr, T mask) { return __sync_fetch_and_xor(ptr, mask); }
template <class T> T Nand(volatile T * ptr, T mask) { return __sync_fetch_and_nand(ptr, mask); }
template <class T> T Increment(volatile T * ptr, T delta = 1) { return __sync_fetch_and_add(ptr, delta); }
template <class T> T Decrement(volatile T * ptr, T delta = 1) { return __sync_fetch_and_sub(ptr, delta); }
};

template <class T> T CmpXChgVal(volatile T * ptr, const T xch, const T cmp) { return __sync_val_compare_and_swap(ptr, cmp, xch); }
template <class T> bool CmpXChgBool(volatile T * ptr, const T xch, const T cmp) { return __sync_bool_compare_and_swap(ptr, cmp, xch); }

static inline void MemoryFence() { __sync_synchronize(); }

template <class T> T Exchange32(volatile T * ptr, const T exchange) {
#if defined(i386) || defined (__x86_64)
    __asm__ __volatile__("lock xchgl %0, (%1)" : "+r"(exchange) : "r"(ptr));
    return exchange;
#else
    T p;
    do { p = *ptr; } while (!__sync_bool_compare_and_swap(ptr, p, exchange));
    return p;
#endif
}

template <class T> T Exchange64(volatile T * ptr, const T exchange) {
#if defined(i386) || defined (__x86_64)
    __asm__ __volatile__("lock xchgq %0, (%1)" : "+r"(exchange) : "r"(ptr));
    return exchange;
#else
    T p;
    do { p = *ptr; } while (!__sync_bool_compare_and_swap(ptr, p, exchange));
    return p;
#endif
}

#else
#ifdef _MSVC
// Visual Studio version of the atomic operations

#error MSVC not yet implemented.

#else
#error No known platform for atomic operations.
#endif
#endif

template <class T> T * ExchangePtr(T * volatile * ptr, const T * exchange) {
#if defined (__x86_64)
    return Exchange64(ptr, exchange);
#else
    return Exchange32(ptr, exchange);
#endif
}

};

};
