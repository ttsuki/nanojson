/** @file
 * nanojson: A Simple JSON Reader/Writer For C++17
 * Copyright (c) 2016-2022 ttsuki
 * This software is released under the MIT License.
 */

#include "nanojson3.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include <array>
#include <string>
#include <tuple>
#include <map>
#include <vector>

#define DEBUG_OUTPUT(...) (#__VA_ARGS__) << " => " << (__VA_ARGS__) << "\n"

void sample_code_snippets()
{
    //  ## 🌟 Sample Code Snippets
    //  😃 Here, some snippets may be useful to learn usage of this library.
    using namespace nanojson3;

    std::cout << std::fixed;

    //  ### 🌟 Simple iostream/string i/o interface.
    std::cout << json_out_pretty << json::parse(R"([123, 456, "abc"])") << "\n";
    //[
    //  123,
    //  456,
    //  "abc"
    //]

    {
        std::stringstream cin{R"([123, 456, "abc"])"};
        std::istream& istream = cin; // std::cin

        json json;
        istream >> json;                      // parse input
        std::cout << json_out_pretty << json; // output pretty
    }

    //  ### 🌟 Some loose parse option by flags.
    {
        //  👇 input(parse with some loose_option flags)
        auto src = R""(
// loose json
{
// in LOOSE MODE, block/line comments are allowed.
  "comments": [ "not comment0"
    ,"not comment1" // line comment // ," still line comment" */ ," still line comment" /*
    ,"not comment2" /*** block comment ***/ ,"not comment3"
    /*//*//** */ ,"not comment4" /* block comment 
    // still in block comment **/ ,"not comment5" // line comment */ still line comment
    /*/, "comment"
    /*/, "not comment6"
    /*/, "block comment"
    /*/, "not comment7"
    //*/, "line comment"
    ,"not comment8"
  ],
  naked_key: "hello world" // in LOOSE MODE, non-quoted keys are allowed.
  , // in LOOSE MODE, trailing comma is allowed.
}
)"";
        std::cout << json_out_pretty << parse_json(
            src
            , json_parse_option::default_option            // default allows utf-8 bom, unescaped forward slash '/'
            | json_parse_option::allow_comment             // allows block/line comments
            | json_parse_option::allow_trailing_comma      // allows comma following last element
            | json_parse_option::allow_unquoted_object_key // allows naked object key
            // or simply `json_reader::loose_option::all` enables all loose option flags.
        );

        //  makes 👇 output.json is
        //  {
        //    "comments": [
        //      "not comment0",
        //      "not comment1",
        //      "not comment2",
        //      "not comment3",
        //      "not comment4",
        //      "not comment5",
        //      "not comment6",
        //      "not comment7",
        //      "not comment8"
        //    ],
        //    "naked_key": "hello world"
        //  }
    }

    // ### 🌟 Basic Read/Write Access To Json Object
    {
        // 👇 Sample Input
        json json = parse_json(
            R""(
{
    "null_literal" : null,
    "bool_true" : true,
    "bool_false" : false,
    "integer" : 1234567890123456789, // parsed to js_integer 1234567890123456789
    "float1" : 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890, // parsed to float_t 1.234568e+119
    "float2" : 1.234567e+89, // parsed to float_t 1.234567e+89,
    "strings" : {
        "a": "a",
        "にほんご": "\/\/あいう\n\tえお",
        "⚡": "\u26a1", // \u???? characters will be decoded to utf-8 sequence.
        "😃": "\uD83D\uDE03", // surrogate pairs
        "aåनि亜𐂃": "\u0061\u0061\u030A\u0928\u093F\u4E9C\uD800\uDC83"
    },
    "test_array": [1, 2, 3, "a", "b", "c"]
}
)"", json_parse_option::allow_comment);

        //  👇 makes parsed.json 
        //  {
        //    "bool_false": false,
        //    "bool_true": true,
        //    "float1": 1.23456789e+119,
        //    "float2": 1.234567e+89,
        //    "integer": 1234567890123456789,
        //    "null_literal": null,
        //    "strings": {
        //      "a": "a",
        //      "aåनि亜𐂃 ": "aåनि亜𐂃 ",
        //      "⚡": "⚡",
        //      "にほんご": "\/\/あいう\n\tえお",
        //      "😃": "😃"
        //    },
        //    "test_array": [
        //      1,
        //      2,
        //      3,
        //      "a",
        //      "b",
        //      "c"
        //    ],
        //    "this": "is ok."
        //  }

        //  Note
        //  - json object is implemented by `std::map<js_string, json_t>` internally, so its properties are sorted by key.
        //  - Numbers into `js_integer (long long)` or `float_t (long double)` type by value.

        //  👇 And that `json` object can be accessed by `operator[]` and `operator->` ...

        // Writing access:

        json["this"] = "is ok."; // will create new property `"this": "is ok."` into json.

        // Writing to undefined reference throws `bad_access` exception.
        try
        {
            json["this"]["node"] = 123; // throws bad_access: invalid reference
        }
        catch (const bad_access& x)
        {
            std::cerr << x.what() << std::endl;
        }

        // Reading access:

        js_integer integer = json["integer"]->get_integer(); // OK
        //js_floating integer = json["integer"]->get_floating(); // throws bad_access (type mismatch)
        std::cout << DEBUG_OUTPUT(integer);

        js_floating float1 = json["float1"]->get_floating(); // OK
        //js_integer float1 = json["float1"]->get_integer();   // throws bad_access (type mismatch)
        std::cout << DEBUG_OUTPUT(float1);

        js_floating integer_as_number = json["integer"]->get_number(); // OK (converted to js_floating)
        js_floating float1_as_number = json["float1"]->get_number();   // OK
        js_floating float2_as_number = json["float2"]->get_number();   // OK
        std::cout << DEBUG_OUTPUT(integer_as_number);
        std::cout << DEBUG_OUTPUT(float1_as_number);
        std::cout << DEBUG_OUTPUT(float2_as_number);

        //  😃.o(`operator []` is used to reference node, `operator ->` is used to reference its value.)

        std::cout << DEBUG_OUTPUT(json["strings"]["にほんご"]->get_string()); // "//あいう\n\tえお"
        // std::cout << DEBUG_OUTPUT(json["strings"]["not defined value"]->get_string()); // throws bad_access (no such key.)
        std::cout << DEBUG_OUTPUT(json["strings"]["not defined value"]->get_string_or("failed")); // "failed": get_*_or method doesn't throw exception, returns argument `default_value` instead.

        // type-mismatched get access throws bad_access.
        try
        {
            (void)json["this"]->get_integer();        // throws bad_access: json["this"] is string.
            (void)json["this"]["foobar"]->get_null(); // throws bad_access: json["this"]["foobar"] is undefined (not a null).
        }
        catch (const bad_access& x)
        {
            std::cerr << x.what() << std::endl;
        }

        std::cout << DEBUG_OUTPUT(json["strings"]->get_string_or("failed")); // "failed": type mismatch json is not string.

        // Testing node existence:

        std::cout << DEBUG_OUTPUT(json->is_defined()); // true
        std::cout << DEBUG_OUTPUT(json->is_array());   // false
        std::cout << DEBUG_OUTPUT(json->is_object());  // true

        // Only referencing undefined node is not error.
        std::cout << DEBUG_OUTPUT(json["aaaa"]->is_defined());              // false: no such key.
        std::cout << DEBUG_OUTPUT(json["test_array"][12345]->is_defined()); // false: index is out of range.

        std::cout << DEBUG_OUTPUT(json["this"]->is_defined());                         // true
        std::cout << DEBUG_OUTPUT(json["this"]["node"]->is_defined());                 // false: doesn't emit bad_access
        std::cout << DEBUG_OUTPUT(json["Non-existent node"]["a child"]->is_defined()); // false: doesn't emit bad_access

        std::cout << json_out_pretty << json << std::endl;
    }

    //  ### 🌟 Making JSON Values From Scratch
    {
        // Makes array from values
        json json = js_array{1, 2, 3, "a", true, false, 4.5, nullptr};
        if (auto a = json->as_array()) // gets js_array interface (is simply std::vector<json_t>
        {
            a->push_back(123);
            a->push_back("abc");
        }
        std::cout << json_out_pretty << json << std::endl;
    }
    {
        json json = js_object{
            {"a", 1},
            {"b", 2},
            {"c", js_array{"X", "Y", "Z", 1, 2, 3}},
        };
        if (auto a = json->as_object()) // gets js_object interface (is simply std::map<js_string, json_t>)
        {
            a->insert_or_assign("d", 12345);
            a->insert_or_assign("e", "abc");
            a->insert_or_assign("f", js_object{{"f1", 123}, {"f2", 456}, {"f3", 789},});
        }
        std::cout << json_out_pretty << json << std::endl;
    }

    //  ### 🌟 Making JSON Values From STL Containers
    //  👇 Let's serialize STL containers into `json_t`.
    {
        // Makes array of array from STL containers
        json json = std::vector<std::vector<float>>
        {
            {1, 2, 3},
            {4, 5, 6},
            {7, 8, 9}
        };
        std::cout << json_out_pretty << json << std::endl;

        // `get_*` method assumes type is array. if not, throws bad_access
        for (auto&& row : json->get_array())
        {
            // `get_*_or` method checks type is array. if not, returns default value in argument
            for (auto&& column : row->get_array_or({}))
                std::cout << "  " << column;
            std::cout << "\n";
        }
    }

    {
        // std::map<string, ...> is converted json_t::js_object
        json json = std::map<std::string, int>{{"a", 1}, {"b", 2}};
        std::cout << json_out_pretty << json << std::endl;
        // makes { "a": 1, "b": 2 }
    }

    //  ### 🌟 Serializing User Defined Types Into `json_t`
    //  👇 Let's serialize user-defined type into `json_t`.
    {
        // example User defined type.
        struct custom_struct
        {
            std::string title{};
            int value{};

            // returns json string (or json_t)
            [[nodiscard]] std::string to_json() const
            {
                using namespace nanojson3;
                return json(js_object{
                    {"title", title},
                    {"value", value},
                }).serialize();
            }
        };

        //  Converts from user-defined struct by member function such 
        //    - string to_json() const;
        //    - json_t to_json() const;
        //  or non-member functions searched global/ADL such 
        //    - string to_json(s); 
        //    - json_t to_json(s);

        json test = custom_struct{"the answer", 42};
        std::cout << DEBUG_OUTPUT(test);

        // Mix use with json_convertible objects.
        json json1 = std::array<custom_struct, 2>{
            {
                {"the answer", 42},
                {"the answer squared", 42 * 42},
            }
        };
        // std::array of json convertible type is converted into json_t::js_array

        auto ref = json1->as_array(); // get js_array interface (is simply std::vector<json_t>)
        ref->emplace_back(custom_struct{"the answer is", 43});
        ref->emplace_back(custom_struct{"the answer is", 44});
        ref->emplace_back(custom_struct{"the answer is", 45});
        //  makes
        //  [
        //      {"title": "the answer", "value": 42},
        //      {"title": "the answer squared", "value": 1764},
        //      {"title": "the answer is", "value": 43},
        //      {"title": "the answer is", "value": 44},
        //      {"title": "the answer is", "value": 45}
        //  ]
        std::cout << json_out_pretty << DEBUG_OUTPUT(json1);

        // tuple is converted into array
        json json2 = std::tuple<int, double, custom_struct>{42, 42.195, {"hello", 12345}};
        std::cout << json_out_pretty << DEBUG_OUTPUT(json2);
        // makes
        // [
        //    42,
        //    42.195, 
        //    {
        //      "title": "hello"
        //      "value": 12345,
        //    }
        // ]
    }

    //  😕.o( if a user-defined type is in another library and it cannot be changed, what should I do? )
    extern void fixed_user_defined_types();
    fixed_user_defined_types();
}

//  ### 🌟 Adding User-defined Json Serializer (User-defined json Constructor Extension)
//  The nanojson provides constructor extension interface.

//  👇 Here are unchangeable `Vector3f` and `Matrix3x3f` provided by another library.
namespace foobar_library
{
    struct Vector3f final
    {
        float x, y, z;
    };

    struct Matrix3x3f final
    {
        Vector3f row0, row1, row2;
    };
}

//  👇 You can specialize the `json_serializer` class template to serialize `Vector3f` into json.
//
//  The prototype of `json_serializer` is
//
//  namespace nanojson3
//  {
//      template <class T, class U = void> // T is source type.
//      struct json_serializer             // U is placeholder for specializations. (std::enable_if_t or std::void_t)
//      {
//          static json serialize(T value) { return /* implement here! */; }
//      };
//  }
//
// Making such specialization for type `T` makes `json(T)` constructor callable.

// Let `json_serializer<Vector3f>` as
//
template <>
struct nanojson3::json_serializer<foobar_library::Vector3f>
{
    static json serialize(const foobar_library::Vector3f& val)
    {
        return js_object
        {
            {"x", val.x},
            {"y", val.y},
            {"z", val.z},
        };
    }
};

// 👇 Or, just implement `to_json(T)` ADL or global function also makes `json(T)` constructor callable.
// (but it may cause conflicting with function in original `foobar_library`)
namespace foobar_library
{
    static nanojson3::json to_json(const foobar_library::Matrix3x3f& val)
    {
        // std::array<Vector3f, N> will be converted to `json` via another `json_serializer`.
        return std::array<foobar_library::Vector3f, 3>{val.row0, val.row1, val.row2};
    }
}

//  👇 then we can use it.

void fixed_user_defined_types()
{
    using namespace nanojson3;
    const foobar_library::Vector3f input = {1.0f, 2.0f, 3.0f};

    // Convert Vector3f into json by `json_serializer<Vector3f>`
    json json_from_vector3f = input;
    std::cout << std::fixed << DEBUG_OUTPUT(json_from_vector3f);
    // makes output like {"x":1.0000,"y":2.0000,"z":3.0000}.

    // Convert Matrix3x3f into json by `foobar_library::to_json`
    json json_from_matrix3x3f = std::vector<foobar_library::Matrix3x3f>{
        {
            {1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f},
            {7.0f, 8.0f, 9.0f},
        },
        {
            {100.1f, 200.2f, 300.3f},
            {400.4f, 500.5f, 600.6f},
            {700.7f, 800.8f, 900.9f},
        },
    };

    // Output float format can be set by i/o manipulators.
    std::cout << std::fixed << std::setprecision(3) << json_out_pretty << DEBUG_OUTPUT(json_from_matrix3x3f);
    std::cout << std::scientific << std::setprecision(16) << json_out_pretty << DEBUG_OUTPUT(json_from_matrix3x3f);
}

//  ### 🌟 EOF
//  😃 Have fun.

// main

#ifdef _WIN32
#include <Windows.h>
#endif

static void more_test();

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // set codepage to utf-8.
#endif
    std::cout << std::boolalpha << std::fixed << std::setprecision(9);
    sample_code_snippets();
    more_test();
}

static void more_test()
{
    using namespace nanojson3;
    try
    {
        for (auto i : json::parse(R"(
[
1,
1234567890,
12345678901234567890,
1234567890123456789012345678901234567890,
12345678901234567890123456789012345678901234567890123456789012345678901234567890,
1e10,
1e100,
1e1000,
1e10000,
1e-1,
1e-10,
1e-100,
1e-1000,
1e-10000,
12345.67890,
1.234567890,
0.1234567890,
0.0000000001234567890,
0.0000000001234567890E+10,
0.12345678901234567890123456789012345678901234567890123456789012345678901234567890,
0.12345678901234567890123456789012345678901234567890123456789012345678901234567890e80,
0.12345678901234567890123456789012345678901234567890123456789012345678901234567890e+80,
0.12345678901234567890123456789012345678901234567890123456789012345678901234567890e-80,
0.001e309,
1.000e309
])")->get_array())
        {
            std::cout << std::defaultfloat << std::setprecision(24)
                << std::setw(32) << i->get_number()
                << std::setw(32) << i << std::endl;
        }
    }
    catch (const nanojson_exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
