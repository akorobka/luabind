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


#ifndef LUABIND_VARARG_CONTAINER_POLICY_HPP_INCLUDED
#define LUABIND_VARARG_CONTAINER_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <luabind/detail/policy.hpp>
#include <boost/mpl/apply_wrap.hpp>

namespace luabind { namespace detail {

	namespace mpl = boost::mpl;

	template<int N, class Policies>
	struct vararg_converter_lua_to_cpp
	{
		int consumed_args(int nargs, int n, ...) const
		{
			return nargs - n; // nargs is the total number and n is the number of previously consumed arguments
		}

		template<class T>
		T apply(lua_State* L, by_const_reference<T>, int index)
		{
			typedef typename T::value_type value_type;

			typedef typename find_conversion_policy<1, Policies>::type converter_policy;
			typename mpl::apply_wrap2<converter_policy,value_type,lua_to_cpp>::type converter;

			T container;

			for (int i = index - lua_gettop(L) - 1; i < 0; i++) {
				if (converter.match(L, LUABIND_DECORATE_TYPE(value_type), i) >= 0)
					container.push_back(converter.apply(L, LUABIND_DECORATE_TYPE(value_type), i));
				else
					throw std::runtime_error("Collection is incompatible with argument data type");
			}
			return container;
		}

		template<class T>
		T apply(lua_State* L, by_value<T>, int index)
		{
			return apply(L, by_const_reference<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, by_const_reference<T>, int index)
		{
			for (int i = index - lua_gettop(L) - 1; i < 0; i++) {
				int type = lua_type(L, i);
				if (type != LUA_TSTRING &&
				    type != LUA_TNUMBER &&
				    type != LUA_TBOOLEAN &&
				    type != LUA_TUSERDATA)
					return -1;
			}
			return 0;
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

	template<int N, class Policies>
//	struct vararg_policy : converter_policy_tag
	struct vararg_policy : conversion_policy<N>
	{
//		BOOST_STATIC_CONSTANT(int, index = N);

		static void precall(lua_State*, const index_map&) {}
		static void postcall(lua_State*, const index_map&) {}

		struct only_accepts_nonconst_pointers {};
		struct can_only_convert_from_lua_to_cpp {};

		template<class T, class Direction>
		struct apply
		{
			typedef typename boost::mpl::if_<boost::is_same<lua_to_cpp, Direction>
				, vararg_converter_lua_to_cpp<N, Policies>
			>::type type;
		};
	};

}}

namespace luabind
{
	template<int N>
	detail::policy_cons<detail::vararg_policy<N, detail::null_type>, detail::null_type> 
	vararg(LUABIND_PLACEHOLDER_ARG(N)) 
	{ 
		return detail::policy_cons<detail::vararg_policy<N, detail::null_type>, detail::null_type>(); 
	}

	template<int N, class Policies>
	detail::policy_cons<detail::vararg_policy<N, Policies>, detail::null_type> 
	vararg(LUABIND_PLACEHOLDER_ARG(N), const Policies&) 
	{ 
		return detail::policy_cons<detail::vararg_policy<N, Policies>, detail::null_type>(); 
	}
}

#endif // LUABIND_VARARG_CONTAINER_POLICY_HPP_INCLUDED
