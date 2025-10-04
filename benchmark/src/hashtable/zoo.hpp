#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace zoo {
namespace meta {

namespace detail {

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Progression how big the pattern is
/// \tparam Remaining how many more times to copy the pattern
template<typename T, T Current, int CurrentSize, bool>
struct BitmaskMaker_impl {
    static constexpr T value = BitmaskMaker_impl<T, T(Current | (Current << CurrentSize)), CurrentSize * 2,
                                                 CurrentSize * 2 < sizeof(T) * 8>::value;
};

template<typename T, T Current, int CurrentSize>
struct BitmaskMaker_impl<T, Current, CurrentSize, false> {
    static constexpr T value = Current;
};

} // namespace detail

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Current the pattern to broadcast
/// \tparam CurrentSize the number of bits in the pattern
template<typename T, T Current, int CurrentSize>
struct BitmaskMaker {
    static constexpr auto value =
        detail::BitmaskMaker_impl<T, Current, CurrentSize, CurrentSize * 2 <= sizeof(T) * 8>::value;
};

static_assert(0xF0F0 == BitmaskMaker<uint16_t, 0xF0, 8>::value);
static_assert(0xEDFEDFED == BitmaskMaker<uint32_t, 0xFED, 12>::value);

} // namespace meta
} // namespace zoo

namespace zoo {
namespace meta {

namespace detail {

template<int Level, typename T = uint64_t>
struct PopcountMask_impl {
    inline static constexpr auto value = BitmaskMaker<T, (1 << Level) - 1, (1 << (Level + 1))>::value;
};

template<int Bits>
struct UInteger_impl;

template<>
struct UInteger_impl<8> {
    using type = uint8_t;
};

template<>
struct UInteger_impl<16> {
    using type = uint16_t;
};

template<>
struct UInteger_impl<32> {
    using type = uint32_t;
};

template<>
struct UInteger_impl<64> {
    using type = uint64_t;
};

// unfortunately this does not work since unsigned long long is not
// the same type as unsigned long, but may have the same size!
template<typename>
constexpr int BitWidthLog = 0;
template<>
inline constexpr int BitWidthLog<uint8_t> = 3;
template<>
inline constexpr int BitWidthLog<uint16_t> = 4;
template<>
inline constexpr int BitWidthLog<uint32_t> = 5;
template<>
inline constexpr int BitWidthLog<uint64_t> = 6;

} // namespace detail

template<int Bits>
using UInteger = typename detail::UInteger_impl<Bits>::type;

template<int LogarithmOfGroupSize, typename T = std::uint64_t>
struct PopcountLogic {
    static constexpr int GroupSize       = 1 << LogarithmOfGroupSize;
    static constexpr int HalvedGroupSize = GroupSize / 2;
    static constexpr auto CombiningMask  = BitmaskMaker<T, T(1 << HalvedGroupSize) - 1, GroupSize>::value;
    using Recursion                      = PopcountLogic<LogarithmOfGroupSize - 1, T>;
    static constexpr T execute(T input);
};

template<typename T>
struct PopcountLogic<0, T> {
    static constexpr T execute(T input) {
        return input;
    }
};

template<typename T>
struct PopcountLogic<1, T> {
    static constexpr T execute(T input) {
        return input - ((input >> 1) & BitmaskMaker<T, 1, 2>::value);
        // For each pair of bits, the expression results in:
        // 11: 3 - 1: 2
        // 10: 2 - 1: 1
        // 01: 1 - 0: 1
        // 00: 0 - 0: 0
    }
};

template<int LogarithmOfGroupSize, typename T>
constexpr T PopcountLogic<LogarithmOfGroupSize, T>::execute(T input) {
    return Recursion::execute(input & CombiningMask) + Recursion::execute((input >> HalvedGroupSize) & CombiningMask);
}

template<int LogarithmOfGroupSize, typename T = uint64_t>
struct PopcountIntrinsic {
    static constexpr T execute(T input) {
        using UI             = UInteger<1 << LogarithmOfGroupSize>;
        constexpr auto times = 8 * sizeof(T);
        uint64_t rv          = 0;
        for (auto n = times; n;) {
            n -= 8 * sizeof(UI);
            UI tmp = input >> n;
            tmp    = __builtin_popcountll(tmp);
            rv |= uint64_t(tmp) << n;
        }
        return rv;
    }
};

} // namespace meta
} // namespace zoo

namespace zoo {
namespace meta {

constexpr int logFloor(uint64_t arg) {
    return 63 - __builtin_clzll(arg);
}

constexpr int logCeiling(uint64_t arg) {
    auto floorLog = logFloor(arg);
    return floorLog + ((arg ^ (1ull << floorLog)) ? 1 : 0);
}

/// The algorithm is, from the perspective of the most significant bit set, to copy it
/// downward to all positions.
/// First copy it once, meaning a group of two copies of the two most significant bit
/// Then copy it again, making a group of four copies, then 8 copies...
template<typename T>
constexpr int logCeiling_WithoutIntrinsic(T value) {
    constexpr auto NBitsTotal = sizeof(T) * 8;
    for (auto groupSize = 1; groupSize < NBitsTotal; groupSize <<= 1) {
        value |= (value >> groupSize);
    }
    return PopcountLogic<detail::BitWidthLog<T>, T>::execute(value) - 1;
}

} // namespace meta
} // namespace zoo

namespace zoo {
namespace swar {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

/// Index into the bits of the type T that contains the MSB.
template<typename T>
constexpr std::make_unsigned_t<T> msbIndex(T v) noexcept {
    return 8 * sizeof(T) - 1 - __builtin_clzll(v);
}

/// Index into the bits of the type T that contains the LSB.
template<typename T>
constexpr std::make_unsigned_t<T> lsbIndex(T v) noexcept {
    return __builtin_ctzll(v) + 1;
}

/// Core abstraction around SIMD Within A Register (SWAR).  Specifies 'lanes'
/// of NBits width against a type T, and provides an abstraction for performing
/// SIMD operations against that primitive type T treated as a SIMD register.
/// SWAR operations are usually constant time, log(lane count) cost, or O(lane count) cost.
/// Certain computational workloads can be materially sped up using SWAR techniques.
template<int NBits_, typename T = uint64_t>
struct SWAR {
    using type                          = T;
    inline static constexpr auto NBits  = NBits_;
    inline static constexpr auto Lanes  = sizeof(T) * 8 / NBits;
    inline static constexpr auto NSlots = Lanes;
    static constexpr T BitMod           = sizeof(T) * 8 % NBits;
    static constexpr T ValidBitsCount   = sizeof(T) * 8 - BitMod;
    static constexpr T AllOnes          = (BitMod == 0) ? ~(T(0)) : ((T(1) << ValidBitsCount) - 1);

    SWAR() = default;

    constexpr explicit SWAR(T v)
        : m_v(v) {}

    constexpr explicit operator T() const noexcept {
        return m_v;
    }

    constexpr T value() const noexcept {
        return m_v;
    }

    // constexpr SWAR operator~() const noexcept { return SWAR{~m_v}; }

    constexpr SWAR operator~() const noexcept {
        return SWAR(~m_v);
    }

    constexpr SWAR operator&(SWAR o) const noexcept {
        return SWAR(m_v & o.m_v);
    }

    constexpr SWAR operator^(SWAR o) const noexcept {
        return SWAR(m_v ^ o.m_v);
    }

    constexpr SWAR operator|(SWAR o) const noexcept {
        return SWAR(m_v | o.m_v);
    }

    constexpr SWAR operator-(SWAR o) const noexcept {
        return SWAR(m_v - o.m_v);
    }

    constexpr SWAR operator+(SWAR o) const noexcept {
        return SWAR(m_v + o.m_v);
    }

    constexpr SWAR operator*(SWAR o) const noexcept {
        return SWAR(m_v * o.m_v);
    }

    // Returns lane at position with other lanes cleared.
    constexpr T isolateLane(int position) const noexcept {
        constexpr auto filter = (T(1) << NBits) - 1;
        return m_v & (filter << (NBits * position));
    }

    // Returns lane value at position, in lane 0, rest of SWAR cleared.
    constexpr T at(int position) const noexcept {
        constexpr auto filter = (T(1) << NBits) - 1;
        return filter & (m_v >> (NBits * position));
    }

    constexpr SWAR clear(int position) const noexcept {
        constexpr auto filter = (T(1) << NBits) - 1;
        auto invertedMask     = filter << (NBits * position);
        auto mask             = ~invertedMask;
        return SWAR(m_v & mask);
    }

    /// The SWAR lane index that contains the MSB.  It is not the bit index of the MSB.
    /// IE: 4 bit wide 32 bit SWAR: 0x0040'0000 will return 5, not 22 (0 indexed).
    constexpr int top() const noexcept {
        return msbIndex(m_v) / NBits;
    }

    constexpr int lsbIndex() const noexcept {
        return __builtin_ctzll(m_v) / NBits;
    }

    constexpr SWAR setBit(int index, int bit) const noexcept {
        return SWAR(m_v | (T(1) << (index * NBits + bit)));
    }

    constexpr auto blitElement(int index, T value) const noexcept {
        auto elementMask = ((T(1) << NBits) - 1) << (index * NBits);
        return SWAR((m_v & ~elementMask) | (value << (index * NBits)));
    }

    constexpr SWAR blitElement(int index, SWAR other) const noexcept {
        constexpr auto OneElementMask = SWAR(~(~T(0) << NBits));
        auto IsolationMask            = OneElementMask.shiftLanesLeft(index);
        return (*this & ~IsolationMask) | (other & IsolationMask);
    }

    constexpr SWAR shiftLanesLeft(int laneCount) const noexcept {
        return SWAR(value() << (NBits * laneCount));
    }

    constexpr SWAR shiftLanesRight(int laneCount) const noexcept {
        return SWAR(value() >> (NBits * laneCount));
    }

    T m_v;
};

// SWAR is a useful abstraction for performing computations in lanes overlaid
// over any given integral type.
// Doing additions, subtractions, and compares via SWAR techniques requires an
// extra bit per lane be available past the lane size, _or_ knowledge that both
// of your MSBs are set 0 (leaving space for the operation).  Similarly, doing
// multiplications via SWAR techniques require double bits per lane (unless you
// can bind your inputs at half lane size).
// This leads to a useful technique (which we use in the robin hood table)
// where we interleave two related small bit count integers inside of a lane of
// swar.  More generally, this is useful because it sometimes allows fast
// operations on side "a" of some lane if side "b" is blitted out, and vice
// versa.  In the spirit of separation of concerns, we provide a cut-lane-SWAR
// abstraction here.

template<int NBitsLeast_, int NBitsMost_, typename T = uint64_t>
struct SWARWithSubLanes : SWAR<NBitsLeast_ + NBitsMost_, T> {
    inline static constexpr auto NBitsLeast = NBitsLeast_;
    inline static constexpr auto NBitsMost  = NBitsMost_;

    using Base                             = SWAR<NBitsMost + NBitsLeast, T>;
    inline static constexpr auto Available = sizeof(T);
    inline static constexpr auto LaneBits  = NBitsLeast + NBitsMost;

    using Base::Base;

    constexpr SWARWithSubLanes(Base b) noexcept
        : Base(b) {}

    constexpr SWARWithSubLanes(T most, T least) noexcept
        : Base((most << NBitsLeast) | least) {}

    // M is most significant bits slice, L is least significant bits slice.
    // 0x....M2L2M1L1 or MN|LN||...||M2|L2||M1|L1
    using SL = SWARWithSubLanes<NBitsLeast, NBitsMost, T>;

    inline static constexpr auto LeastOnes = Base(meta::BitmaskMaker<T, Base{1}.value(), LaneBits>::value);
    inline static constexpr auto MostOnes  = Base(LeastOnes.value() << NBitsLeast);
    inline static constexpr auto LeastMask = MostOnes - LeastOnes;
    inline static constexpr auto MostMask  = ~LeastMask;

    constexpr auto least() const noexcept {
        return SL{LeastMask & *this};
    }

    // Isolate the least significant bits of the lane at the specified position.
    constexpr auto least(int pos) const noexcept {
        constexpr auto Filter = SL((T(1) << NBitsLeast) - 1);
        return Filter.shiftLanesLeft(pos) & *this;
    }

    // Returns only the least significant bits at specified position, 'decoded' to their integer value.
    constexpr auto leastFlat(int pos) const noexcept {
        return least().at(pos);
    }

    constexpr auto most() const noexcept {
        return SL{MostMask & *this};
    }

    // The most significant bits of the lane at the specified position.
    constexpr auto most(int pos) const noexcept {
        constexpr auto Filter = SL(((T(1) << SL::NBitsMost) - 1) << SL::NBitsLeast);
        return Filter.shiftLanesLeft(pos) & *this;
    }

    // The most significant bits of the lane at the specified position,
    // 'decoded' to their integer value.
    constexpr auto mostFlat(int pos) const noexcept {
        return most().at(pos) >> SL::NBitsLeast;
    }

    // Sets the lsb sublane at |pos| with least significant NBitsLeast of |in|
    constexpr auto least(T in, int pos) const noexcept {
        constexpr auto filter  = (T(1) << LaneBits) - 1;
        const auto keep        = ~(filter << (LaneBits * pos)) | MostMask.value();
        const auto rdyToInsert = this->m_v & keep;
        const auto rval        = rdyToInsert | ((in & LeastMask.value()) << (LaneBits * pos));
        return SL(rval);
    }

    // Sets the msb sublane at |pos| with least significant NBitsMost of |in|
    constexpr auto most(T in, int pos) const noexcept {
        constexpr auto filter  = (T(1) << LaneBits) - 1;
        const auto keep        = ~(filter << (LaneBits * pos)) | LeastMask.value();
        const auto rdyToInsert = this->m_v & keep;
        const auto insVal      = (((in << NBitsLeast) & MostMask.value()) << (LaneBits * pos));
        const auto rval        = rdyToInsert | insVal;
        return SL(rval);
    }
};

/// Defining operator== on base SWAR types is entirely too error prone. Force a verbose invocation.
template<int NBits, typename T = uint64_t>
constexpr auto horizontalEquality(SWAR<NBits, T> left, SWAR<NBits, T> right) {
    return left.m_v == right.m_v;
}

/// Isolating >= NBits in underlying integer type currently results in disaster.
// TODO(scottbruceheart) Attempting to use binary not (~) results in negative shift warnings.
template<int NBits, typename T = uint64_t>
constexpr auto isolate(T pattern) {
    return pattern & ((T(1) << NBits) - 1);
}

/// Clears the least bit set in type T
template<typename T = uint64_t>
constexpr auto clearLSB(T v) {
    return v & (v - 1);
}

/// Leaves on the least bit set, or all 1s for a 0 input.
template<typename T = uint64_t>
constexpr auto isolateLSB(T v) {
    return v & ~clearLSB(v);
}

template<int NBits, typename T>
constexpr T leastNBitsMask() {
    constexpr auto type_bits = sizeof(T) * 8;

    if constexpr (NBits == 0) {
        return T{0};
    } else if constexpr (NBits < type_bits) {
        using UnsignedT = std::make_unsigned_t<T>;
        return static_cast<T>((UnsignedT{1} << NBits) - 1);
    } else {
        return ~T{0};
    }
}

template<int NBits, typename T = uint64_t>
constexpr T mostNBitsMask() {
    return ~leastNBitsMask<sizeof(T) * 8 - NBits, T>();
}

/// Clears the block of N bits anchored at the LSB.
/// clearLSBits<3> applied to binary 00111100 is binary 00100000
template<int NBits, typename T = uint64_t>
constexpr auto clearLSBits(T v) {
    constexpr auto lowMask = leastNBitsMask<NBits, T>();
    return v & (~(lowMask << meta::logFloor(isolateLSB<T>(v))));
}

/// Isolates the block of N bits anchored at the LSB.
/// isolateLSBits<2> applied to binary 00111100 is binary 00001100
template<int NBits, typename T = uint64_t>
constexpr auto isolateLSBits(T v) {
    constexpr auto lowMask = leastNBitsMask<NBits, T>();
    return v & (lowMask << meta::logFloor(isolateLSB<T>(v)));
}

/// Broadcasts the value in the 0th lane of the SWAR to the entire SWAR.
/// Precondition: 0th lane of |v| contains a value to broadcast, remainder of input SWAR zero.
template<int NBits, typename T = uint64_t>
constexpr auto broadcast(SWAR<NBits, T> v) {
    constexpr T Ones = meta::BitmaskMaker<T, 1, NBits>::value;
    return SWAR<NBits, T>(T(v) * Ones);
}

/// BooleanSWAR treats the MSB of each SWAR lane as the boolean associated with that lane.
template<int NBits, typename T>
struct BooleanSWAR : SWAR<NBits, T> {
    // Booleanness is stored in MSB of a given swar.
    static constexpr auto MaskLaneMSB = broadcast<NBits, T>(SWAR<NBits, T>(T(1) << (NBits - 1)));

    constexpr explicit BooleanSWAR(T v)
        : SWAR<NBits, T>(v) {}

    constexpr BooleanSWAR clear(int bit) const noexcept {
        constexpr auto Bit = T(1) << (NBits - 1);
        return this->m_v ^ (Bit << (NBits * bit));
    }

    constexpr BooleanSWAR clearLSB() const noexcept {
        return BooleanSWAR(swar::clearLSB(this->value()));
    }

    /// BooleanSWAR treats the MSB of each lane as the boolean associated with that lane.
    /// A logical NOT in this circumstance _only_ flips the MSB of each lane.  This operation is
    /// not ones or twos complement.
    constexpr auto operator not() const noexcept {
        return BooleanSWAR(MaskLaneMSB ^ *this);
    }

    explicit constexpr operator bool() const noexcept {
        return this->m_v;
    }

private:
    constexpr BooleanSWAR(SWAR<NBits, T> initializer) noexcept
        : SWAR<NBits, T>(initializer) {}

    template<int N, int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT> constantIsGreaterEqual(SWAR<NB, TT>) noexcept;

    template<int N, int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT> constantIsGreaterEqual_MSB_off(SWAR<NB, TT>) noexcept;

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT> greaterEqual(SWAR<NB, TT>, SWAR<NB, TT>) noexcept;

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT> greaterEqual_MSB_off(SWAR<NB, TT>, SWAR<NB, TT>) noexcept;
};

template<int N, int NBits, typename T>
constexpr BooleanSWAR<NBits, T> constantIsGreaterEqual(SWAR<NBits, T> subtrahend) noexcept {
    static_assert(1 < NBits, "Degenerated SWAR");
    constexpr auto MSB_Position = NBits - 1;
    constexpr auto MSB          = T(1) << MSB_Position;
    constexpr auto MSB_Mask     = SWAR<NBits, T>{meta::BitmaskMaker<T, MSB, NBits>::value};
    constexpr auto Minuend      = SWAR<NBits, T>{meta::BitmaskMaker<T, N, NBits>::value};
    constexpr auto N_MSB        = MSB & N;

    auto subtrahendWithMSB_on     = MSB_Mask & subtrahend;
    auto subtrahendWithMSB_off    = ~subtrahendWithMSB_on;
    auto subtrahendMSBs_turnedOff = subtrahend ^ subtrahendWithMSB_on;
    if constexpr (N_MSB) {
        auto leastSignificantComparison = Minuend - subtrahendMSBs_turnedOff;
        auto merged                     = subtrahendMSBs_turnedOff | // the minuend MSBs are on
                      leastSignificantComparison;
        return MSB_Mask & merged;
    } else {
        auto minuendWithMSBs_turnedOn   = Minuend | MSB_Mask;
        auto leastSignificantComparison = minuendWithMSBs_turnedOn - subtrahendMSBs_turnedOff;
        auto merged                     = subtrahendWithMSB_off & // the minuend MSBs are off
                      leastSignificantComparison;
        return MSB_Mask & merged;
    }
}

template<int N, int NBits, typename T>
constexpr BooleanSWAR<NBits, T> constantIsGreaterEqual_MSB_off(SWAR<NBits, T> subtrahend) noexcept {
    static_assert(1 < NBits, "Degenerated SWAR");
    constexpr auto MSB_Position = NBits - 1;
    constexpr auto MSB          = T(1) << MSB_Position;
    constexpr auto MSB_Mask     = meta::BitmaskMaker<T, MSB, NBits>::value;
    constexpr auto Minuend      = meta::BitmaskMaker<T, N, NBits>::value;
    constexpr auto N_MSB        = MSB & Minuend;

    auto subtrahendWithMSB_on     = subtrahend;
    auto subtrahendWithMSB_off    = ~subtrahendWithMSB_on;
    auto subtrahendMSBs_turnedOff = subtrahend ^ subtrahendWithMSB_on;
    if constexpr (N_MSB) {
        return MSB_Mask;
    } else {
        auto minuendWithMSBs_turnedOn   = Minuend | MSB_Mask;
        auto leastSignificantComparison = minuendWithMSBs_turnedOn - subtrahendMSBs_turnedOff;
        return MSB_Mask & leastSignificantComparison;
    }
}

template<int NBits, typename T>
constexpr BooleanSWAR<NBits, T> greaterEqual_MSB_off(SWAR<NBits, T> left, SWAR<NBits, T> right) noexcept {
    constexpr auto MLMSB = BooleanSWAR<NBits, T>::MaskLaneMSB;
    auto minuend         = MLMSB | left;
    return MLMSB & (minuend - right);
}

template<int NB, typename T>
constexpr auto booleans(SWAR<NB, T> arg) noexcept {
    return not constantIsGreaterEqual<0>(arg);
}

template<int NBits, typename T>
constexpr auto differents(SWAR<NBits, T> a1, SWAR<NBits, T> a2) {
    return booleans(a1 ^ a2);
}

template<int NBits, typename T>
constexpr auto equals(SWAR<NBits, T> a1, SWAR<NBits, T> a2) {
    return not differents(a1, a2);
}

/*
This is just a draft implementation:
b1. The isolator needs pre-computing instead of adding 3 ops per iteration
2. The update of the isolator is not needed in the last iteration
3. Consider returning not the logarithm, but the biased by 1 (to support 0)
 */
template<int NBits, typename T>
constexpr SWAR<NBits, T> logarithmFloor(SWAR<NBits, T> v) noexcept {
    constexpr auto LogNBits = meta::logFloor(NBits);
    static_assert(NBits == (1 << LogNBits), "Logarithms of element width not power of two is un-implemented");
    auto whole         = v.value();
    auto isolationMask = BooleanSWAR<NBits, T>::MaskLaneMSB.value();
    for (auto groupSize = 1; groupSize < NBits; groupSize <<= 1) {
        auto shifted = whole >> groupSize;

        // When shifting down a group to double the size of a group, the upper
        // "groupSize" bits will come from the element above, mask them out
        auto isolator        = ~isolationMask;
        auto withoutCrossing = shifted & isolator;
        whole |= withoutCrossing;
        isolationMask |= (isolationMask >> groupSize);
    }
    constexpr auto ones = meta::BitmaskMaker<T, 1, NBits>::value;
    auto popcounts      = meta::PopcountLogic<LogNBits, T>::execute(whole);
    return SWAR<NBits, T>{popcounts - ones};
}

static_assert(logarithmFloor(SWAR<8>{0x8040201008040201ull}).value() == 0x0706050403020100ull);
static_assert(logarithmFloor(SWAR<8>{0xFF7F3F1F0F070301ull}).value() == 0x0706050403020100ull);

} // namespace swar
} // namespace zoo

namespace zoo {

template<typename T>
struct GeneratorFromPointer {
    T* p_;

    constexpr T& operator*() noexcept {
        return *p_;
    }

    constexpr GeneratorFromPointer operator++() noexcept {
        return {++p_};
    }
};

template<typename T, int MisalignmentBits>
struct MisalignedGenerator {
    T* base_;
    static constexpr auto Width = sizeof(T) * 8;

    constexpr T operator*() noexcept {
        auto firstPart  = base_[0];
        auto secondPart = base_[1];
        // how to make sure the "logical" shift right is used, regardless
        // of the signedness of T?
        // I'd prefer to not use std::make_unsigned_t, since how do we
        // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> MisalignmentBits;
        auto secondPartRaised = secondPart.value() << (Width - MisalignmentBits);
        return T{firstPartLowered | secondPartRaised};
    }

    constexpr MisalignedGenerator operator++() noexcept {
        return {++base_};
    }
};

template<typename T>
struct MisalignedGenerator<T, 0> : GeneratorFromPointer<T> {};

// This is tightly coupled with a Metadata that happens-to have lane widths of
// 8.
template<typename T>
struct MisalignedGenerator_Dynamic {
    static constexpr auto Width = sizeof(T) * 8;
    T* base_;

    int misalignmentFirst, misalignmentSecondLessOne;

    MisalignedGenerator_Dynamic(T* base, int ma)
        : base_(base), misalignmentFirst(ma), misalignmentSecondLessOne(Width - ma - 1) {}

    constexpr T operator*() noexcept {
        auto firstPart  = base_[0];
        auto secondPart = base_[1];
        // how to make sure the "logical" shift right is used, regardless
        // of the signedness of T?
        // I'd prefer to not use std::make_unsigned_t, since how do we
        // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> misalignmentFirst;
        // Avoid undefined behavior of << width of type.
        auto secondPartRaised = (secondPart.value() << misalignmentSecondLessOne) << 1;
        return T{firstPartLowered | secondPartRaised};
    }

    constexpr MisalignedGenerator_Dynamic operator++() noexcept {
        ++base_;
        return *this;
    }
};

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

namespace impl {

/// \todo decide on whether to rename this?
template<int PSL_Bits, int HashBits, typename U>
struct Metadata : swar::SWARWithSubLanes<PSL_Bits, HashBits, U> {
    using Base = swar::SWARWithSubLanes<PSL_Bits, HashBits, U>;
    using Base::Base;

    constexpr auto PSLs() const noexcept {
        return this->least();
    }

    constexpr auto hashes() const noexcept {
        return this->most();
    }
};

template<int PSL_Bits, int HashBits, typename U>
struct MatchResult {
    U deadline;
    Metadata<PSL_Bits, HashBits, U> potentialMatches;
};

} // namespace impl

template<int NBits, typename U>
constexpr auto hashReducer(U n) noexcept {
    constexpr auto Shift   = (NBits * ((sizeof(U) * 8 / NBits) - 1));
    constexpr auto AllOnes = meta::BitmaskMaker<U, 1, NBits>::value;
    auto temporary         = AllOnes * n;
    auto higestNBits       = temporary >> Shift;
    return (0 == (64 % NBits)) ? higestNBits : swar::isolate<NBits>(higestNBits);
}

template<typename T>
constexpr auto fibonacciIndexModulo(T index) {
    constexpr std::array<uint64_t, 4> GoldenRatioReciprocals = {
        159,
        40503,
        2654435769,
        11400714819323198485ull,
    };
    constexpr T MagicalConstant = T(GoldenRatioReciprocals[meta::logFloor(sizeof(T))]);
    return index * MagicalConstant;
}

template<size_t Size, typename T>
constexpr auto lemireModuloReductionAlternative(T input) noexcept {
    static_assert(sizeof(T) == sizeof(uint64_t));
    constexpr T MiddleBit = 1ull << 32;
    static_assert(Size < MiddleBit);
    auto lowerHalf = input & (MiddleBit - 1);
    return Size * lowerHalf >> 32;
}

// Scatters a range onto itself
template<typename T>
struct FibonacciScatter {
    constexpr auto operator()(T index) noexcept {
        return fibonacciIndexModulo(index);
    }
};

// Reduces an int onto a range via Lemire reduction.
template<size_t Size, typename T>
struct LemireReduce {
    constexpr auto operator()(T input) noexcept {
        return lemireModuloReductionAlternative<Size, T>(input);
    }
};

// Reduces an input value of U to NBits width, via ones multiply and top bits.
template<int NBits, typename U>
struct TopHashReducer {
    constexpr auto operator()(U n) noexcept {
        return hashReducer<NBits>(n);
    }
};

/// Given a key and sufficient templates specifying its transformation process,
/// return hoisted hash bits and home index in table.
template<typename K, size_t RequestedSize, int HashBits, typename U = std::uint64_t, typename Hash = std::hash<K>,
         typename Scatter = FibonacciScatter<U>, typename RangeReduce = LemireReduce<RequestedSize, U>,
         typename HashReduce = TopHashReducer<HashBits, U>>
static constexpr auto findBasicParameters(const K& k) noexcept {
    auto hashCode  = Hash{}(k);
    auto scattered = Scatter{}(hashCode);
    auto homeIndex = RangeReduce{}(scattered);
    auto hoisted   = HashReduce{}(hashCode);
    return std::tuple{hoisted, homeIndex};
}

template<int NBits>
constexpr auto cheapOkHash(u64 n) noexcept {
    constexpr auto shift  = (NBits * ((64 / NBits) - 1));
    constexpr u64 allOnes = meta::BitmaskMaker<u64, 1, NBits>::value;
    auto temporary        = allOnes * n;
    auto higestNBits      = temporary >> shift;
    return (0 == (64 % NBits)) ? higestNBits : swar::isolate<NBits, u64>(higestNBits);
}

/// Does some multiplies with a lot of hash bits to mix bits, returns only a few
/// of them.
template<int NBits>
auto badMixer(u64 h) noexcept {
    constexpr u64 allOnes      = ~0ull;
    constexpr u64 mostSigNBits = swar::mostNBitsMask<NBits, u64>();
    auto tmp                   = h * allOnes;

    auto mostSigBits = tmp & mostSigNBits;
    return mostSigBits >> (64 - NBits);
}

/// Evenly map a large int to an int without division or modulo.
template<int SizeTable, typename T>
constexpr int mapToSlotLemireReduction(T halved) {
    // TODO: E and S think that the upper bits of are higher quality entropy,
    // explore at some point.
    return (halved * T(SizeTable)) >> (sizeof(T) / 2);
}

namespace impl {

template<typename MetadataCollection>
auto peek(const MetadataCollection& collection, size_t index) {
    using MD        = std::remove_const_t<std::remove_reference_t<decltype(collection[0])>>;
    auto swarIndex  = index / MD::NSlots;
    auto intraIndex = index % MD::NSlots;
    auto swar       = collection[swarIndex];
    return std::tuple{swar.leastFlat(intraIndex), swar.mostFlat(intraIndex)};
}

template<typename MetadataCollection>
void poke(MetadataCollection& collection, size_t index, u64 psl, u64 hash) {
    using MD              = std::remove_reference_t<decltype(collection[0])>;
    auto swarIndex        = index / MD::NSlots;
    auto intraIndex       = index % MD::NSlots;
    auto swar             = collection[swarIndex];
    auto newPSL           = swar.least(psl, intraIndex);
    auto replacement      = newPSL.most(hash, intraIndex);
    collection[swarIndex] = replacement;
}

} // namespace impl
} // namespace rh
} // namespace zoo

namespace zoo {
namespace meta {

template<typename TypeToModel, typename ValueCategoryAndConstnessReferenceType>
struct copy_cr;

template<typename Type, typename LValue>
struct copy_cr<Type, LValue&> {
    using type = std::conditional_t<std::is_const_v<LValue>, const Type&, Type&>;
};

template<typename Type, typename RValue>
struct copy_cr<Type, RValue&&> {
    using type = std::conditional_t<std::is_const_v<RValue>, const Type&&, Type&&>;
};

template<typename Type, typename Reference>
using copy_cr_t = typename copy_cr<Type, Reference>::type;

template<typename T>
using remove_cr_t = std::remove_const_t<std::remove_reference_t<T>>;
} // namespace meta
} // namespace zoo

namespace zoo {

constexpr auto VPSize = sizeof(void*), VPAlignment = alignof(void*);

namespace impl {

template<typename, typename = void>
struct Arraish_impl : std::false_type {};

template<typename T>
struct Arraish_impl<T, decltype(&std::declval<T>()[0])> : std::true_type {};

template<typename T, typename... Args>
struct Constructible : std::is_constructible<T, Args...> {};

template<typename T, std::size_t L, typename Source>
struct Constructible<T[L], Source>
    : Constructible<T, meta::copy_cr_t<meta::remove_cr_t<decltype(std::declval<Source>()[0])&&>, Source>> {};

template<typename T, typename... Args>
constexpr auto Constructible_v = Constructible<T, Args&&...>::value;

template<typename T>
void destroy(T& t) noexcept {
    t.~T();
}

template<typename T, std::size_t L>
void destroy(T (&a)[L]) noexcept {
    auto base = &a[0], lifo = base + L;
    while (base != lifo--) {
        destroy(*lifo);
    }
}

template<typename T, typename... Args>
void build(T&, Args&&...) noexcept(std::is_nothrow_constructible_v<T, Args...>);

template<typename T, std::size_t L, typename ArrayLike>
auto build(T (&destination)[L], ArrayLike&& a) noexcept(
    std::is_nothrow_constructible_v<T,

                                    meta::copy_cr_t<meta::remove_cr_t<decltype(a[0])>, ArrayLike&&>&&>) {
    using Source = meta::copy_cr_t<meta::remove_cr_t<decltype(a[0])>, ArrayLike&&>&&;

    constexpr auto Noexcept = std::is_nothrow_constructible_v<T, Source>;
    auto base = &destination[0], to = base, top = to + L;
    auto from      = &a[0];
    auto transport = [&]() {
        while (top != to) {
            build(*to, static_cast<Source>(*from));
            ++to;
            ++from;
        }
    };
    if constexpr (Noexcept) {
        transport();
    } else {
        try {
            transport();
        } catch (...) {
            while (to-- != base) {
                destroy(*to);
            }
            throw;
        }
    }
}

template<typename T, typename... Args>
void build(T& to, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    new (&to) T(std::forward<Args>(args)...);
}

} // namespace impl

///! \note What about constructors and destructor?
template<int S = VPSize, int A = VPAlignment>
struct AlignedStorage {
    static constexpr auto Size = S, Alignment = A;

    alignas(A) char space_[Size];

    template<typename T>
    T* as() noexcept {
        return reinterpret_cast<T*>(&space_);
    }

    template<typename T>
    const T* as() const noexcept {
        return const_cast<AlignedStorage*>(this)->as<T>();
    }

    template<typename T>
    static constexpr auto SuitableType() {
        return sizeof(T) <= S && 0 == A % alignof(T);
    }

    template<typename T, typename... Args>

    auto build(Args&&... args) noexcept(noexcept(impl::build(*as<T>(), std::forward<Args>(args)...)))
        -> std::enable_if_t<SuitableType<T>() && impl::Constructible_v<T, Args...>, T*> {
        impl::build(*as<T>(), std::forward<Args>(args)...);

        return as<T>();
    }

    template<typename T>
    auto destroy() noexcept -> std::enable_if_t<SuitableType<T>() && std::is_nothrow_destructible_v<T>> {
        impl::destroy(*as<T>());
    }
};

template<typename V>
using AlignedStorageFor = AlignedStorage<sizeof(V), alignof(V)>;

} // namespace zoo

namespace zoo {
namespace rh {

struct RobinHoodException : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct MaximumProbeSequenceLengthExceeded : RobinHoodException {
    using RobinHoodException::RobinHoodException;
};

struct RelocationStackExhausted : RobinHoodException {
    using RobinHoodException::RobinHoodException;
};

template<int PSL_Bits, int HashBits, typename U = std::uint64_t>
struct RH_Backend {
    using Metadata = impl::Metadata<PSL_Bits, HashBits, U>;

    inline static constexpr auto Width = Metadata::NBits;
    Metadata* md_;

    /*! \brief SWAR check for a potential match
    The invariant in Robin Hood is that the element being looked for, the "needle", is "richer"
    than the elements already present, the "haystack".
    "Richer" means that the PSL is smaller.
    A PSL of 0 can only happen in the haystack, to indicate the slot is empty, this is "richest".
    The first time the needle has a PSL greater than the haystacks' means the matching will fail,
    because the hypothetical prior insertion would have "stolen" that slot.
    If there is an equal, it would start a sequence of potential matches.  To determine an actual match:
    1. A cheap SWAR check of hoisted hashes
    2. If there are still potential matches (now also the hoisted hashes), fall back to non-SWAR,
    or iterative and expensive "deep equality" test for each potential match, outside of this function

    The above makes it very important to detect the first case in which the PSL is greater equal to the needle.
    We call this the "deadline".
    Because we assume the LITTLE ENDIAN byte ordering, the first element would be the least significant
    non-false Boolean SWAR.

    Note about performance:
    Every "early exit" faces a big justification hurdle, the proportion of cases
    they intercept to be large enough that the branch prediction penalty of the entropy introduced is
    overcompensated.
    */

    /// Boolean SWAR true in the first element/lane of the needle strictly poorer than its corresponding
    /// haystack
    static constexpr auto firstInvariantBreakage(Metadata needle, Metadata haystack) {
        auto nPSL              = needle.PSLs();
        auto hPSL              = haystack.PSLs();
        auto theyKeepInvariant = greaterEqual_MSB_off(hPSL, nPSL);
        // BTW, the reason to have encoded the PSLs in the least
        // significant bits is to be able to call the cheaper version
        // _MSB_off here

        auto theyBreakInvariant = not theyKeepInvariant;
        // because we make the assumption of LITTLE ENDIAN byte ordering,
        // we're interested in the elements up to the first haystack-richer
        auto firstBreakage = swar::isolateLSB(theyBreakInvariant.value());
        return firstBreakage;
    }

    static constexpr impl::MatchResult<PSL_Bits, HashBits, U> potentialMatches(Metadata needle,
                                                                               Metadata haystack) noexcept {
        // We need to determine if there are potential matches to consider
        auto sames    = equals(needle, haystack);
        auto deadline = firstInvariantBreakage(needle, haystack);
        // In a valid haystack, the PSLs can grow at most by 1 per entry.
        // If a PSL is richer than the needle in any place, because the
        // needle, by construction, always grows at least by 1 per entry,
        // then the PSL won't be equal again.
        // There is no need to filter potential matches using the deadline
        // as previous versions of the code did.
        return {deadline, sames};
    }

    /*! \brief converts the given starting PSL and reduced hash code into a SWAR-ready needle

    The given needle would have a PSL as the starting (PSL + 1) in the first slot, the "+ 1" is because
    the count starts at 1, in this way, a haystack PSL of 0 is always "richer"
    */
    static constexpr auto makeNeedle(U startingPSL, U hoistedHash) {
        constexpr auto Ones               = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression        = Ones * Ones;
        auto core                         = startingPSL | (hoistedHash << PSL_Bits);
        auto broadcasted                  = broadcast(Metadata(core));
        auto startingPSLmadePotentialPSLs = Metadata(Progression) + broadcasted;
        return startingPSLmadePotentialPSLs;
    }

    template<typename KeyComparer>
    inline constexpr std::tuple<std::size_t, U, Metadata>
    findMisaligned_assumesSkarupkeTail(U hoistedHash, int homeIndex, const KeyComparer& kc) const noexcept
        __attribute__((always_inline));
};

template<int PSL_Bits, int HashBits, typename U>
template<typename KeyComparer>
inline constexpr std::tuple<std::size_t, U, typename RH_Backend<PSL_Bits, HashBits, U>::Metadata>
RH_Backend<PSL_Bits, HashBits, U>::findMisaligned_assumesSkarupkeTail(U hoistedHash, int homeIndex,
                                                                      const KeyComparer& kc) const noexcept {
    auto misalignment = homeIndex % Metadata::NSlots;
    auto baseIndex    = homeIndex / Metadata::NSlots;
    auto base         = this->md_ + baseIndex;

    // constexpr auto Ones        = meta::BitmaskMaker<U, 1, Width>::value;
    // constexpr auto Progression = Metadata{Ones * Ones};
    constexpr auto AllNSlots = Metadata{meta::BitmaskMaker<U, Metadata::NSlots, Width>::value};
    MisalignedGenerator_Dynamic<Metadata> p(base, int(Metadata::NBits * misalignment));
    auto index  = homeIndex;
    auto needle = makeNeedle(0, hoistedHash);

    for (;;) {
        auto hay       = *p;
        auto result    = potentialMatches(needle, hay);
        auto positives = result.potentialMatches;
        while (positives.value()) {
            auto matchSubIndex = positives.lsbIndex();
            auto matchIndex    = index + matchSubIndex;
            // Possible specialist optimization to kick off all possible
            // matches to an array (like chaining evict) and check them
            // later.
            if (kc(matchIndex)) {
                return std::tuple(matchIndex, U(0), Metadata(0));
            }
            positives = Metadata{swar::clearLSB(positives.value())};
        }
        auto deadline = result.deadline;
        if (deadline) {
            // The deadline is relative to the misalignment.
            // To build an absolute deadline, there are two cases:
            // the bit falls in the first SWAR or the second SWAR.
            // The same applies for needle.
            // in general, for example a misaglignment of 6:
            // { . | . | . | . | . | . | . | .}{ . | . | . | . | . | . | . | . }
            //                         { a | b | c | d | e | f | g | h }
            // shift left (to higher bits) by the misalignment
            // { 0 | 0 | 0 | 0 | 0 | 0 | a | b }
            // shift right (to lower bits) by NSlots - misalignment:
            // { c | d | e | f | g | h | 0 | 0 }
            // One might hope undefined behavior might be reasonable (zero
            // result, unchanged result), but ARM proves that undefined
            // behavior is indeed undefined, so we do our right shift as a
            // double: shift by n-1, then shift by 1.
            auto mdd        = Metadata{deadline};
            auto toAbsolute = [](auto v, auto ma) {
                auto shiftedLeft  = v.shiftLanesLeft(ma);
                auto shiftedRight = v.shiftLanesRight(Metadata::NSlots - ma - 1).shiftLanesRight(1);
                return Metadata{shiftedLeft | shiftedRight};
            };
            auto position = index + Metadata{deadline}.lsbIndex();
            return std::tuple(position, toAbsolute(mdd, misalignment).value(), toAbsolute(needle, misalignment));
        }
        // Skarupke's tail allows us to not have to worry about the end
        // of the metadata
        ++p;
        index += Metadata::NSlots;
        needle = needle + AllNSlots;
    }
}

template<typename K, typename MV>
struct KeyValuePairWrapper {
    using type = std::pair<K, MV>;
    AlignedStorageFor<type> pair_;

    template<typename... Initializers>
    void build(Initializers&&... izers) noexcept(
        noexcept(pair_.template build<type>(std::forward<Initializers>(izers)...))) {
        pair_.template build<type>(std::forward<Initializers>(izers)...);
    }

    template<typename RHS>
    KeyValuePairWrapper& operator=(RHS&& rhs) noexcept(noexcept(std::declval<type&>() = std::forward<RHS>(rhs))) {
        *pair_.template as<type>() = std::forward<RHS>(rhs);
        return *this;
    }

    void destroy() noexcept {
        pair_.template destroy<type>();
    }

    auto& value() noexcept {
        return *this->pair_.template as<type>();
    }

    const auto& value() const noexcept {
        return const_cast<KeyValuePairWrapper*>(this)->value();
    }
};

template<typename K, typename MV, size_t RequestedSize_, int PSL_Bits, int HashBits, typename Hash = std::hash<K>,
         typename KE = std::equal_to<K>, typename U = std::uint64_t, typename Scatter = FibonacciScatter<U>,
         typename RangeReduce = LemireReduce<RequestedSize_, U>, typename HashReduce = TopHashReducer<HashBits, U>>
struct RH_Frontend_WithSkarupkeTail {
    using Backend = RH_Backend<PSL_Bits, HashBits, U>;
    using MD      = typename Backend::Metadata;

    inline static constexpr auto RequestedSize       = RequestedSize_;
    inline static constexpr auto LongestEncodablePSL = (1 << PSL_Bits);
    inline static constexpr auto WithTail            = RequestedSize + LongestEncodablePSL; // the Skarupke tail
    inline static constexpr auto SWARCount =
        (WithTail + MD::NSlots - 1) / MD::NSlots; // to calculate the ceiling rounding
    inline static constexpr auto SlotCount      = SWARCount * MD::NSlots;
    inline static constexpr auto HighestSafePSL = LongestEncodablePSL - MD::NSlots - 1;

    using MetadataCollection = std::array<MD, SWARCount>;
    using value_type         = std::pair<K, MV>;

    MetadataCollection md_;
    /// \todo Scatter key and value in a flavor
    std::array<KeyValuePairWrapper<K, MV>, SlotCount> values_;
    size_t elementCount_;

    RH_Frontend_WithSkarupkeTail() noexcept
        : elementCount_(0) {
        for (auto& mde : md_) {
            mde = MD{0};
        }
    }

    template<typename Callable>
    void traverse(Callable&& c) const {
        for (size_t swarIndex = 0; swarIndex < SWARCount; ++swarIndex) {
            auto PSLs     = md_[swarIndex].PSLs();
            auto occupied = booleans(PSLs);
            while (occupied) {
                auto intraIndex = occupied.lsbIndex();
                c(swarIndex, intraIndex);
                occupied = occupied.clearLSB();
            }
        }
    }

    ~RH_Frontend_WithSkarupkeTail() {
        traverse([thy = this](std::size_t sI, std::size_t intra) {
            thy->values_[intra + sI * MD::NSlots].destroy();
        });
    }

    RH_Frontend_WithSkarupkeTail(const RH_Frontend_WithSkarupkeTail& model)
        : RH_Frontend_WithSkarupkeTail() {
        model.traverse([thy = this, other = &model](std::size_t sI, std::size_t intra) {
            auto index = intra + sI * MD::NSlots;
            thy->values_[index].build(other->values_[index].value());
            thy->md_[sI] = thy->md_[sI].blitElement(other->md_[sI], intra);
            ++thy->elementCount_;
        });
    }

    RH_Frontend_WithSkarupkeTail(RH_Frontend_WithSkarupkeTail&& donor) noexcept
        : md_(donor.md_), elementCount_(donor.elementCount_) {
        traverse([thy = this, other = &donor](std::size_t sI, std::size_t intra) {
            auto index = intra + sI * MD::NSlots;
            thy->values_[index].build(std::move(other->values_[index].value()));
        });
    }

    auto findParameters(const K& k) const noexcept {
        auto [hoisted, homeIndex] =
            findBasicParameters<K, RequestedSize, HashBits, U, Hash, Scatter, RangeReduce, HashReduce>(k);
        return std::tuple{hoisted, homeIndex, [thy = this, &k](size_t ndx) noexcept {
                              return KE{}(thy->values_[ndx].value().first, k);
                          }};
    }

    template<typename ValuteTypeCompatible>
    auto insert(ValuteTypeCompatible&& val) {
        auto& k = val.first;
        // auto& mv                        = val.second;
        auto [hoistedT, homeIndexT, kc] = findParameters(k);
        auto hoisted                    = hoistedT;
        auto homeIndex                  = homeIndexT;
        auto thy                        = const_cast<RH_Frontend_WithSkarupkeTail*>(this);
        Backend be{thy->md_.data()};
        auto [iT, deadlineT, needleT] = be.findMisaligned_assumesSkarupkeTail(hoisted, homeIndex, kc);
        auto index                    = iT;
        if (HighestSafePSL < index - homeIndex) {
            throw MaximumProbeSequenceLengthExceeded("Scanning for eviction, from finding");
        }
        auto deadline = deadlineT;
        if (!deadline) {
            return std::pair{iterator(values_.data() + index), false};
        }
        auto needle = needleT;
        auto rv     = insertionEvictionChain(index, deadline, needle, std::forward<ValuteTypeCompatible>(val));
        ++elementCount_;
        return rv;
    }

    // Do the chain of relocations
    // From this point onward, the hashes don't matter except for the
    // updates to the metadata, the relocations
    template<typename VTC>
    auto insertionEvictionChain(std::size_t index, U deadline, MD needle, VTC&& val) {
        // auto& k         = val.first;
        // auto& mv        = val.second;
        auto swarIndex  = index / MD::Lanes;
        auto intraIndex = index % MD::Lanes;
        auto mdp        = this->md_.data() + swarIndex;

        // Because we have not decided about strong versus basic exception
        // safety guarantee, for the time being we will just put a very large
        // number here.
        constexpr auto MaxRelocations = 100000;
        std::array<std::size_t, MaxRelocations> relocations;
        std::array<int, MaxRelocations> newElements;
        auto relocationsCount = 0;
        auto elementToInsert  = needle.at(intraIndex);

        // The very last element in the metadata will always have a psl of 0
        // this serves as a sentinel for insertions, the only place to make
        // sure the table has not been exhausted is an eviction chain that
        // ends in the sentinel
        // Also, the encoding for the PSL may be exhausted
        for (;;) {
            // Loop invariant:
            // deadline, index, swarIndex, intraIndex, elementToInsert correct
            // mdp points to the haystack that gave the deadline
            auto md         = *mdp;
            auto evictedPSL = md.PSLs().at(intraIndex);
            if (0 == evictedPSL) { // end of eviction chain!
                if (SlotCount - 1 <= index) {
                    throw MaximumProbeSequenceLengthExceeded("full table");
                }
                if (0 == relocationsCount) { // direct build of a new value
                    values_[index].build(std::piecewise_construct, std::tuple(std::forward<VTC>(val).first),
                                         std::tuple(std::forward<VTC>(val).second));
                    *mdp = mdp->blitElement(intraIndex, elementToInsert);
                    return std::pair{iterator(values_.data() + index), true};
                }
                // the last element is special because it is a
                // move-construction, not a move-assignment
                --relocationsCount;
                auto fromIndex = relocations[relocationsCount];
                values_[index].build(std::move(values_[fromIndex].value()));
                md_[swarIndex]  = md_[swarIndex].blitElement(intraIndex, elementToInsert);
                elementToInsert = newElements[relocationsCount];
                index           = fromIndex;
                swarIndex       = index / MD::NSlots;
                intraIndex      = index % MD::NSlots;
                // do the pair relocations
                while (relocationsCount--) {
                    fromIndex              = relocations[relocationsCount];
                    values_[index].value() = std::move(values_[fromIndex].value());
                    md_[swarIndex]         = md_[swarIndex].blitElement(intraIndex, elementToInsert);
                    elementToInsert        = newElements[relocationsCount];
                    index                  = fromIndex;
                    swarIndex              = index / MD::NSlots;
                    intraIndex             = index % MD::NSlots;
                }
                values_[index].value() = std::forward<VTC>(val);
                md_[swarIndex]         = md_[swarIndex].blitElement(intraIndex, elementToInsert);
                return std::pair{iterator(values_.data() + index), true};
            }
            if (HighestSafePSL < evictedPSL) {
                throw MaximumProbeSequenceLengthExceeded("Encoding insertion");
            }

            // evict the "deadline" element:
            // first, insert the element in its place (it "stole")
            // find the place for the evicted: when Robin Hood breaks again.

            // for this search, we need to make a search needle with only
            // the PSL being evicted.

            // "push" the index of the element that will be evicted
            relocations[relocationsCount] = index;
            // we have a place for the element being inserted, at this index
            newElements[relocationsCount++] = elementToInsert;
            if (MaxRelocations <= relocationsCount) {
                throw RelocationStackExhausted("Relocation Stack");
            }

            // now the insertion will be for the old metadata entry
            elementToInsert = md.hashes().at(intraIndex);

            // now, where should the evicted element go to?
            // assemble a new needle

            // Constants relevant for the rest
            constexpr auto Ones = meta::BitmaskMaker<U, 1, MD::NBits>::value;
            // | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
            constexpr auto ProgressionFromOne = MD(Ones * Ones);
            // | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
            constexpr auto ProgressionFromZero = MD(ProgressionFromOne - MD(Ones));
            // | 0 | 1 | 2 | 3 | ...       | 7 |
            constexpr auto BroadcastSWAR_ElementCount = MD(meta::BitmaskMaker<U, MD::Lanes, MD::NBits>::value);
            // | 8 | 8 | 8 | 8 | ...       | 8 |
            // constexpr auto SWARIterationAddendumBase = ProgressionFromZero + BroadcastSWAR_ElementCount;
            // | 8 | 9 | ...               | 15 |

            auto broadcastedEvictedPSL             = broadcast(MD(evictedPSL));
            auto evictedPSLWithProgressionFromZero = broadcastedEvictedPSL + ProgressionFromZero;
            // | ePSL+0 | ePSL+1 | ePSL+2 | ePSL+3 | ... | ePSL+7 |
            auto needlePSLs = evictedPSLWithProgressionFromZero.shiftLanesLeft(intraIndex);
            // zeroes make the new needle
            // "richer" in all elements lower than the deadline
            // because of the progression starts with 0
            // the "deadline" element will have equal PSL, not
            // "poorer".
            // assuming the deadline happened in the index 2:
            // needlePSLs = |    0   |   0    | ePSL   | ePSL+1 | ... | ePSL+5 |
            // find the place for the new needle, without checking the keys.
            auto haystackPSLs = md.PSLs();
            // haystack < needle => !(haystack >= needle)
            auto breaksRobinHood = not greaterEqual_MSB_off(haystackPSLs, needlePSLs);
            if (!bool(breaksRobinHood)) {
                // no place for the evicted element found in this swar.
                // increment the PSLs in the needle to check the next haystack

                // for the next swar, we will want (continuing the assumption
                // of the deadline happening at index 2)
                // old needle:
                // |    0   |    0   |  ePSL  | ePSL+1 | ... | ePSL+5  |
                // desired new needle PSLs:
                // | ePSL+6 | ePSL+7 | ePSL+8 | ePSL+9 | ... | ePSL+13 |
                // from evictedPSLWithProgressionFromZero,
                // shift "right" NLanes - intraIndex (keep the last two lanes):
                // | ePSL+6 | ePSL+7 | 0 | ... | 0 |
                auto lowerPart =
                    evictedPSLWithProgressionFromZero.shiftLanesRight(MD::Lanes - intraIndex - 1).shiftLanesRight(1);
                // the other part, of +8 onwards, is BroadcastElementCount,
                // shifted:
                //    | 8 | 8 | 8 | 8 | ...       | 8 |
                // shifted two lanes:
                //    | 0 | 0 | 8 | 8 | ...       | 8 |
                //
                auto topAdd = BroadcastSWAR_ElementCount.shiftLanesLeft(intraIndex);
                needlePSLs  = needlePSLs + lowerPart + topAdd;
                for (;;) { // hunt for the next deadline
                    ++swarIndex;
                    // should the maintenance of `index` be replaced
                    // with pointer arithmetic on mdp?
                    index += MD::NSlots;
                    ++mdp;
                    haystackPSLs    = mdp->PSLs();
                    breaksRobinHood = not greaterEqual_MSB_off(haystackPSLs, needlePSLs);
                    if (breaksRobinHood) {
                        break;
                    }
                    evictedPSL += MD::NSlots;
                    if (HighestSafePSL < evictedPSL) {
                        throw MaximumProbeSequenceLengthExceeded("Scanning for eviction, insertion");
                    }
                    needlePSLs = needlePSLs + BroadcastSWAR_ElementCount;
                }
            }
            deadline        = swar::isolateLSB(breaksRobinHood.value());
            intraIndex      = breaksRobinHood.lsbIndex();
            index           = swarIndex * MD::NSlots + intraIndex;
            elementToInsert = elementToInsert | needlePSLs.at(intraIndex);
        }
    }

    struct const_iterator;

    struct iterator {
        KeyValuePairWrapper<K, MV>* p;

        // note: ++ not yet implemented

        value_type& operator*() noexcept {
            return p->value();
        }

        value_type* operator->() noexcept {
            return &p->value();
        }

        bool operator==(iterator other) const noexcept {
            return p == other.p;
        }

        bool operator!=(iterator other) const noexcept {
            return p == other.p;
        }

        bool operator==(const_iterator c) const noexcept {
            return p == c.p;
        }

        iterator(KeyValuePairWrapper<K, MV>* p)
            : p(p) {}
    };

    struct const_iterator {
        const KeyValuePairWrapper<K, MV>* p;

        const value_type& operator*() noexcept {
            return p->value();
        }

        const value_type* operator->() noexcept {
            return &p->value();
        }

        bool operator==(const_iterator other) const noexcept {
            return p == other.p;
        }

        bool operator!=(const_iterator other) const noexcept {
            return p == other.p;
        }

        const_iterator(const KeyValuePairWrapper<K, MV>* p)
            : p(p) {}

        const_iterator(iterator other) noexcept
            : const_iterator(other.p) {}
    };

    const_iterator begin() const noexcept {
        return this->values_.begin();
    }

    const_iterator end() const noexcept {
        return this->values_.end();
    }

    inline iterator find(const K& k) noexcept __attribute__((always_inline));

    const_iterator find(const K& k) const noexcept {
        const_cast<RH_Frontend_WithSkarupkeTail*>(this)->find(k);
    }
};

template<typename K, typename MV, size_t RequestedSize_, int PSL_Bits, int HashBits, typename Hash, typename KE,
         typename U, typename Scatter, typename RangeReduce, typename HashReduce>
auto RH_Frontend_WithSkarupkeTail<K, MV, RequestedSize_, PSL_Bits, HashBits, Hash, KE, U, Scatter, RangeReduce,
                                  HashReduce>::find(const K& k) noexcept -> iterator {
    auto [hoisted, homeIndex, keyChecker] = findParameters(k);
    Backend be{this->md_.data()};
    auto [index, deadline, dontcare] = be.findMisaligned_assumesSkarupkeTail(hoisted, homeIndex, keyChecker);
    return deadline ? values_.end() : values_.data() + index;
}

} // namespace rh

} // namespace zoo

using Canonical = zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 8000, 5, 3>;
