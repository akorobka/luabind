// Copyright (c) 2004 Daniel Wallin

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

#include "test.hpp"
#include <luabind/luabind.hpp>

using namespace luabind;

namespace {

struct base : counted_type<base>
{
    virtual ~base() {}

    virtual std::string f()
    {
        return "base:f()";
    }
};

struct base_wrap : base
{
    object self;
    base_wrap(object const& self_): self(self_) {}

    static std::string f_static(base* obj)
    {
        return obj->base::f();
    }

    virtual std::string f()
    {
        return call_member<std::string>(self, "f");
    }
};

struct simple_class : counted_type<simple_class>
{
    static int feedback;

    void f()
    {
        feedback = 1;
    }

    void f(int, int) {}
    void f(std::string a)
    {
        const char str[] = "foo\0bar";
        if (a == std::string(str, sizeof(str)-1))
            feedback = 2;
    }

    std::string g()
    {
        const char str[] = "foo\0bar";
        return std::string(str, sizeof(str)-1);
    }
};

int simple_class::feedback = 0;

} // namespace unnamed

#include <iostream>

void test_lua_classes()
{
    COUNTER_GUARD(base);
	COUNTER_GUARD(simple_class);

	lua_state L;

    module(L)
    [
        class_<base, base_wrap>("base")
            .def(constructor<>())
            .def("f", &base_wrap::f_static)
    ];

	DOSTRING(L,
		"function base:fun()\n"
		"  test = 4\n"
		"end\n"
		"a = base()\n"
		"a:fun()\n"
		"assert(test == 4)\n");

    DOSTRING(L, 
        "class 'derived' (base)\n"
        "  function derived:__init() super() end\n"
        "  function derived:f()\n"
        "    return 'derived:f() : ' .. base.f(self)\n"
        "  end\n"

        "function make_derived()\n"
        "  return derived()\n"
        "end\n");

    base* ptr;
    
    BOOST_CHECK_NO_THROW(
        ptr = call_function<base*>(L, "make_derived")
    );

    BOOST_CHECK_NO_THROW(
        ptr = call_function<base*>(L, "derived")
    );    

    BOOST_CHECK_NO_THROW(
        BOOST_CHECK(ptr->f() == "derived:f() : base:f()")
    );

	typedef void(simple_class::*f_overload1)();
	typedef void(simple_class::*f_overload2)(int, int);
	typedef void(simple_class::*f_overload3)(std::string);

    module(L)
    [
        class_<simple_class>("simple")
            .def(constructor<>())
			.def("f", (f_overload1)&simple_class::f)
			.def("f", (f_overload2)&simple_class::f)
			.def("f", (f_overload3)&simple_class::f)
			.def("g", &simple_class::g)
    ];

    DOSTRING(L,
        "class 'simple_derived' (simple)\n"
        "  function simple_derived:__init() super() end\n"
        "a = simple_derived()\n"
        "a:f()\n");
    BOOST_CHECK(simple_class::feedback == 1);

    DOSTRING(L, "a:f('foo\\0bar')");
    BOOST_CHECK(simple_class::feedback == 2);

    simple_class::feedback = 0;

    DOSTRING_EXPECTED(L, "a:f('incorrect', 'parameters')",
        "no overload of  'simple:f' matched the arguments "
        "(string, string)\ncandidates are:\n"
        "simple:f()\nsimple:f(number, number)\nsimple:f(string)\n");

    DOSTRING(L, "if a:g() == \"foo\\0bar\" then a:f() end");
    BOOST_CHECK(simple_class::feedback == 1);
}

