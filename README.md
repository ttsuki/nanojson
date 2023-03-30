# nanojson (v3) - Simple json reader/writer for C++17

🌟 Single C++ header only. ALL in [nanojson3.h](nanojson3.h).

This project is an experimental, proof-of-concept, implementation of json library using modern C++17 functions.

This is the version 3 of the nanojson library, which is wrote from scratch.
There is no API compatibility from previous version of the nanojson.
The previous version is in `v1` or `v2` branch.

## 🌟 License
[MIT License. (c) 2016-2023 ttsuki](LICENSE)

## 🌟 The Concept ⚡

This library provides simply a json implementation type `json_t` such as

```cpp
class json_t
{
    using undefined_t = ...;
    using null_t = std::nullptr_t;
    using bool_t = bool;
    using integer_t = long long;
    using floating_t = long double;
    using string_t = std::string;
    using array_t = std::vector<json_t>;
    using object_t = std::map<string_t, json_t>;
    
    // member
    std::variant<
        undefined_t,
        null_t,
        bool_t,
        integer_t,
        floating_t,
        string_t,
        array_t,
        object_t> value_;

    // with accessors,
    bool    operator -> is_*();  // value accessor
    value&  operator -> get_*(); // value accessor
    json_t& operator [int]       // array child reference
    json_t& operator [string]    // object child reference

    // and some useful helper objects/functions.
    class json_reader; // parse from string or istream.
    class json_writer; // write  to  string or ostream.
    json_t(...);       // many importing constructor overloads.
};
```
## 🌟 Sample Code Snippets

😃 Here, some snippets may be useful to learn usage of this library.

[nanojson3.samples.cpp](nanojson3.samples.cpp)

### 🌟 Simple iostream/string i/o interface.

```cpp
using namespace nanojson3;
std::cout << json_ios_pretty << json_t::parse(R"([123, 456, "abc"])") << "\n";
```

```json
[
  123,
  456,
  "abc"
]
```

```cpp
json_t json;
std::cin >> json;                     // parse input
std::cout << json_ios_pretty << json; // output pretty
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
std::cout << json_ios_pretty << json_reader::parse_json(
    src
    , json_reader::option::default_option            // default allows utf-8 bom, unescaped forward slash '/'
    | json_reader::option::allow_comment             // allows block/line comments
    | json_reader::option::allow_trailing_comma      // allows comma following last element
    | json_reader::option::allow_unquoted_object_key // allows naked object key
    // or simply `json_reader::loose_option::all` enables all loose option flags.
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
json_t json = json_reader::parse_json(R""(
````
```js
{
    "null_literal" : null,
    "bool_true" : true,
    "bool_false" : false,
    "integer" : 1234567890123456789, // parsed to integer_t 1234567890123456789
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
```
````cpp
)"", json_reader::loose_option::allow_comment);
std::cout << json_ios_pretty << json;
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
Note
- json object is implemented by `std::map<string_t, json_t>` internally, so its properties are sorted by key.
- Numbers into `integer_t (long long)` or `float_t (long double)` type by value.

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
catch (const bad_access& x)
{
    std::cerr << x.what() << std::endl;
}

// Reading access:

integer_t integer = json["integer"]->get_integer(); // OK
//floating_t integer = json["integer"]->get_floating(); // throws bad_access (type mismatch)
std::cout << DEBUG_OUTPUT(integer);

floating_t float1 = json["float1"]->get_floating(); // OK
//integer_t float1 = json["float1"]->get_integer();   // throws bad_access (type mismatch)
std::cout << DEBUG_OUTPUT(float1);

floating_t integer_as_number = json["integer"]->get_number(); // OK (converted to floating_t)
floating_t float1_as_number = json["float1"]->get_number();   // OK
floating_t float2_as_number = json["float2"]->get_number();   // OK
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

```

### 🌟 Making JSON Values From Scratch
```cpp
//.cpp
// Makes array from values
json_t json = array_t{1, 2, 3, "a", true, false, 4.5, nullptr};
if (auto a = json->as_array()) // gets array_t interface (is simply std::vector<json_t>
{
    a->push_back(123);
    a->push_back("abc");
}
std::cout << json_ios_pretty << json << std::endl;
```

```cpp
//.cpp
json_t json = object_t{
    {"a", 1},
    {"b", 2},
    {"c", array_t{"X", "Y", "Z", 1, 2, 3}},
};
if (auto a = json->as_object()) // gets object_t interface (is simply std::map<string_t, json_t>)
{
    a->insert_or_assign("d", 12345);
    a->insert_or_assign("e", "abc");
    a->insert_or_assign("f", object_t{{"f1", 123}, {"f2", 456}, {"f3", 789},});
}
std::cout << json_ios_pretty << json << std::endl;
```

### 🌟 Making JSON Values From STL Containers

👇 Let's serialize STL containers into `json_t`.

```cpp
//.cpp
{
    // Makes array of array from STL containers
    json_t json = std::vector<std::vector<float>>
    {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    std::cout << json_ios_pretty << json << std::endl;

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
    // std::map<string, ...> is converted json_t::object_t
    json_t json = std::map<std::string, int>{{"a", 1}, {"b", 2}};
    std::cout << json_ios_pretty << json << std::endl;
    // makes { "a": 1, "b": 2 }
}
```

### 🌟 Serializing User Defined Types Into `json_t`

👇 Let's serialize user-defined type into `json_t`.

```cpp
//.cpp
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
            return json_t(object_t{
                {"title", title},
                {"value", value},
            }).to_json_string();
        }
    };

    //  Converts from user-defined struct by member function such 
    //    - string to_json() const;
    //    - json_t to_json() const;
    //  or non-member functions searched global/ADL such 
    //    - string to_json(s); 
    //    - json_t to_json(s);

    json_t test = custom_struct{"the answer", 42};
    std::cout << DEBUG_OUTPUT(test);

    // Mix use with json_convertible objects.
    json_t json = std::array<custom_struct, 2>{
        {
            {"the answer", 42},
            {"the answer squared", 42 * 42},
        }
    };
    // std::array of json convertible type is converted into json_t::array_t

    auto ref = json->as_array(); // get array_t interface (is simply std::vector<json_t>)
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
    std::cout << json_ios_pretty << DEBUG_OUTPUT(json);

    // tuple is converted into array
    json_t json2 = std::tuple<int, double, custom_struct>{42, 42.195, {"hello", 12345}};
    std::cout << json_ios_pretty << DEBUG_OUTPUT(json2);
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


### 🌟 Adding User-defined Json Importer (User-defined json_t Constructor Extension)
The nanojson provides constructor extension interface.

This is an additional json importer which converts `std::tuple<...>` into `array_t`. (defined in nanojson3.h)
```cpp
//.cpp

// map `std::tuple<U...>` to `array_t`
template <class...U>
struct json_t::json_ext<
    std::tuple<U...>,
    std::enable_if_t<std::conjunction_v<std::is_constructible<json_t, U>...>>
    >
{
    static array_t serialize(const std::tuple<U...>& val) {
        return std::apply(
            [](auto&& ...x) {
                return array_t{{json_t(std::forward<decltype(x)>(x))...}};
            },
            std::forward<decltype(val)>(val));
    }
};
```

The prototype of `json_t::json_ext` is
```cpp
//.cpp
   template <class T, class U = void> struct json_t::json_ext;
   // T is source type.
   // U is placeholder for void_t in specializations. (for std::enable_if_t or std::void_t)
```

Your task is implementing `static TYPE serialize(T) { ... }` which returns any of json types `null_t`, `integer_t`, `floating_t`, `string_t`, `array_t`, `object_t`, or `json_t` or another json convertible type.

If `json_t(T)` constructor can find out that specialization, type `T` can be convertible to json by `json_t` constructor.

👇 Here are unchangeable `Vector3f` and `Matrix3x3f` provided by another library.

```cpp
//.cpp

struct Vector3f final
{
    float x, y, z;
};

struct Matrix3x3f final
{
    Vector3f row0, row1, row2;
};
```

👇 You can write specialization of `json_ext` which serializes `Vector3f` into json_t

```cpp
//.cpp

// json constructor extension for `Vector3f`
template <>
struct nanojson3::json_t::json_ext<Vector3f>
{
    static auto serialize(const Vector3f& val)
    {
        //*/ // Serialize Vector3f into `object_t`
        return object_t
        {
            {"x", val.x},
            {"y", val.y},
            {"z", val.z},
        };
        /*/ // or simply `array_t`.
        return array_t{ val.x, val.y, val.z };
        //*/
    }
};

// json constructor extension for `Matrix3x3f`
template <>
struct nanojson3::json_t::json_ext<Matrix3x3f>
{
    static auto serialize(const Matrix3x3f& val)
    {
        // array<Vector3f, N> is json convertible with `json_ext<Vector3f>`.
        return std::array<Vector3f, 3>{val.row0, val.row1, val.row2};
    }
};
```
👇 then use it.
```cpp
//.cpp

{
    using namespace nanojson3;

    Vector3f input = {1.0f, 2.0f, 3.0f};
    json_t json_from_vector3f = input; // Convert Vector3f into json by `json_ext<Vector3f>`
    std::cout << std::fixed << DEBUG_OUTPUT(json_from_vector3f);
    // makes output like {"x":1.0000,"y":2.0000,"z":3.0000}.

    // With other json convertible containers.
    json_t json_from_matrix3x3f = std::vector<Matrix3x3f>{
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
    [[maybe_unused]] auto i = std::cout.flags() & std::ios_base::floatfield;
    std::cout
        << json_ios_pretty
        << DEBUG_OUTPUT(json_from_matrix3x3f);
}
```

### 🌟 Built-in constructor extensions

The nanojson provides some built-in specializations and helper classes (in `nanojson3.h`).

The importing functions for STL objects described above are implemented as constructor extensions.

- built-in ext ctor for primitives.
  - prevent `bool` constructor by pointer types (`std::is_pointer<T>`)
  - map all integral types (`char`, `int`, `unsigned long`, ...) to `int_t` (`std::is_integral<T>`)
  - map all floating point types (`float`, `double`, ...) to `float_t` (`std::is_floating_point<T>`)
  - map `const char*` to `string_t`
  - map `std::string_view` to `string_t`
- built-in ext ctor for array_t: map some STL container`<T>` to `array_t`, if `T` is convertible json.
  - map `std::initializer_list<T>` to `array_t`
  - map `std::vector<T>` to `array_t`
  - map `std::array<T, n>` to `array_t`
  - map `std::set<T>` to `array_t`
  - map `std::multiset<T>` to `array_t`
  - map `std::unordered_set<T>` to `array_t`
  - map `std::unordered_multiset<T>` to `array_t`
  - map `std::tuple<T...>` to `array_t`
- built-in ext ctor for object_t: map some STL container`<K, T>` to `object_t`, if `K` is convertible string and `T` is convertible json.
  - map `std::map<K, T>` to `object_t`
  - map `std::unordered_map<K, T>` to `object_t`
- built-in ext ctor for to_json types: some types which has `to_json()` function to `json_t`
  - enable_if `T` has `string T::to_json() const;`
  - enable_if `T` has `nanojson3::json_t T::to_json() const;`
  - enable_if `T` does not have `T::to_json() const;` and `string to_json(T);` is available  in global/ADL.
  - enable_if `T` does not have `T::to_json() const;` and `nanojson3::json_t to_json(T);` is available in global/ADL.

### 🌟 EOF

😃 Have fun.
