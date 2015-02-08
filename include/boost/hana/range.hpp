/*!
@file
Defines `boost::hana::Range`.

@copyright Louis Dionne 2014
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_RANGE_HPP
#define BOOST_HANA_RANGE_HPP

#include <boost/hana/fwd/range.hpp>

#include <boost/hana/comparable.hpp>
#include <boost/hana/constant.hpp>
#include <boost/hana/core/common.hpp>
#include <boost/hana/core/convert.hpp>
#include <boost/hana/core/datatype.hpp>
#include <boost/hana/core/models.hpp>
#include <boost/hana/detail/create.hpp>
#include <boost/hana/detail/std/forward.hpp>
#include <boost/hana/detail/std/integer_sequence.hpp>
#include <boost/hana/detail/std/integral_constant.hpp>
#include <boost/hana/enumerable.hpp>
#include <boost/hana/foldable.hpp>
#include <boost/hana/group.hpp>
#include <boost/hana/integral.hpp> // required by fwd decl
#include <boost/hana/iterable.hpp>
#include <boost/hana/logical.hpp>
#include <boost/hana/monoid.hpp>
#include <boost/hana/orderable.hpp>


namespace boost { namespace hana {
    //////////////////////////////////////////////////////////////////////////
    // range
    //////////////////////////////////////////////////////////////////////////
    template <typename From, typename To>
    struct _range
        : operators::enable_adl
        , operators::Iterable_ops<_range<From, To>>
    {
        using underlying = typename common<
            typename datatype<From>::type,
            typename datatype<To>::type
        >::type;

        constexpr _range(From f, To t) : from(f), to(t) {
            auto valid_range = hana::less_equal(from, to);
            static_assert(hana::value(valid_range),
            "invalid usage of boost::hana::range(from, to) with from > to");
        }
        From from;
        To to;
    };

    template <typename From, typename To>
    struct datatype<_range<From, To>> {
        using type = Range;
    };

    template <typename From, typename To>
    constexpr auto _make_range::operator()(From from, To to) const {
        using C = typename common<
            typename datatype<From>::type,
            typename datatype<To>::type
        >::type;
        return detail::create<_range>{}(hana::to<C>(from), hana::to<C>(to));
    }

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct enabled_operators<Range>
        : Comparable, Iterable
    { };

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct models<Comparable(Range)>
        : detail::std::true_type
    { };

    template <>
    struct equal_impl<Range, Range> {
        template <typename R1, typename R2>
        static constexpr auto apply(R1 r1, R2 r2) {
            return hana::or_(
                hana::and_(hana::is_empty(r1), hana::is_empty(r2)),
                hana::and_(
                    hana::equal(r1.from, r2.from),
                    hana::equal(r1.to, r2.to)
                )
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct models<Foldable(Range)>
        : detail::std::true_type
    { };

    template <>
    struct unpack_impl<Range> {
        template <typename R, typename F, typename T, T ...v>
        static constexpr decltype(auto)
        unpack_helper(R r, F&& f, detail::std::integer_sequence<T, v...>) {
            using U = typename R::underlying;
            constexpr auto from = hana::value(r.from);
            return detail::std::forward<F>(f)(to<U>(integral<T, from + v>)...);
        }

        template <typename R, typename F>
        static constexpr decltype(auto) apply(R r, F&& f) {
            constexpr auto from = hana::value(r.from);
            constexpr auto to = hana::value(r.to);
            constexpr auto size = to - from;
            return unpack_helper(r, detail::std::forward<F>(f),
                    detail::std::make_integer_sequence<decltype(size), size>{});
        }
    };

    template <>
    struct length_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return hana::minus(r.to, r.from); }
    };

    template <>
    struct minimum_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return r.from; }
    };

    template <>
    struct maximum_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return hana::pred(r.to); }
    };

    template <>
    struct sum_impl<Range> {
        // Returns the sum of `[m, n]`, where `m <= n` always hold.
        template <typename I>
        static constexpr I sum_helper(I m, I n) {
            if (m == n)
                return m;

            // 0 == m < n
            else if (0 == m)
                return n * (n+1) / 2;

            // 0 < m < n
            else if (0 < m)
                return sum_helper(0, n) - sum_helper(0, m-1);

            // m < 0 <= n
            else if (0 <= n)
                return sum_helper(0, n) - sum_helper(0, -m);

            // m < n < 0
            else
                return -sum_helper(-n, -m);
        }

        template <typename R>
        static constexpr auto apply(R r) {
            using U = typename R::underlying;
            constexpr auto from = hana::value(r.from);
            constexpr auto to = hana::value(r.to);
            constexpr auto s = from == to ? 0 : sum_helper(from, to-1);
            return hana::to<U>(integral<decltype(s), s>);
        }
    };

    template <>
    struct product_impl<Range> {
        // Returns the product of `[m, n)`, where `m <= n` always hold.
        template <typename I>
        static constexpr I product_helper(I m, I n) {
            if (m <= 0 && 0 < n)
                return 0;
            else {
                I p = 1;
                for (; m != n; ++m)
                    p *= m;
                return p;
            }
        }

        template <typename R>
        static constexpr auto apply(R r) {
            using U = typename R::underlying;
            constexpr auto from = hana::value(r.from);
            constexpr auto to = hana::value(r.to);
            constexpr auto s = product_helper(from, to);
            return hana::to<U>(integral<decltype(s), s>);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Iterable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct models<Iterable(Range)>
        : detail::std::true_type
    { };

    template <>
    struct head_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return r.from; }
    };

    template <>
    struct tail_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return hana::range(hana::succ(r.from), r.to); }
    };

    template <>
    struct is_empty_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return hana::equal(r.from, r.to); }
    };

    template <>
    struct at_impl<Range> {
        template <typename N, typename R>
        static constexpr auto apply(N n, R r) {
            using U = typename R::underlying;
            return hana::plus(r.from, to<U>(n));
        }
    };

    template <>
    struct last_impl<Range> {
        template <typename R>
        static constexpr auto apply(R r)
        { return hana::pred(r.to); }
    };

    template <>
    struct drop_impl<Range> {
        template <typename N, typename R>
        static constexpr auto apply(N n, R r) {
            using U = typename R::underlying;
            return hana::range(
                    hana::min(r.to, hana::plus(r.from, to<U>(n))),
                    r.to
            );
        }
    };
}} // end namespace boost::hana

#endif // !BOOST_HANA_RANGE_HPP
