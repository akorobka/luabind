// Copyright Daniel Wallin 2007. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_POINTER_ITERATOR_POLICY__071111_HPP
# define LUABIND_POINTER_ITERATOR_POLICY__071111_HPP

# include <luabind/config.hpp>
# include <luabind/detail/policy.hpp>
# include <luabind/detail/convert_to_lua.hpp>

namespace luabind { namespace detail {

template <class Iterator>
struct pointer_iterator
{
    static int next(lua_State* L)
    {
        pointer_iterator* self = static_cast<pointer_iterator*>(
            lua_touserdata(L, lua_upvalueindex(1)));

        if (self->first != self->last)
        {
            convert_to_lua(L, &*self->first);
            ++self->first;
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }

    static int destroy(lua_State* L)
    {
        pointer_iterator* self = static_cast<pointer_iterator*>(lua_touserdata(L, 1));
        self->~pointer_iterator();
        return 0;
    }

    pointer_iterator(Iterator first, Iterator last)
      : first(first)
      , last(last)
    {}

    Iterator first;
    Iterator last;
};

template <class Iterator>
int make_pointer_range(lua_State* L, Iterator first, Iterator last)
{
    void* storage = lua_newuserdata(L, sizeof(pointer_iterator<Iterator>));
    lua_newtable(L);
    lua_pushcclosure(L, pointer_iterator<Iterator>::destroy, 0);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    lua_pushcclosure(L, pointer_iterator<Iterator>::next, 1);
    new (storage) pointer_iterator<Iterator>(first, last);
    return 1;
}

template <class Container>
int make_pointer_range(lua_State* L, Container& container)
{
    return make_pointer_range(L, container.begin(), container.end());
}

struct pointer_iterator_converter
{
    typedef pointer_iterator_converter type;

    template <class Container>
    void apply(lua_State* L, Container& container)
    {
        make_pointer_range(L, container);
    }

    template <class Container>
    void apply(lua_State* L, Container const& container)
    {
        make_pointer_range(L, container);
    }
};

struct pointer_iterator_policy : conversion_policy<0>
{
    static void precall(lua_State*, index_map const&)
    {}

    static void postcall(lua_State*, index_map const&)
    {}

    template <class T, class Direction>
    struct apply
    {
        typedef pointer_iterator_converter type;
    };
};

}} // namespace luabind::detail

namespace luabind { namespace {

LUABIND_ANONYMOUS_FIX detail::policy_cons<
    detail::pointer_iterator_policy, detail::null_type> return_stl_pointer_iterator;

}} // namespace luabind::unnamed

#endif // LUABIND_POINTER_ITERATOR_POLICY__071111_HPP

