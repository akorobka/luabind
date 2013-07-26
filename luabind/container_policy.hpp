// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef LUABIND_CONTAINER_POLICY_HPP_INCLUDED
#define LUABIND_CONTAINER_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <luabind/detail/policy.hpp>    // for policy_cons, etc
#include <luabind/detail/decorate_type.hpp>  // for LUABIND_DECORATE_TYPE
#include <luabind/detail/primitives.hpp>  // for null_type (ptr only), etc
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/if.hpp>             // for if_
#include <boost/type_traits/is_same.hpp>  // for is_same
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/and.hpp>

namespace luabind { namespace detail {

	BOOST_MPL_HAS_XXX_TRAIT_DEF(first_type)
	BOOST_MPL_HAS_XXX_TRAIT_DEF(second_type)

	namespace mpl = boost::mpl;

	template <typename T> struct has_compound_value_type :
	        mpl::and_<has_first_type<typename T::value_type>,
			  has_second_type<typename T::value_type> > {
	};

	template<class Policies>
	struct container_converter_lua_to_cpp
	{
		typedef typename find_conversion_policy<1, Policies>::type converter_policy;

		template <typename T> struct default_adapter
		{
			typedef typename T::value_type value_type;

			typename mpl::apply_wrap2<converter_policy,value_type,lua_to_cpp>::type converter;

			T container_;		

			void push_top(lua_State* L) {
				if (converter.match(L, LUABIND_DECORATE_TYPE(value_type), -1) >= 0)
					container_.push_back(converter.apply(L, LUABIND_DECORATE_TYPE(value_type), -1));
				else
					throw std::runtime_error("Container is incompatible with argument data type");
			}

			operator T() {
				return container_;
			}
		};

		template <typename T> struct compound_value_adapter
		{
			typedef typename T::value_type value_type;

			typedef typename value_type::first_type first_type;
			typedef typename value_type::second_type second_type;

			typename mpl::apply_wrap2<converter_policy,first_type,lua_to_cpp>::type converter_key;
			typename mpl::apply_wrap2<converter_policy,second_type,lua_to_cpp>::type converter_value;

			T container_;

			void push_top(lua_State* L) {
				if (converter_key.match(L, LUABIND_DECORATE_TYPE(first_type), -2) < 0)
					throw std::runtime_error("Container key type is incompatible with table entry key");

				if (converter_value.match(L, LUABIND_DECORATE_TYPE(second_type), -1) < 0)
					throw std::runtime_error("Container value type is incompatible with table entry value");

				container_.insert(value_type(converter_key.apply(L, LUABIND_DECORATE_TYPE(first_type), -2),
							     converter_value.apply(L, LUABIND_DECORATE_TYPE(second_type), -1)));
			}

			operator T() {
				return container_;
			}
		};

		int consumed_args(...) const
		{
			return 1;
		}

		template<class T>
		T apply(lua_State* L, by_const_reference<T>, int index)
		{
			typename mpl::eval_if<has_compound_value_type<T>,
					mpl::identity<compound_value_adapter<T> >,
					mpl::identity<default_adapter<T> >
			>::type adapter;

			lua_pushnil(L);
			while (lua_next(L, index))
			{
				adapter.push_top(L);
				lua_pop(L, 1); // pop value
			}

			return adapter;
		}

		template<class T>
		T apply(lua_State* L, by_value<T>, int index)
		{
			return apply(L, by_const_reference<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, by_const_reference<T>, int index)
		{
			if (lua_istable(L, index)) return 0; else return -1;
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

	template<class Policies>
	struct container_converter_cpp_to_lua
	{
		typedef typename find_conversion_policy<1, Policies>::type converter_policy;

		template <typename T> struct default_adapter
		{
			typedef typename T::value_type value_type;

			typename mpl::apply_wrap2<converter_policy,value_type,cpp_to_lua>::type converter;

			void convert(lua_State* L, int index, const value_type& v) {
				converter.apply(L, v);
				lua_rawseti(L, -2, index);
			}
		};

		template <typename T> struct compound_value_adapter
		{
			typedef typename T::value_type value_type;

			typedef typename value_type::first_type first_type;
			typedef typename value_type::second_type second_type;

			typename mpl::apply_wrap2<converter_policy,first_type,cpp_to_lua>::type converter_key;
			typename mpl::apply_wrap2<converter_policy,second_type,cpp_to_lua>::type converter_value;

			void convert(lua_State* L, int index, const value_type& v) {
				converter_key.apply(L, v.first);
				converter_value.apply(L, v.second);
				lua_settable(L, -3);
			}
		};

		template<class T>
		void apply(lua_State* L, const T& container)
		{
			typename mpl::eval_if<has_compound_value_type<T>,
					mpl::identity<compound_value_adapter<T> >,
					mpl::identity<default_adapter<T> >
			>::type adapter;

			lua_newtable(L);

			int index = 1;

			for (typename T::const_iterator it = container.begin(); it != container.end(); ++it)
			{
				adapter.convert(L, index++, *it);
			}
		}
	};

	template<int N, class Policies>
//	struct container_policy : converter_policy_tag
	struct container_policy : conversion_policy<N>
	{
//		BOOST_STATIC_CONSTANT(int, index = N);

		static void precall(lua_State*, const index_map&) {}
		static void postcall(lua_State*, const index_map&) {}

		struct only_accepts_nonconst_pointers {};

		template<class T, class Direction>
		struct apply
		{
			typedef typename mpl::if_<boost::is_same<lua_to_cpp, Direction>
				, container_converter_lua_to_cpp<Policies>
				, container_converter_cpp_to_lua<Policies>
			>::type type;
		};
	};

}}

namespace luabind
{
	template<int N>
	detail::policy_cons<detail::container_policy<N, detail::null_type>, detail::null_type> 
	container(LUABIND_PLACEHOLDER_ARG(N)) 
	{ 
		return detail::policy_cons<detail::container_policy<N, detail::null_type>, detail::null_type>(); 
	}

	template<int N, class Policies>
	detail::policy_cons<detail::container_policy<N, Policies>, detail::null_type> 
	container(LUABIND_PLACEHOLDER_ARG(N), const Policies&) 
	{ 
		return detail::policy_cons<detail::container_policy<N, Policies>, detail::null_type>(); 
	}
}

#endif // LUABIND_CONTAINER_POLICY_HPP_INCLUDED
