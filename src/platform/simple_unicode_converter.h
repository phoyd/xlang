#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <stdexcept>

namespace xlang::impl::code_converter
{
//
// A simple, flexible, self contained and stateless UTF8<->UTF16 converter.
//
// Note:
// CCG=C++ Core Guidelines (https://github.com/isocpp/CppCoreGuidelines)

#ifndef GSL_LIKELY
#if defined(__clang__) || defined(__GNUC__)
#define GSL_LIKELY(x) __builtin_expect(!!(x), 1)
#define GSL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define GSL_LIKELY(x) (!!(x))
#define GSL_UNLIKELY(x) (!!(x))
#endif
#endif

//
// error codes returned by user visible conversion functions.
//
enum class converter_result
{
    OK,
    INVALID_INPUT_DATA,
    OUTPUT_TOO_SMALL
};

//
// throw converter_result as exception (internal use)
//
[[noreturn]] inline void invalid() { throw converter_result::INVALID_INPUT_DATA; }
[[noreturn]] inline void buffer_error() { throw converter_result::OUTPUT_TOO_SMALL; }

//
// [CCG]: Make narrowing explicit
// must be correct at call size
//
template <class R, class A>
static constexpr R narrow_cast(const A& a)
{
    return static_cast<R>(a);
}

//
// encodings with a common ASCII Plane might copy code values
// without conversion. Specialize for EBCDIC, UTF-16
//
template <class SrcFilter, class DestFilter>
static bool constexpr is_passthrough(typename SrcFilter::cvt v)
{
    return v <= 0x7f; // ASCII plane
}

//
// Codepoints in the surrogate area or after U++10FFFF are invalid.
//
static constexpr bool is_invalid_cp(uint32_t u)
{
    return ((u >= 0xd800) && (u <= 0xdfff)) || (u > 0x10ffff);
}
//
// They can only be used in the UTF16 encoding, to mark a "surrogate pair",
// an encoding to map code points above 0x10000 into UTF16.
//
static constexpr bool is_high_surrogate(uint32_t u) { return ((u >= 0xd800) && (u <= 0xdbff)); }
static constexpr bool is_low_surrogate(uint32_t u) { return ((u >= 0xdc00) && (u <= 0xdfff)); }

//
// return codepoint, but only if it is valid, otherwise assume malformed data
// and rise exception
//
static constexpr uint32_t if_valid(uint32_t u)
{
    if (is_invalid_cp(u)) { invalid(); }
    else
    {
        return u;
    }
}

//
// The uft32_filter simply copies inputs and outputs after checking,
// because we are using UTF32 as the intermediate format.
//
class utf32_filter
{
public:
    // The code value type this format is using.
    typedef uint32_t cvt;
    // Maximum number of code values per code point. This can be used
    // for optimizations (like omitting buffer checks)
    static constexpr size_t max_cv_len = 1;

    // Read as single code point from input 'in'. 'b' is a lookahead,
    // so we don't need to read anything.
    template <class In>
    static uint32_t read(uint32_t b, In&& in)
    {
        return if_valid(b);
    }
    // Convert 'c' to the output format and write it to 'out'
    // Returns the number of cove values that have been written
    template <class Out>
    static int write(uint32_t c, Out&& out)
    {
        return write_valid(if_valid(c), out);
    }
    // Same as above, except that it does not check if 'c' is a
    // valid code point (for example, because it has been checked before)
    template <class Out>
    static int write_valid(uint32_t c, Out&& out)
    {
        out(c);
        return 1;
    }
};

//
// The utf16_filter reads and writes UTF16 encoded code values in the
// native endian, without BOM checking.
//
class utf16_filter
{
private:
public:
    typedef uint16_t cvt;                   // code value type
    static constexpr size_t max_cv_len = 2; // a surrgate pair is 2 values long

    // read up to 'max_cv_len' UTF-16 code values from 'h' and 'in' and try to make
    // a valid codepoint from it.
    template <class In>
    static uint32_t read(uint16_t h, In&& in)
    {
        if (is_high_surrogate(h))
        {
            uint16_t l = in();
            if (!is_low_surrogate(l)) { invalid(); }
            uint32_t cp = ((h - 0xd800u) << 10) + (l - 0xdc00u) + 0x10000u;
            return if_valid(cp);
        }
        return if_valid(h); // TODO: simplyfy check.
    }

    // write up to two UTF-16 code values to 'out'. The output byte order
    // is the native byte order.
    template <class Out>
    static int write(uint32_t c, Out&& out)
    {
        return write_valid(if_valid(c), out);
    }

    template <class Out>
    static int write_valid(uint32_t c, Out&& out)
    {
        if (c < 0x10000)
        {
            out(c);
            return 1;
        }
        else
        {
            c -= 0x10000;
            uint16_t h = narrow_cast<uint16_t>(0xd800 + (c >> 10));
            if (!is_high_surrogate(h)) { invalid(); }
            uint16_t l = 0xdc00 + (c & 0x3ffu);
            if (!is_low_surrogate(l)) { invalid(); }
            out(h);
            out(l);
            return 2;
        }
    }
};

//
// utf8 filter
//
class utf8_filter
{
public:
    typedef uint8_t cvt;
    static constexpr size_t max_cv_len = 4;

private:
    // Helper function. Read 'Count' bits starting from 'Start' in cp
    // and put 'Mark' over the octet result.
    template <uint8_t Mark, unsigned Start, unsigned Count>
    static constexpr uint8_t fetch(uint32_t cp)
    {
        static_assert(Count < 8, "invalid bitcount");
        static_assert(Count + Start < 32, "invalid bitstart");
        // this can't overflow
        return (Mark | ((cp >> Start) & ((1u << Count) - 1)));
    }
    // This writes 'Count' bits from 'b' to 'cp' starting at 'Start'
    // Returns a check value that is 0 if 'b' without the 'Count' bits
    // is 'Mark' or some other value if not. This indicates failure
    // and or'ing multiple results from storke_ck show if any of the
    // results indicate failure.
    template <unsigned Mark, unsigned Start, unsigned Count>
    static auto constexpr store_ck(uint32_t& cp, uint8_t b)
    {
        static_assert(Count < 8, "invalid bitcount");
        static_assert(Count + Start < 32, "invalid bitstart");
        auto mask = ((1 << Count) - 1);
        cp |= (b & mask) << Start;
        return ((b & ~mask) ^ Mark); // return zero if valid
        // return ((b & ~mask) == Mark);
    }

public:
    // Read up to 4 input values as UTF-8 and produce a UTF-32 code point
    // in native byte order. NOTE: We are dealing with UTF-32 code points
    // here, if we would want to support the full UCS encoding, we would
    // need up to 6 input values here.
    // As a quick reminder, this illustration from Rob Pike
    // (http://doc.cat-v.org/bell_labs/utf-8_history)
    //
    //    Bits  Hex Min  Hex Max  Byte Sequence in Binary
    // 1    7  00000000 0000007f 0vvvvvvv
    // 2   11  00000080 000007FF 110vvvvv 10vvvvvv
    // 3   16  00000800 0000FFFF 1110vvvv 10vvvvvv 10vvvvvv
    // 4   21  00010000 001FFFFF 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
    // 5   26  00200000 03FFFFFF 111110vv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
    // 6   31  04000000 7FFFFFFF 1111110v 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
    //
    template <class In>
    static uint32_t read(uint8_t b, In&& in)
    {
        // ATTENTION:
        // * no returns are falling through 'invalid' at the end.
        if (b <= 0x7f) // 0x00..0x7f (Hex Min and Hex Max in the table above)
        {
            return b; // always valid
        }
        else if (b <= 0xdf) // 0x80..0x7ff
        {
            uint32_t cp = 0;
            uint8_t b1 = in();
            // see "Byte Sequence in Binary"
            auto fail = (store_ck<0xc0, 6, 5>(cp, b) | store_ck<0x80, 0, 6>(cp, b1));
            // this sequence must return a code point from
            // the range above. Smalle code points are an overlong encoding
            // error.
            if (!fail && (cp >= 0x80)) return cp;
        }
        else if (b <= 0xef) // 0x800..0xffff
        {
            uint32_t cp = 0;
            uint8_t b1 = in();
            uint8_t b2 = in();

            auto fail = (store_ck<0xe0, 12, 4>(cp, b) | store_ck<0x80, 6, 6>(cp, b1) ||
                         store_ck<0x80, 0, 6>(cp, b2));
            // check if cp is in the surrogate area
            if (!fail && (cp >= 0x800) && !is_invalid_cp(cp)) return cp;
        }
        else if (b <= 0xf7) // 0x10000-0x10ffff
        {
            uint32_t cp = 0;
            uint8_t b1 = in();
            uint8_t b2 = in();
            uint8_t b3 = in();
            auto fail = (store_ck<0xf0, 18, 3>(cp, b) | store_ck<0x80, 12, 6>(cp, b1) |
                         store_ck<0x80, 6, 6>(cp, b2) | store_ck<0x80, 0, 6>(cp, b3));
            if (!fail && (cp >= 0x10000) && (cp <= 0x10ffff)) return cp;
        }
        invalid();
    }

    // Convert a UTF-32 codepoint into up to 4 UTF-8 code units and
    // write them to 'out'.
    template <class Out>
    static int write(uint32_t cp, Out&& out)
    {
        return write_valid(if_valid(cp), out);
    }
    // for the magic numbers see 'read'
    template <class Out>
    static int write_valid(uint32_t cp, Out&& out)
    {
        if (cp <= 0x7f)
        {
            out(narrow_cast<uint8_t>(cp));
            return 1;
        }
        else if (cp <= 0x7ff)
        {
            out(fetch<0xc0, 6, 5>(cp));
            out(fetch<0x80, 0, 6>(cp));
            return 2;
        }
        else if (cp <= 0xffff)
        {
            out(fetch<0xe0, 12, 4>(cp));
            out(fetch<0x80, 6, 6>(cp));
            out(fetch<0x80, 0, 6>(cp));
            return 3;
        }
        else if (cp <= 0x10FFFF)
        {
            out(fetch<0xf0, 18, 3>(cp));
            out(fetch<0x80, 12, 6>(cp));
            out(fetch<0x80, 6, 6>(cp));
            out(fetch<0x80, 0, 6>(cp));
            return 4;
        }
        else
        {
            invalid();
        }
    }
};

// 'convert' tries to convert the *complete* range from 'in_start' to
// 'in_end' and writes the result to the 'out_start' iterator, after
// processing the data with 'src_filter' and 'dst_filter'.
//
// 'src_filter' reads data in input format from its supplied reader
// argument and produces a data representation that is understood by
// dst_filter (for example an UTF-32 code unit).
//
// 'dst_filter' converts the internmediate representation
// to the output format and writes the result to its supplied writer.
//
// if 'count_only' is true, then no data is written to the output
// iterator. The number of output characters however is counted
// and returned.
//
// For the return value see 'converter_result'
//
// The function returns the number of characters written to the
// output iterator to the reference parameter "result_size"
//
// If a complete conversion requires more input than available, then
// the input is considered malformed.
//
// if the conversion needs more output buffer than present, then
// buffer_error() is signaled.

template <class In, class Out, class SrcFilter, class DestFilter,
          class InCat = typename std::iterator_traits<In>::iterator_category,
          class OutCat = typename std::iterator_traits<Out>::iterator_category>
class converter_spec;

template <class In, class SrcFilter, class DestFilter,
          class InCat = typename std::iterator_traits<In>::iterator_category>
class output_size_counter_spec;

// forward to a specialization on the iterator types.
template <class In, class Out, class SrcFilter, class DestFilter>
static converter_result convert(In in_start, In in_end, Out out_start, Out out_end,
                                SrcFilter&& src_filter, DestFilter&& dst_filter,
                                size_t& result_size)
{
    try
    {
        // dispatch conversion to an specialized overload.
        auto sz = converter_spec<In, Out, SrcFilter, DestFilter>::convert(
            in_start, in_end, out_start, out_end, std::forward<SrcFilter>(src_filter),
            std::forward<DestFilter>(dst_filter));
        result_size = sz;
        return converter_result::OK;
    } catch (converter_result r)
    {
        return r;
    }
}
// forward to a specialization on the iterator types.
template <class In, class SrcFilter, class DestFilter>
static converter_result output_size(In in_start, In in_end, SrcFilter&& src_filter,
                                    DestFilter&& dst_filter, size_t& result_size)
{
    try
    {
        // dispatch conversion to an specialized overload.
        auto sz = output_size_counter_spec<In, SrcFilter, DestFilter>::output_size(
            in_start, in_end, std::forward<SrcFilter>(src_filter),
            std::forward<DestFilter>(dst_filter));
        result_size = sz;
        return converter_result::OK;
    } catch (converter_result r)
    {
        return r;
    }
}

//
//          Specialized Converter
//
//
// This is the default template and the general case
// In and Out can move forward, compare for *(in-)equality* only,
// Out is mutable
//
template <class In, class Out, class SrcFilter, class DestFilter, class InCat, class OutCat>
class converter_spec
{

public:
    static size_t convert(In in_start, In in_end, Out out_start, Out out_end,
                          SrcFilter&& src_filter, DestFilter&& dst_filter)
    {
        // the reader closure reads from the input iterator
        auto reader_checked = [&in_start, in_end]() {
            if (GSL_LIKELY(in_start != in_end)) { return *in_start++; }
            else
            {
                invalid();
            }
        };
        auto reader_unchecked = [&in_start]() { return *in_start++; };

        auto writer_checked = [&out_start, out_end](auto item) {
            if (GSL_LIKELY(out_start != out_end)) { *out_start++ = item; }
            else
            {
                buffer_error();
            }
        };
        size_t write_count = 0;

        while (in_start != in_end)
        {
            auto b = reader_unchecked();
            if (is_passthrough<SrcFilter, DestFilter>(b))
            {
                writer_checked(b);
                write_count++;
            }
            else
            {
                auto cp = src_filter.read(b, reader_checked);
                write_count += dst_filter.write_valid(cp, writer_checked);
            }
        }
        return write_count;
    }
};

//
// Specialization: In and Out are random access iterators.
//
template <class In, class Out, class SrcFilter, class DestFilter>
class converter_spec<In, Out, SrcFilter, DestFilter, std::random_access_iterator_tag,
                     std::random_access_iterator_tag>
{

public:
    static size_t convert(In in_start, In in_end, Out out_start, Out out_end,
                          SrcFilter&& src_filter, DestFilter&& dst_filter)
    {
        // the reader closure reads from the input iterator
        auto reader_checked = [&in_start, in_end]() {
            if (GSL_LIKELY(in_start != in_end)) { return *in_start++; }
            else
            {
                invalid();
            }
        };
        auto reader_unchecked = [&in_start]() { return *in_start++; };

        auto writer_checked = [&out_start, out_end](auto item) {
            if (GSL_LIKELY(out_start != out_end)) { *out_start++ = item; }
            else
            {
                buffer_error();
            }
        };
        auto writer_unchecked = [&out_start](auto item) { *out_start++ = item; };
        size_t write_count = 0;

#define XLANG_CONVERT_ONE(R, W)                                                                    \
    do                                                                                             \
    {                                                                                              \
        auto b = reader_unchecked(); /* always safe */                                             \
        if (is_passthrough<SrcFilter, DestFilter>(b))                                              \
        {                                                                                          \
            W(b);                                                                                  \
            write_count++;                                                                         \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            auto cp = src_filter.read(b, R);                                                       \
            write_count += dst_filter.write_valid(cp, W);                                          \
        }                                                                                          \
    } while (0)

        //
        while (in_start != in_end)
        {
            auto in_len = in_end - in_start;
            auto out_len = out_end - out_start;

            auto safelen =
                std::min(in_len / SrcFilter::max_cv_len, out_len / DestFilter::max_cv_len);
            auto batchlen = safelen / 4;
            if (batchlen == 0) break;
            int i = 0;

            for (int i = 0; i < batchlen; i++)
            {
                XLANG_CONVERT_ONE(reader_unchecked, writer_unchecked);
                XLANG_CONVERT_ONE(reader_unchecked, writer_unchecked);
                XLANG_CONVERT_ONE(reader_unchecked, writer_unchecked);
                XLANG_CONVERT_ONE(reader_unchecked, writer_unchecked);
            }
        }

        while (in_start != in_end)
        {
            auto b = reader_unchecked();
            if (is_passthrough<SrcFilter, DestFilter>(b))
            {
                writer_checked(b);
                write_count++;
            }
            else
            {
                auto cp = src_filter.read(b, reader_checked);
                write_count += dst_filter.write_valid(cp, writer_checked);
            }
        }
        return write_count;
    }
};

//
// This is the default template and the general case the
// output_size calculator.
// In can move forward, compare for *(in-)equality* only,
template <class In, class SrcFilter, class DestFilter, class InCat>
class output_size_counter_spec
{
public:
    // Default case, In and Out are forward only, mutable and compare for ineqality.
    static size_t output_size(In in_start, In in_end, SrcFilter&& src_filter,
                              DestFilter&& dst_filter)
    {
        // the reader closure reads from the input iterator
        auto reader_checked = [&in_start, &in_end]() {
            if (GSL_LIKELY(in_start != in_end)) { return *in_start++; }
            else
            {
                invalid();
            }
        };
        auto reader_unchecked = [&in_start]() { return *in_start++; };

        auto nullput = [](...) {};
        size_t write_count = 0;
        while (in_start != in_end)
        {
            auto b = reader_unchecked();
            auto cp = src_filter.read(b, reader_checked);
            write_count += dst_filter.write_valid(cp, nullput);
        }
        return write_count;
    }
};
} // namespace xlang::impl::code_converter
