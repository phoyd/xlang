#pragma once
#include <cstddef>
#include <cstdint>

namespace xlang::impl
{
    //
    // A simple, flexible, self contained and stateless UTF8<->UTF16 converter.
    //
    // Note:
    // CCG=C++ Core Guidelines (https://github.com/isocpp/CppCoreGuidelines)

    template <class Traits> class simple_unicode_converter
    {

        // The Traits parameter should contain static functions "data_error()"
        // to report malformed input and "buffer_error()" to report an
        // insufficient output buffer. These functions may never return.
        [[noreturn]] static void invalid() { Traits::data_error(); }
        [[noreturn]] static void buffer_error() { Traits::buffer_error(); }

        // Make narrowing explicit [CCG]
        // must be correct at call size
        template<class R, class A>
        static constexpr R narrow_cast(const A &a)
        {
            return static_cast<R>(a);
        }

      public:
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
        // The function returns the number of characters written to the
        // output iterator.
        // If a complete conversion requires more input than available, then
        // the input is considered malformed.
        //
        // if the conversion needs more output buffer than present, then
        // buffer_error() is signaled.
        //
        // Actually, this routines does not know anything about the kind of data
        // processed.

        // encodings with a common ASCII Plane could simply fast forward
        // conversion here. Specialize for EBCDIC.
        template<class SrcFilter, class DestFilter>
        static bool constexpr is_passthrough(typename SrcFilter::cvt v)
        {
            return v<=0x7fu; // ASCII plane
        }

        template <class In, class Out, class SrcFilter, class DestFilter>
        static size_t convert(In in_start, In in_end, Out out_start,
                              Out out_end, SrcFilter&& src_filter,
                              DestFilter&& dst_filter, bool count_only)
        {

            // the reader closure reads from the input iterator
            auto reader = [&in_start, &in_end]() {
                if (in_start < in_end)
                {
                    return *in_start++;
                }
                else
                {
                    invalid(); // input too short, missing data
                }
            };

            // the writer closures writes to the output iterator
            // and counts the number of written characters.
            size_t written = 0;
            auto writer = [&out_start, &out_end, &written,
                           &count_only](auto item) {
                if (!count_only)
                {
                    if (out_start < out_end)
                    {
                        *out_start++ = item;
                    }
                    else
                    {
                        buffer_error();
                    }
                }
                written++;
            };

            while (in_start < in_end)
            {
                auto b=*in_start++;
                if (is_passthrough<SrcFilter,DestFilter>(b))
                {
                    writer(b);
                }
                else
                {
                    auto cp = src_filter.read(b,reader);
                    dst_filter.write_unchecked(cp, writer);
                }
            }
            return written;
        }

        // codepoints in the surrogate area or after U++10FFFF are invalid.
        static constexpr bool is_invalid_cp(uint32_t u)
        {
            return ((u >= 0xd800) && (u <= 0xdfff)) || (u>0x10ffff);
        }
        // return codepoint, but only if it is valid, otherwise assume malformed data.
        static constexpr uint32_t if_valid(uint32_t u)
        {
            if (is_invalid_cp(u))
            {
                invalid();
            }
            else
            {
                return u;
            }
        }

        class utf32_filter
        {
        public:
            typedef uint32_t cvt; // code value type
            template <class In> static uint32_t read(uint32_t b,In &&in)
            {
                return if_valid(b);
            }
            template <class Out> static int write(uint32_t c, Out &&out)
            {
                return write_unchecked(if_valid(c),out);
            }
            template <class Out> static int write_unchecked(uint32_t c, Out &&out)
            {
              out(if_valid(c));
              return 1;
            }
        };

        class utf16_filter
        {
          private:
            static constexpr bool is_high_surrogate(uint32_t u)
            {
                return ((u >= 0xd800) && (u <= 0xdbff));
            }
            static constexpr bool is_low_surrogate(uint32_t u)
            {
                return ((u >= 0xdc00) && (u <= 0xdfff));
            }

          public:
            typedef uint16_t cvt; // code value type
            // read up to two UTF-16 code units from 'in' and try to make
            // a valid codepoint from it.
            template <class In> static uint32_t read(uint16_t h, In&& in)
            {
                // uint16_t h = in();
                if (is_high_surrogate(h))
                {
                    uint16_t l = in();
                    if (!is_low_surrogate(l))
                    {
                        invalid();
                    }
                    uint32_t cp=((h - 0xd800u) << 10) + (l - 0xdc00u) + 0x10000u;
                    return if_valid(cp);
                }
                return if_valid(h); // stale low surrogates are caught here.
            }

            // write up to two UTF-16 codepoints to 'out'. The output byte order
            // is the native byte order.
            template <class Out> static int write(uint32_t c, Out&& out)
            {
                return write_unchecked(if_valid(c),out);
            }

            template <class Out> static int write_unchecked(uint32_t c, Out&& out)
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
                    if (!is_high_surrogate(h))
                    {
                        invalid();
                    }
                    uint16_t l = 0xdc00 + (c & 0x3ffu);
                    if (!is_low_surrogate(l))
                    {
                        invalid();
                    }
                    out(h);
                    out(l);
                    return 2;
                }
            }
        };

        class utf8_filter
        {
          private:
            // Read 'Count' bits starting from 'Start' in cp and put 'Mark' over
            // the octet result.
            template <uint8_t Mark, unsigned Start, unsigned Count>
            static constexpr uint8_t fetch(uint32_t cp)
            {
                static_assert(Count<8,"invalid bitcount");
                static_assert(Count+Start<32,"invalid bitstart");
                // this can't overflow
                return (Mark | ((cp >> Start) & ((1u << Count) - 1)));
            }
            template <unsigned Mark, unsigned Start, unsigned Count>
            static auto constexpr store_ck(uint32_t &cp, uint8_t b)
            {
                static_assert (Count<8,"invalid bitcount");
                static_assert(Count+Start<32,"invalid bitstart");
                auto mask = ((1 << Count) - 1);
                cp |= (b & mask) << Start;
                return ((b & ~mask) ^ Mark); // return zero if valid
                // return ((b & ~mask) == Mark);
            }

          public:
            typedef uint8_t cvt; // code value type
            // Read up to 4 input bytes as UTF-8 and produce a UTF-32 codepoint
            // in native byte order.
            template <class In> static uint32_t read(uint8_t b, In&& in)
            {
                // ATTENTION:
                // This code tries to mimimize branches
                // using & and | instead of && and ||

                __asm__ volatile("// XXXXXXXXXXX UTF8 read");
                // uint8_t b = in();
                if (b <= 0x7f) // 0x00..0x7f
                {
                    return b; // always valid
                }
                else if (b <= 0xdf) // 0x80..0x7ff
                {
                    uint32_t cp = 0;
                    uint8_t b1=in();
                    auto fail=(store_ck<0xc0, 6, 5>(cp, b)
                            | store_ck<0x80, 0, 6>(cp, b1));

                    if (!fail & (cp>=0x80)) return cp;
                }
                else if (b <= 0xef) // 0x800..0xffff
                {
                    uint32_t cp = 0;
                    uint8_t b1=in();
                    uint8_t b2=in();

                    auto fail=(store_ck<0xe0, 12, 4>(cp, b)
                            | store_ck<0x80, 6, 6>(cp, b1)
                            | store_ck<0x80, 0, 6>(cp, b2));
                    if (!fail & (cp>=0x800) & !is_invalid_cp(cp)) return cp;
                }
                else if (b <= 0xf7) // 0x10000-0x10ffff
                {
                    uint32_t cp = 0;
                    uint8_t b1=in();
                    uint8_t b2=in();
                    uint8_t b3=in();
                    auto fail=(store_ck<0xf0, 18, 3>(cp, b)
                         | store_ck<0x80, 12, 6>(cp, b1)
                         | store_ck<0x80, 6, 6>(cp, b2)
                         | store_ck<0x80, 0, 6>(cp, b3));
                    if (!fail & (cp>=0x10000) & (cp<=0x10ffff)) return cp;
                }
                invalid();
            }

            // Convert a UTF-32 codepoint into up to 4 UTF-8 code unit and
            // write them to 'out' callable.
            template <class Out> static int write(uint32_t cp, Out&& out)
            {
                return write(if_valid(cp),out);
            }
            template <class Out> static int write_unchecked(uint32_t cp, Out&& out)
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
    };

} // namespace xlang::impl
