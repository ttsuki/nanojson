# nanojson (v3) - Simple json reader/writer for C++17

🌟 Single C++ header only. ALL in [nanojson3.h](nanojson3.h).

This project is an experimental, proof-of-concept, implementation of json library using modern C++17 functions.

This is the version 3 of the nanojson library, which is wrote from scratch.
There is no API compatibility from previous version of the nanojson.
The previous version is in `v1` or `v2` branch.

## 🌟 License
[MIT License. (c) 2016-2023 ttsuki](LICENSE)

## 🌟 The Concept ⚡

This library provides simply a json implementation type `json` such as

```cpp
// psudo-code

class json
{
    using js_undefined = ...;
    using js_null = std::nullptr_t;
    using js_boolean = bool;
    using js_integer = long long;
    using js_floating = long double;
    using js_string = std::string;
    using js_array = std::vector<json>;
    using js_object = std::map<js_string, json>;
    
    // member
    std::variant<
        js_undefined,
        js_null,
        js_boolean,
        js_integer,
        js_floating,
        js_string,
        js_array,
        js_object> value_;

    // with accessors,
    bool    operator -> is_*();  // value accessor
    value&  operator -> get_*(); // value accessor
    json& operator [int]       // array child reference
    json& operator [string]    // object child reference

    // and some useful helper objects/functions.
    class json_reader; // parse from string or istream.
    class json_writer; // write  to  string or ostream.
    json(...);         // constructor overloads for vary types.
};
```
## 🌟 Sample Code Snippets

😃 Here, some snippets may be useful to learn usage of this library.

[nanojson3.samples.cpp](nanojson3.samples.cpp)

### 🌟 Simple iostream/string i/o interface.

```cpp
std::cout << njs3::json_out_pretty << njs3::json::parse(R"([123, 456, "abc"])") << "\n";
```

```json
[
  123,
  456,
  "abc"
]
```

```cpp
njs3::json json;
istream >> json;                      // parse input
std::cout << njs3::json_out_pretty << json; // output pretty
```

### 🌟 Some loose parse option by flags.

👇 input (parse with some flags)

```js
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
```
```cpp
std::cout << njs3::json_out_pretty << parse_json(
    src
    , njs3::json_parse_option::default_option            // default allows utf-8 bom, unescaped forward slash '/'
    | njs3::json_parse_option::allow_comment             // allows block/line comments
    | njs3::json_parse_option::allow_trailing_comma      // allows comma following last element
    | njs3::json_parse_option::allow_unquoted_object_key // allows naked object key
    // or simply ` njs3::json_parse_option::all` enables all loose option flags.
);
```

👇 output `.json` is
```json
{
  "comments": [
    "not comment0",
    "not comment1",
    "not comment2",
    "not comment3",
    "not comment4",
    "not comment5",
    "not comment6",
    "not comment7",
    "not comment8"
  ],
  "naked_key": "hello world"
}
```

### 🌟 Basic Read/Write Access To Json Object

👇 input
````cpp
njs3::json json = njs3::parse_json(R""(
````
```js
{
    "null_literal" : null,
    "bool_true" : true,
    "bool_false" : false,
    "integer" : 1234567890123456789, // parsed to js_integer 1234567890123456789
    "float1" : 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890, // parsed to js_floating 1.234568e+119
    "float2" : 1.234567e+89, // parsed to js_floating 1.234567e+89,
    "strings" : {
        "a": "a",
        "にほんご": "\/\/あいう\n\tえお",
        "⚡": "\u26a1", // \u???? characters will be decoded to utf-8 sequence.
        "😃": "\uD83D\uDE03", // surrogate pairs
        "aåनि亜𐂃": "\u0061\u0061\u030A\u0928\u093F\u4E9C\uD800\uDC83"
    },
    "test_array": [1, 2, 3, "a", "b", "c"]
}
```
````cpp
)"", njs3::json_parse_option::allow_comment);
std::cout << njs3::json_out_pretty << json;
````

👇 The Output `.json` is

```json
{
  "bool_false": false,
  "bool_true": true,
  "float1": 1.23456789e+119,
  "float2": 1.234567e+89,
  "integer": 1234567890123456789,
  "null_literal": null,
  "strings": {
    "a": "a",
    "aåनि亜𐂃 ": "aåनि亜𐂃 ",
    "⚡": "⚡",
    "にほんご": "\/\/あいう\n\tえお",
    "😃": "😃"
  },
  "test_array": [
    1,
    2,
    3,
    "a",
    "b",
    "c"
  ]
}
```
※ Note: Numbers are parsed into integer or floating-point types,
         the type is determined by their value range and representation.

👇 And that `json` object can be accessed by `operator[]` and `operator->` ...

```cpp
//.cpp
#define DEBUG_OUTPUT(...) (#__VA_ARGS__) << " => " << (__VA_ARGS__) << "\n"

// Writing access:

json["this"] = "is ok."; // will create new property `"this": "is ok."` into json.

// Writing to undefined reference throws `bad_access` exception.
try
{
    json["this"]["node"] = 123; // throws bad_access: invalid reference
}
catch (const njs3::bad_access& x)
{
    std::cerr << x.what() << std::endl;
}

// Reading access:

njs3::js_integer integer = json["integer"]->get_integer(); // OK
//njs3::js_floating integer = json["integer"]->get_floating(); // throws bad_access (type mismatch)
std::cout << DEBUG_OUTPUT(integer);

njs3::js_floating float1 = json["float1"]->get_floating(); // OK
//njs3::js_integer float1 = json["float1"]->get_integer();   // throws bad_access (type mismatch)
std::cout << DEBUG_OUTPUT(float1);

njs3::js_floating integer_as_number = json["integer"]->get_number(); // OK (converted to js_floating)
njs3::js_floating float1_as_number = json["float1"]->get_number();   // OK
njs3::js_floating float2_as_number = json["float2"]->get_number();   // OK
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
catch (const njs3::bad_access& x)
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

```

### 🌟 Making JSON Values From Scratch
```cpp
//.cpp
// Makes array from values
njs3::json json = njs3::js_array{1, 2, 3, "a", true, false, 4.5, nullptr};
if (auto a = json->as_array()) // gets js_array interface (is simply std::vector<json>
{
    a->push_back(123);
    a->push_back("abc");
}
std::cout << njs3::json_out_pretty << json << std::endl;
```

```cpp
//.cpp
njs3::json json = njs3::js_object{
    {"a", 1},
    {"b", 2},
    {"c", njs3::js_array{"X", "Y", "Z", 1, 2, 3}},
};
if (auto a = json->as_object()) // gets js_object interface (is simply std::map<js_string, json>)
{
    a->insert_or_assign("d", 12345);
    a->insert_or_assign("e", "abc");
    a->insert_or_assign("f", njs3::js_object{{"f1", 123}, {"f2", 456}, {"f3", 789},});
}
std::cout << njs3::json_out_pretty << json << std::endl;
```

### 🌟 Making JSON Values From STL Containers

👇 Let's serialize STL containers into `json`.

```cpp
//.cpp
{
    // Makes array of array from STL containers
    njs3::json json = std::vector<std::vector<float>>
    {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    std::cout << njs3::json_out_pretty << json << std::endl;

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
    // std::map<string, ...> is converted json::js_object
    njs3::json json = std::map<std::string, int>{{"a", 1}, {"b", 2}};
    std::cout << njs3::json_out_pretty << json << std::endl;
    // makes { "a": 1, "b": 2 }
}
```

### 🌟 Serializing User Defined Types Into `json`

👇 Let's serialize user-defined type into `json`.

```cpp
//.cpp
{
    // example User defined type.
    struct custom_struct
    {
        std::string title{};
        int value{};

        // returns json-formated string (or simply `nanojson3::json`)
        [[nodiscard]] std::string to_json() const
        {
            return njs3::json(njs3::js_object{
                {"title", title},
                {"value", value},
            }).serialize();
        }
    };

    //  Converts from user-defined struct by member function such 
    //    - string to_json() const;
    //    - json to_json() const;
    //  or non-member functions searched global/ADL such 
    //    - string to_json(s); 
    //    - json to_json(s);

    njs3::json test = custom_struct{"the answer", 42};
    std::cout << DEBUG_OUTPUT(test);

    // Mix use with json_convertible objects.
    njs3::json json = std::array<custom_struct, 2>{
        {
            {"the answer", 42},
            {"the answer squared", 42 * 42},
        }
    };
    // std::array of json convertible type is converted into json::js_array

    auto ref = json->as_array(); // get js_array interface (is simply std::vector<json>)
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
    std::cout << njs3::json_out_pretty << DEBUG_OUTPUT(json);

    // tuple is converted into array
    njs3::json json2 = std::tuple<int, double, custom_struct>{42, 42.195, {"hello", 12345}};
    std::cout << njs3::json_out_pretty << DEBUG_OUTPUT(json2);
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
```

😕.o( if a user-defined type is in another library and it cannot be changed, what should I do? )



### 🌟 Adding User-defined JSON Serializer (User-defined JSON Constructor Extension)
The nanojson provides constructor extension interface.


👇 Here are unchangeable `Vector3f` and `Matrix3x3f` provided by another library.

```cpp
//.cpp
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
```

👇 You can specialize the `json_serializer` class template to serialize `Vector3f` into `json`.

The prototype of `json_serializer` is

```cpp
// cpp
namespace nanojson3
{
    template <class T, class U = void> // T is source type.
    struct json_serializer             // U is placeholder for specializations. (std::enable_if_t or std::void_t)
    {
        static json serialize(T value) { return /* implement here! */; }
    };
}
```

Making such specialization for type `T` makes `json(T)` constructor callable.

Let `json_serializer<Vector3f>` as

```cpp
// cpp

// Vector3f JSON serializer
template <>
struct njs3::json_serializer<foobar_library::Vector3f>
{
    static json serialize(const foobar_library::Vector3f& val)
    {
        return njs3::js_object
        {
            {"x", val.x},
            {"y", val.y},
            {"z", val.z},
        };
    }
};
```

👇 Or, just implement `to_json(T)` ADL or global function also makes `json(T)` constructor callable.
(but it may cause conflicts with the original `foobar_library`'s functions).

```cpp
// cpp

namespace foobar_library
{
    static njs3::json to_json(const Matrix3x3f& val)
    {
        // std::array<Vector3f, N> will be converted to `json` via another `json_serializer`.
        return std::array<Vector3f, 3>{val.row0, val.row1, val.row2};
    }
}
```

👇 then we can use it.

```cpp
//.cpp

void fixed_user_defined_types()
{
    const foobar_library::Vector3f input = {1.0f, 2.0f, 3.0f};

    // Convert Vector3f into json by `json_serializer<Vector3f>`
    njs3::json json_from_vector3f = input;
    std::cout << std::fixed << DEBUG_OUTPUT(json_from_vector3f);
    // makes output like {"x":1.0000,"y":2.0000,"z":3.0000}.

    // Convert Matrix3x3f into json by `foobar_library::to_json`
    njs3::json json_from_matrix3x3f = std::vector<foobar_library::Matrix3x3f>{
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
    std::cout << std::fixed << std::setprecision(3) << njs3::json_out_pretty << DEBUG_OUTPUT(json_from_matrix3x3f);
    std::cout << std::scientific << std::setprecision(16) << njs3::json_out_pretty << DEBUG_OUTPUT(json_from_matrix3x3f);
}
```


### 🌟 Built-in Json serializer extensions

The nanojson provides some built-in specializations and helper classes (in `nanojson3.h`).

The importing functions for STL objects described above are implemented as constructor extensions.

  - built-in `json_serializer` for primitives:
    - map all integral types (except `char`) to `js_integer`
    - map all floating-point types to `js_floating`
    - map `const char*` to `js_string`
    - map `container<char>` to `js_string`
  - built-in `json_serializer` for STL containers:
    - map `container<T>` to `js_array` if `T` is json-convertible.
    - map `std::tuple<U...>` to `js_array` if all of `U...` is json-convertible.
    - map `container<[K,V]>` to `js_object` if `K` is js_object_key-convertible and `V` is json-convertible.
  - built-in `json_serializer` for User-defined types:
    - map `T` to `json` if `T` has member function `json_string T::to_json() const`
    - map `T` to `json` if there is ADL/global function `json_string to_json(T)`
    - map `T` to `json` if `T` has member function `json T::to_json() const`
    - map `T` to `json` if there is ADL/global function `json to_json(T)`

### 🌟 EOF

😃 Have fun.
