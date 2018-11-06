#pragma once
#include <algorithm>
#include <cassert>
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
// GSL=CCG support library (https://github.com/Microsoft/GSL/)

using char8_t = uint8_t;

#ifndef GSL_LIKELY
#if defined(__clang__) || defined(__GNUC__)
#define GSL_LIKELY(x) __builtin_expect(!!(x), 1)
#define GSL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define GSL_LIKELY(x) (!!(x))
#define GSL_UNLIKELY(x) (!!(x))
#endif
#endif

// copied from gsl/gsl_assert:
//
// GSL_ASSUME(cond)
//
// Tell the optimizer that the predicate cond must hold. It is unspecified
// whether or not cond is actually evaluated.
//
#ifdef _MSC_VER
#define GSL_ASSUME(cond) __assume(cond)
#elif defined(__clang__) || defined(__GNUC__)
#define GSL_ASSUME(cond) ((cond) ? static_cast<void>(0) : __builtin_unreachable())
#else
#define GSL_ASSUME(cond) static_cast<void>((cond) ? 0 : 0)
#endif
//
// GSL_ASSUME() is not a compiler hint. Instead it kills all code paths where the assumption would
// not hold, making the code undefined if the assumption were wrong. We turn ASSUME in to ASSERT for
// Debug build to check our assumptions at runtime.
//
#ifdef _DEBUG
#define XLANG_ASSUME(cond) assert(cond)
#else
#define XLANG_ASSUME(cond) GSL_ASSUME(cond)
#endif

#ifdef _MSC_VER
#define XLANG_FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
#define XLANG_FORCE_INLINE __attribute__((always_inline))
#else
XLANG_FORCE_INLINE
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
constexpr R narrow_cast(const A& a)
{
    return static_cast<R>(a);
}

//
// bit helper
//
// create a mask of Count bits.
template<unsigned Count>
auto constexpr mask()
{
 return ((1 << Count) - 1);
}
//
// deposit 'Count' bits into return value
//
template<unsigned Start, unsigned Count, class T>
auto constexpr deposit(T in)
{
    constexpr auto m = mask<Count>();
    return (in & m)<<Start;
}
//
// extract 'Count' bits
//
template<unsigned Start, unsigned Count, class T>
auto constexpr extract(T in)
{
    constexpr auto m = mask<Count>();
    return (in>>Start)&m;
}

//
// Codepoints in the surrogate area or after U++10FFFF are invalid.
//
// constexpr bool is_invalid_cp(char32_t u)
//{
//    return ((u >= 0xd800) && (u <= 0xdfff)) || (u > 0x10ffff);
//}
constexpr bool is_valid_cp(char32_t u)
{
    return (u <= 0xd7ff) || ((u > 0xdfff) && (u <= 0x10ffff));
}
//
// They can only be used in the UTF16 encoding, to mark a "surrogate pair",
// an encoding to map code points above 0x10000 into UTF16.
//
constexpr bool is_high_surrogate(char32_t u) { return ((u >= 0xd800) && (u <= 0xdbff)); }
constexpr bool is_low_surrogate(char32_t u) { return ((u >= 0xdc00) && (u <= 0xdfff)); }
constexpr bool is_surrogate(char32_t u) { return ((u >= 0xd800) && (u <= 0xdfff)); }

//
// return codepoint, but only if it is valid, otherwise assume malformed data
// and rise exception
//
constexpr char32_t if_valid(char32_t u)
{
    if (is_valid_cp(u))
        return u;
    else
        invalid();
}

//
// The uft32_filter simply copies inputs and outputs after checking,
// because we are using UTF32 as the intermediate format.
//
class utf32_filter
{
public:
    // The code value type this format is using.
    typedef char32_t cvt;
    // Maximum number of code values per code point. This can be used
    // for optimizations (like omitting buffer checks)
    static constexpr size_t max_cv_len = 1;

    // Read as single code point from input 'in'. 'b' is a lookahead,
    // so we don't need to read anything.
    template <class In>
    static char32_t XLANG_FORCE_INLINE read(char32_t b, In&&)
    {
        return if_valid(b);
    }
    // Convert 'c' to the output format and write it to 'out'
    // Returns the number of cove values that have been written
    template <class Out>
    static int XLANG_FORCE_INLINE write(char32_t c, Out&& out)
    {
        return write_valid(if_valid(c), out);
    }
    // Same as above, except that it does not check if 'c' is a
    // valid code point (for example, because it has been checked before)
    template <class Out>
    static int XLANG_FORCE_INLINE write_valid(char32_t c, Out&& out)
    {
        XLANG_ASSUME(is_valid_cp(c));
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
    typedef char16_t cvt;                   // code value type
    static constexpr size_t max_cv_len = 2; // a surrgate pair is 2 values long

    // read up to 'max_cv_len' UTF-16 code values from 'h' and 'in' and try to make
    // a valid codepoint from it.
    template <class In>
    static char32_t XLANG_FORCE_INLINE read(char16_t h, In&& in)
    {
        if (is_high_surrogate(h))
        {
            char16_t l = in();
            if (!is_low_surrogate(l)) { invalid(); }
            char32_t cp = ((h - 0xd800u) << 10) + (l - 0xdc00u) + 0x10000u;
            return if_valid(cp);
        }
        return if_valid(h);
    }

    // write up to two UTF-16 code values to 'out'. The output byte order
    // is the native byte order.
    template <class Out>
    static int XLANG_FORCE_INLINE write(char32_t c, Out&& out)
    {
        return write_valid(if_valid(c), out);
    }

    template <class Out>
    static int XLANG_FORCE_INLINE write_valid_ext(char32_t c, Out&& out)
    {
        c -= 0x10000;
        XLANG_ASSUME(c <= 0xfffff); // because is_valid_cp(c);

        // 0xfffff>>10 == 0x3ff, 0xd800+0x3ff == 0xdbff
        // therefore h is a valid high surrogate.
        char16_t h = narrow_cast<char16_t>(0xd800 + (c >> 10));
        out(h);
        // 0xdc00 + 0x3ff == 0xdfff
        // therefore l is a valid low surrogate
        char16_t l = 0xdc00 + (c & 0x3ffu);
        out(l);
        return 2;
    }
    template <class Out>
    static int XLANG_FORCE_INLINE write_valid(char32_t c, Out&& out)
    {
        XLANG_ASSUME(is_valid_cp(c));
        if (c < 0x10000)
        {
            out(c);
            return 1;
        }
        else
        {
            return write_valid_ext(c, std::forward<Out>(out));
        }
    }
};

//
// utf8 filter
//
class utf8_filter
{
public:
    typedef char8_t cvt;
    static constexpr size_t max_cv_len = 4;

private:
    // Helper function. Read 'Count' bits starting from 'Start' in cp
    // and put 'Mark' over the octet result.
    template <char8_t Mark, unsigned Start, unsigned Count>
    static constexpr char8_t fetch(const char32_t cp)
    {
        static_assert(Count < 8, "invalid bitcount");
        static_assert(Count + Start < 32, "invalid bitstart");
        return narrow_cast<char8_t>(Mark | extract<Start,Count>(cp));
    }
    // This writes 'Count' bits from 'b' to 'cp' starting at 'Start'
    // Returns a check value that is 0 if 'b' without the 'Count' bits
    // is 'Mark' or some other value if not.
    // return false or non-zero for failure (Mark not found in b)
    template <unsigned Mark, unsigned Start, unsigned Count>
    static auto constexpr store_ck(char32_t& cp, char8_t b)
    {
        static_assert(Count < 8, "invalid bitcount");
        static_assert(Count + Start < 32, "invalid bitstart");
        cp |= deposit<Start,Count>(b);
        //constexpr auto m = ((1u << Count) - 1u);
        constexpr auto m=mask<Count>();
        return (b & ~m ) ^ Mark;
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
    static char32_t XLANG_FORCE_INLINE read(char8_t b, In&& in)
    {
        // ATTENTION:
        // * no-returns are falling through 'invalid' at the end.
        if (b <= 0x7f) // 0x00..0x7f (Hex Min and Hex Max in the table above)
        {
            return b; // always valid
        }
        else if (b <= 0xdf) // 0x80..0x7ff
        {
            char32_t cp = 0;
            char8_t b1 = in();
            auto fail = (store_ck<0xc0, 6, 5>(cp, b) || store_ck<0x80, 0, 6>(cp, b1));
            if (!fail && (cp >= 0x80)) return cp;
        }
        else if (b <= 0xef) // 0x800..0xffff
        {
            char32_t cp = 0;
            char8_t b1 = in();
            char8_t b2 = in();
            auto fail = (store_ck<0xe0, 12, 4>(cp, b) || store_ck<0x80, 6, 6>(cp, b1) ||
                         store_ck<0x80, 0, 6>(cp, b2));
            if (!fail && (cp >= 0x800) && is_valid_cp(cp)) return cp;
        }
        else if (b <= 0xf7) // 0x10000-0x10ffff
        {
            char32_t cp = 0;
            auto fail = (store_ck<0xf0, 18, 3>(cp, b) || store_ck<0x80, 12, 6>(cp, in()) ||
                         store_ck<0x80, 6, 6>(cp, in()) || store_ck<0x80, 0, 6>(cp, in()));
            if (!fail && (cp >= 0x10000) && (cp <= 0x10ffff)) return cp;
        }
        invalid();
    }

    // Convert a UTF-32 codepoint into up to 4 UTF-8 code units and
    // write them to 'out'.
    template <class Out>
    static int XLANG_FORCE_INLINE write(char32_t cp, Out&& out)
    {
        return write_valid(if_valid(cp), out);
    }

    template <class Out>
    static int XLANG_FORCE_INLINE write_valid(char32_t cp, Out&& out)
    {
        XLANG_ASSUME(is_valid_cp(cp));
        if (cp <= 0x7f)
        {
            out(narrow_cast<char8_t>(cp));
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

//
// encodings with a common ASCII Plane might copy code values
// without conversion. Specialize for EBCDIC, UTF-16
//

template<class S,class D>
struct conv_pair
{
    static bool constexpr is_passthrough(typename S::cvt v)
    {
        return v <= 0x7f; // ASCII plane
    }
};
template<>
struct conv_pair<utf16_filter, utf32_filter>
{
    static bool constexpr is_passthrough(typename utf16_filter::cvt v)
    {
        return (v <= 0xd7ff) || (v>=0xdfff); // ASCII plane
    }
};
template<>
struct conv_pair<utf32_filter, utf16_filter>
{
    static bool constexpr is_passthrough(typename utf32_filter::cvt v)
    {
        return (v <= 0xd7ff);
    }
};

//
// We can use SrcFilter and DestFilter as they are or join them in
// a 'transformer' which can be specialized on its type parameters, for
// example to implement an optimized conversion pair.
//
template <class SrcFilter, class DestFilter>
struct transformer
{
    SrcFilter&& src;
    DestFilter&& dst;

    template <class R, class W>
    size_t XLANG_FORCE_INLINE tranform_one(typename SrcFilter::cvt b, R&& reader, W&& writer)
    {
        if (conv_pair<SrcFilter,DestFilter>::is_passthrough(b))
        {
            writer(b);
            return 1;
        }
        else
        {
            auto cp = src.read(b, reader);
            return dst.write_valid(cp, writer);
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
inline converter_result convert(In in_start, In in_end, Out out_start, Out out_end,
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
inline converter_result output_size(In in_start, In in_end, SrcFilter&& src_filter,
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
// This is the default template and the general case.
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
        transformer<SrcFilter, DestFilter> trans{std::forward<SrcFilter>(src_filter),
                                                 std::forward<DestFilter>(dst_filter)};

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
        { write_count += trans.tranform_one(reader_unchecked(), reader_checked, writer_checked); }
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
        transformer<SrcFilter, DestFilter> trans{std::forward<SrcFilter>(src_filter),
                                                 std::forward<DestFilter>(dst_filter)};

        auto out_start_org = out_start; // (out-start-out_start_org) => number of values written
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

        while (in_start != in_end)
        {
            auto in_len = in_end - in_start;
            auto out_len = out_end - out_start;

            auto safelen =
                std::min(in_len / SrcFilter::max_cv_len, out_len / DestFilter::max_cv_len);
            auto batchlen = safelen / 4;
            if (batchlen == 0) break;
            for (size_t i = 0; i < batchlen; i++)
            {
                trans.tranform_one(reader_unchecked(), reader_unchecked, writer_unchecked);
                trans.tranform_one(reader_unchecked(), reader_unchecked, writer_unchecked);
                trans.tranform_one(reader_unchecked(), reader_unchecked, writer_unchecked);
                trans.tranform_one(reader_unchecked(), reader_unchecked, writer_unchecked);
            }
        }

        while (in_start != in_end)
        { trans.tranform_one(reader_unchecked(), reader_checked, writer_checked); }
        return out_start - out_start_org;
    }
};
//
// This is the general case for the
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
