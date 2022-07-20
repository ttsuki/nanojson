/** @file
 * nanojson: A Simple JSON Reader/Writer For C++17
 * Copyright (c) 2016-2022 ttsuki
 * This software is released under the MIT License.
 */

// https://github.com/ttsuki/nanojson
// This project is an experiment, proof-of-concept, implementation of json library on standard C++17 functions.

#pragma once
#ifndef NANOJSON2_H_INCLUDED
#define NANOJSON2_H_INCLUDED

#include <cstddef>
#include <cassert>
#include <cmath>

#include <limits>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <stdexcept>

#include <variant>
#include <optional>
#include <string>
#include <vector>
#include <map>

#include <istream>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <charconv>

#include <initializer_list>
#include <array>
#include <tuple>
#include <set>
#include <unordered_set>
#include <unordered_map>

#if !((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
#error "nanojson needs C++17 support."
#endif

#if !(defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L)
#error "nanojson needs C++17 Elementary string conversions (P0067R5) including Floating-Point (FP) values support.  See https://en.cppreference.com/w/cpp/compiler_support/17#:~:text=Elementary%20string%20conversions"
#endif

namespace nanojson2
{
    /// nanojson_exception: is base class of nanojson exceptions
    struct nanojson_exception : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    /// bad_access: represents invalid node access
    struct bad_access final : nanojson_exception
    {
        explicit bad_access() : nanojson_exception("bad_access") { }
        using nanojson_exception::nanojson_exception;
    };

    /// bad_format: represents FAILED to decode from json string.
    struct bad_format : nanojson_exception
    {
        using nanojson_exception::nanojson_exception;
    };

    /// bad_value: represents FAILED to encode to json string.
    struct bad_value : nanojson_exception
    {
        using nanojson_exception::nanojson_exception;
    };

    /// undefined_t: represents undefined.
    struct undefined_t
    {
        struct undefined_tag { };

        constexpr explicit undefined_t(undefined_tag) noexcept { }
        constexpr bool operator ==(undefined_t) const noexcept { return true; }
        constexpr bool operator !=(undefined_t) const noexcept { return false; }
    };

    /// json_t: represents a json element
    class json_t final
    {
    public:
        using char_t = char;

        using undefined_t = nanojson2::undefined_t;
        using null_t = std::nullptr_t;
        using bool_t = bool;
        using integer_t = long long int;
        using floating_t = long double;
        using string_t = std::basic_string<char_t>;
        using array_t = std::vector<json_t>;
        using object_t = std::map<string_t, json_t, std::less<>>;

        using number_t = floating_t;
        using string_view_t = std::basic_string_view<string_t::value_type>;
        using array_index_t = array_t::size_type;
        using object_key_view_t = string_view_t;

        using json_string_t = std::string;

        enum struct type_index
        {
            undefined_t,
            null_t,
            bool_t,
            integer_t,
            floating_t,
            string_t,
            array_t,
            object_t,
        };

        /// json_value_t: holds a json element value
        class json_value_t final
        {
        private:
            friend class json_t;

            static inline constexpr undefined_t value_of_undefined = undefined_t{undefined_t::undefined_tag{}};
            static inline constexpr null_t value_of_null = null_t{};

            std::variant<
                undefined_t,
                null_t,
                bool_t,
                integer_t,
                floating_t,
                string_t,
                array_t,
                object_t> value_ = value_of_undefined;

            constexpr json_value_t() = default;
            constexpr json_value_t(null_t value) : value_{value} { }
            constexpr json_value_t(bool_t value) : value_{value} { }
            constexpr json_value_t(integer_t value) : value_{value} { }
            constexpr json_value_t(floating_t value) : value_{value} { }
            json_value_t(string_t value) : value_{std::move(value)} { }
            json_value_t(array_t value) : value_{std::move(value)} { }
            json_value_t(object_t value) : value_{std::move(value)} { }
            json_value_t(const json_value_t& other) = default;
            json_value_t(json_value_t&& other) noexcept = default;
            json_value_t& operator=(const json_value_t& other) = default;
            json_value_t& operator=(json_value_t&& other) noexcept = default;
            ~json_value_t() = default;

        public:
            bool operator ==(const json_value_t& rhs) const noexcept { return value_ == rhs.value_; }
            bool operator !=(const json_value_t& rhs) const noexcept { return value_ != rhs.value_; }

            // pointer to value
            template <class T>
            struct pointer
            {
                T* const pointer_{};
                T& operator *() const { return pointer_ ? *pointer_ : throw bad_access(); }
                T* operator ->() const { return pointer_ ? pointer_ : throw bad_access(); }
                explicit operator bool() const noexcept { return pointer_; }
            };

            [[nodiscard]] type_index get_type() const noexcept { return static_cast<type_index>(value_.index()); }

            template <class T> [[nodiscard]] bool is() const noexcept { return std::holds_alternative<T>(value_); }
            [[nodiscard]] bool is_defined() const noexcept { return !is<undefined_t>(); }
            [[nodiscard]] bool is_undefined() const noexcept { return is<undefined_t>(); }
            [[nodiscard]] bool is_null() const noexcept { return is<null_t>(); }
            [[nodiscard]] bool is_bool() const noexcept { return is<bool_t>(); }
            [[nodiscard]] bool is_integer() const noexcept { return is<integer_t>(); }
            [[nodiscard]] bool is_floating() const noexcept { return is<floating_t>(); }
            [[nodiscard]] bool is_string() const noexcept { return is<string_t>(); }
            [[nodiscard]] bool is_array() const noexcept { return is<array_t>(); }
            [[nodiscard]] bool is_object() const noexcept { return is<object_t>(); }

            template <class T> [[nodiscard]] pointer<const T> as() const noexcept { return pointer<const T>{std::get_if<T>(&value_)}; }
            [[nodiscard]] pointer<const null_t> as_null() const noexcept { return as<null_t>(); }
            [[nodiscard]] pointer<const bool_t> as_bool() const noexcept { return as<bool_t>(); }
            [[nodiscard]] pointer<const integer_t> as_integer() const noexcept { return as<integer_t>(); }
            [[nodiscard]] pointer<const floating_t> as_floating() const noexcept { return as<floating_t>(); }
            [[nodiscard]] pointer<const string_t> as_string() const noexcept { return as<string_t>(); }
            [[nodiscard]] pointer<const array_t> as_array() const noexcept { return as<array_t>(); }
            [[nodiscard]] pointer<const object_t> as_object() const noexcept { return as<object_t>(); }

            template <class T> [[nodiscard]] pointer<T> as() noexcept { return pointer<T>{std::get_if<T>(&value_)}; }
            [[nodiscard]] pointer<null_t> as_null() noexcept { return as<null_t>(); }
            [[nodiscard]] pointer<bool_t> as_bool() noexcept { return as<bool_t>(); }
            [[nodiscard]] pointer<integer_t> as_integer() noexcept { return as<integer_t>(); }
            [[nodiscard]] pointer<floating_t> as_floating() noexcept { return as<floating_t>(); }
            [[nodiscard]] pointer<string_t> as_string() noexcept { return as<string_t>(); }
            [[nodiscard]] pointer<array_t> as_array() noexcept { return as<array_t>(); }
            [[nodiscard]] pointer<object_t> as_object() noexcept { return as<object_t>(); }

            template <class T> [[nodiscard]] const T& get() const { return *as<T>(); }
            [[nodiscard]] null_t get_null() const { return *as<null_t>(); }
            [[nodiscard]] bool_t get_bool() const { return *as<bool_t>(); }
            [[nodiscard]] integer_t get_integer() const { return *as<integer_t>(); }
            [[nodiscard]] floating_t get_floating() const { return *as<floating_t>(); }
            [[nodiscard]] string_t get_string() const { return *as<string_t>(); }
            [[nodiscard]] array_t get_array() const { return *as<array_t>(); }
            [[nodiscard]] object_t get_object() const { return *as<object_t>(); }

            template <class T, class U, std::enable_if_t<std::is_convertible_v<U&&, T>>* = nullptr> [[nodiscard]] T get_or(U&& default_value) const noexcept { return as<T>() ? *as<T>() : static_cast<T>(std::forward<U>(default_value)); }
            [[nodiscard]] null_t get_null_or(null_t default_value) const noexcept { return this->get_or<null_t>(default_value); }
            [[nodiscard]] bool_t get_bool_or(bool_t default_value) const noexcept { return this->get_or<bool_t>(default_value); }
            [[nodiscard]] integer_t get_integer_or(integer_t default_value) const noexcept { return this->get_or<integer_t>(default_value); }
            [[nodiscard]] floating_t get_floating_or(floating_t default_value) const noexcept { return this->get_or<floating_t>(default_value); }
            [[nodiscard]] string_t get_string_or(const string_t& default_value) const noexcept { return this->get_or<string_t>(default_value); }
            [[nodiscard]] string_t get_string_or(string_t&& default_value) const noexcept { return this->get_or<string_t>(std::move(default_value)); }
            [[nodiscard]] array_t get_array_or(const array_t& default_value) const noexcept { return this->get_or<array_t>(default_value); }
            [[nodiscard]] array_t get_array_or(array_t&& default_value) const noexcept { return this->get_or<array_t>(std::move(default_value)); }
            [[nodiscard]] object_t get_object_or(const object_t& default_value) const noexcept { return this->get_or<object_t>(default_value); }
            [[nodiscard]] object_t get_object_or(object_t&& default_value) const noexcept { return this->get_or<object_t>(std::move(default_value)); }

            [[nodiscard]] bool is_number() const noexcept
            {
                return is<integer_t>() || is<floating_t>();
            }

            [[nodiscard]] std::optional<number_t> as_number() const noexcept
            {
                if (const auto num = as<integer_t>()) return std::optional<number_t>(std::in_place, static_cast<floating_t>(*num));
                if (const auto num = as<floating_t>()) return std::optional<number_t>(std::in_place, *num);
                return std::optional<number_t>{};
            }

            [[nodiscard]] number_t get_number() const
            {
                if (as<integer_t>()) return static_cast<number_t>(get_integer());
                if (as<floating_t>()) return static_cast<number_t>(get_floating());
                throw bad_access();
            }

            template <class U, std::enable_if_t<std::is_convertible_v<U&&, number_t>>* = nullptr>
            [[nodiscard]] number_t get_number_or(U&& default_value) const
            {
                if (as<integer_t>()) return static_cast<number_t>(get_integer());
                if (as<floating_t>()) return static_cast<number_t>(get_floating());
                return static_cast<number_t>(std::forward<U>(default_value));
            }
        };

    private:
        json_value_t value_ = json_value_t{json_value_t::value_of_null};

    public:
        // constructors

        constexpr json_t(null_t value = json_value_t::value_of_null) noexcept : value_{value} { return; }
        constexpr json_t(bool_t value) noexcept : value_{value} { return; }
        constexpr json_t(integer_t value) noexcept : value_{value} { return; }
        constexpr json_t(floating_t value) noexcept : value_{value} { return; }
        json_t(string_t value) : value_{(std::move(value))} { return; }
        json_t(array_t value) : value_{(std::move(value))} { return; }
        json_t(object_t value) : value_{(std::move(value))} { return; }

        json_t(const json_t& other) = default;
        json_t(json_t&& other) noexcept = default;
        json_t& operator=(const json_t& other) = default;
        json_t& operator=(json_t&& other) noexcept = default;

        ~json_t() = default;

    public:
        // value

        [[nodiscard]] const json_value_t& value() const noexcept { return value_; } // 
        [[nodiscard]] json_value_t& value() noexcept { return value_; }             //
        const json_value_t& operator *() const noexcept { return value(); }         //
        const json_value_t* operator ->() const noexcept { return &value(); }       //
        json_value_t& operator *() noexcept { return value(); }                     //
        json_value_t* operator ->() noexcept { return &value(); }                   //

    public:
        // children

        struct node_ref;
        using const_node_ref = const json_t&;
        [[nodiscard]] const_node_ref operator [](array_index_t key) const noexcept;
        [[nodiscard]] const_node_ref operator [](object_key_view_t key) const noexcept;

        [[nodiscard]] node_ref operator [](array_index_t key) noexcept;
        [[nodiscard]] node_ref operator [](object_key_view_t key) noexcept;

        bool operator ==(const json_t& rhs) const noexcept { return this->value() == rhs.value(); }
        bool operator !=(const json_t& rhs) const noexcept { return this->value() != rhs.value(); }
        bool operator ==(const node_ref& rhs) const noexcept;
        bool operator !=(const node_ref& rhs) const noexcept;

    public:
        // i/o

        class json_reader;
        class json_writer;
        using json_istream = std::basic_istream<char_t>;
        using json_ostream = std::basic_ostream<char_t>;

        static json_t read_json_string(json_istream& src);

        void write_json_string(json_ostream& dst, bool pretty = false) const;

        template <class json_string_t>
        [[nodiscard]] static json_t parse(json_string_t&& source)
        {
            std::basic_istringstream<json_t::char_t> iss{std::forward<json_string_t>(source)};
            return read_json_string(iss);
        }

        [[nodiscard]] json_string_t to_json_string(bool pretty = false) const
        {
            std::basic_ostringstream<json_t::char_t> oss;
            write_json_string(oss, pretty);
            return oss.str();
        }

    public:
        // extensive constructors

        template <class T, class U = void> struct json_ext;

        // copy construct
        template <class T, std::void_t<decltype(json_ext<std::decay_t<const T&>>::serialize(std::declval<const T&>()))>* = nullptr>
        json_t(const T& value)
            : json_t(json_ext<std::decay_t<const T&>>::serialize(value)) { }

        // move construct
        template <class T, std::enable_if_t<std::is_rvalue_reference_v<T>, std::void_t<decltype(json_ext<std::decay_t<T>>::serialize(std::declval<T>()))>>* = nullptr>
        json_t(T&& value)
            : json_t(json_ext<std::decay_t<T>>::serialize(std::forward<T>(value))) { }

    public:
        // etc
        [[nodiscard]] static json_t make_undefined() noexcept
        {
            json_t j;
            j.value_.value_ = json_value_t::value_of_undefined;
            return j;
        }
    };

    static const json_t undefined = json_t::make_undefined();

    /// json_t::node_ref: represents a reference to node or a parent node and index pair.
    struct json_t::node_ref final
    {
        using null_pointer = std::nullptr_t;
        using normal_pointer = json_t*;
        using virtual_array_pointer = std::pair<array_t*, array_t::size_type>;
        using virtual_object_pointer = std::pair<object_t*, object_t::key_type>;
        using pointer = std::variant<null_pointer, normal_pointer, virtual_array_pointer, virtual_object_pointer>;
        pointer index_{null_pointer{}};

        node_ref(pointer p) : index_(std::move(p)) { return; }
        node_ref(const node_ref& other) = delete;
        node_ref(node_ref&& other) noexcept = delete;
        node_ref& operator=(const node_ref& other) = delete;
        node_ref& operator=(node_ref&& other) noexcept = delete;
        ~node_ref() = default;

        [[nodiscard]]
        const json_value_t& value() const noexcept
        {
            if (auto p = std::get_if<normal_pointer>(&index_))
                return (*p)->value_;

            return undefined.value_;
        }

        [[nodiscard]]
        json_value_t& value() noexcept
        {
            if (auto p = std::get_if<normal_pointer>(&index_))
                return (*p)->value_;

            // assumes the value type can't be changed from json_value_t interface.
            return const_cast<json_value_t&>(undefined.value_);
        }

        const json_value_t& operator *() const noexcept { return value(); }
        const json_value_t* operator ->() const noexcept { return &value(); }
        json_value_t& operator *() noexcept { return value(); }
        json_value_t* operator ->() noexcept { return &value(); }

        node_ref operator [](array_index_t key) const noexcept
        {
            if (const auto p = std::get_if<normal_pointer>(&index_))
            {
                if (const auto a = (**p)->as_array())
                {
                    if (key < a->size())
                        return node_ref{normal_pointer{&a->operator[](key)}}; // normal reference
                    else
                        return node_ref{virtual_array_pointer{a.pointer_, key}}; // write only virtual reference
                }
            }

            return node_ref{null_pointer{}};
        }

        node_ref operator [](object_key_view_t key) const noexcept
        {
            if (const auto p = std::get_if<normal_pointer>(&index_))
            {
                if (const auto o = (**p)->as_object())
                {
                    if (const auto it = o->find(key); it != o->end())
                        return node_ref{normal_pointer{&it->second}}; // normal reference
                    else
                        return node_ref{virtual_object_pointer(o.pointer_, key)}; // write only virtual reference
                }
            }

            return node_ref{null_pointer{}};
        }

        json_t& operator =(json_t val)
        {
            if (const auto p = std::get_if<normal_pointer>(&index_))
            {
                return **p = json_t(std::move(val));
            }

            if (const auto p = std::get_if<virtual_array_pointer>(&index_))
            {
                if (p->second >= p->first->size())
                    p->first->resize(p->second + 1);

                json_t& r = p->first->operator[](p->second) = json_t(std::move(val));
                index_ = normal_pointer{&r};
                return r;
            }

            if (const auto p = std::get_if<virtual_object_pointer>(&index_))
            {
                json_t& r = p->first->operator[](p->second) = json_t(std::move(val));
                index_ = normal_pointer{&r};
                return r;
            }

            throw bad_access();
        }

        bool operator ==(const node_ref& rhs) const noexcept { return this->value() == rhs.value(); }
        bool operator !=(const node_ref& rhs) const noexcept { return this->value() != rhs.value(); }
        bool operator ==(const json_t& rhs) const noexcept { return this->value() == rhs.value(); }
        bool operator !=(const json_t& rhs) const noexcept { return this->value() != rhs.value(); }
    };

    inline json_t::const_node_ref json_t::operator[](array_index_t key) const noexcept
    {
        if (const auto a = value_.as_array())
            if (key < a->size())
                return a->at(key);
        return undefined;
    }

    inline json_t::const_node_ref json_t::operator[](object_key_view_t key) const noexcept
    {
        if (const auto o = value_.as_object())
            if (const auto it = o->find(key); it != o->end())
                return it->second;

        return undefined;
    }

    inline json_t::node_ref json_t::operator[](array_index_t key) noexcept { return node_ref{node_ref::normal_pointer{this}}[key]; }
    inline json_t::node_ref json_t::operator[](object_key_view_t key) noexcept { return node_ref{node_ref::normal_pointer{this}}[key]; }
    inline bool json_t::operator ==(const node_ref& rhs) const noexcept { return this->value() == rhs.value(); }
    inline bool json_t::operator !=(const node_ref& rhs) const noexcept { return this->value() != rhs.value(); }

    /// json_reader
    class json_t::json_reader
    {
    public:
        using json_istream = json_t::json_istream;
        using int_t = json_istream::int_type;
        using char_t = json_istream::char_type;

        enum struct option : unsigned long
        {
            none = 0,
            allow_utf8_bom = 1ul << 0,
            allow_unescaped_forward_slash = 1ul << 1,
            allow_comment = 1ul << 2,
            allow_trailing_comma = 1ul << 3,
            allow_unquoted_object_key = 1ul << 4,
            all = ~0u,

            // default_option
            default_option = allow_utf8_bom | allow_unescaped_forward_slash,
        };

        // From stream
        static json_t read_json(json_istream& source, option opt = option::default_option)
        {
            return json_reader(source, opt).execute();
        }

        // From string
        template <class json_string_t, std::void_t<decltype(std::stringstream(std::declval<json_string_t>()))>* = nullptr>
        static json_t parse_json(json_string_t&& source, option loose = option::default_option)
        {
            std::basic_istringstream<json_t::char_t> iss{std::forward<json_string_t>(source)};
            return json_reader(iss, loose).execute();
        }

    private:
        struct source_iterator // peeking istreambuf_iterator with current position
        {
            json_istream& src;
            size_t pos_char{};
            int pos_line{};
            int pos_column{};

            struct proxy
            {
                int_t value;
                int_t operator *() const { return value; }
            };

            int_t operator *() const
            {
                return src.peek();
            }

            source_iterator& operator ++()
            {
                const int i = src.get(); // advance

                if (i != EOF)
                {
                    pos_char++;
                    if (i != '\n')
                        pos_column++;
                    else
                    {
                        pos_line++;
                        pos_column = 0;
                    }
                }

                return *this;
            }

            proxy operator ++(int)
            {
                const proxy ret{this->operator*()};
                this->operator++();
                return ret;
            }

            [[nodiscard]] bool eat(int_t chr)
            {
                return operator *() == chr
                           ? operator++(), true
                           : false;
            }
        } input;

        option option_bits{};

        [[nodiscard]] bool has_option(option bit) const noexcept
        {
            return static_cast<std::underlying_type_t<option>>(option_bits) & static_cast<std::underlying_type_t<option>>(bit);
        }

        explicit json_reader(json_istream& source, option loose)
            : input{source}
            , option_bits(loose) { }

        [[nodiscard]] json_t execute()
        {
            if (input.eat(0xEF)) // beginning of UTF-8 BOM
            {
                if (has_option(option::allow_utf8_bom))
                {
                    if (!input.eat(0xBB)) throw bad_format("invalid json format: UTF-8 BOM sequence expected... 0xBB", *input);
                    if (!input.eat(0xBF)) throw bad_format("invalid json format: UTF-8 BOM sequence expected... 0xBF", *input);
                }
                else
                {
                    throw bad_format("invalid json format: expected an element. (UTF-8 BOM not allowed)", *input);
                }
            }

            eat_whitespaces();

            return read_element();
        }

        json_t read_element()
        {
            switch (*input)
            {
            case 'n': // null
                if (!input.eat('n')) throw bad_format("invalid 'null' literal: expected 'n'", *input);
                if (!input.eat('u')) throw bad_format("invalid 'null' literal: expected 'u'", *input);
                if (!input.eat('l')) throw bad_format("invalid 'null' literal: expected 'l'", *input);
                if (!input.eat('l')) throw bad_format("invalid 'null' literal: expected 'l'", *input);
                return json_t::null_t{};

            case 't': // true
                if (!input.eat('t')) throw bad_format("invalid 'true' literal: expected 't'", *input);
                if (!input.eat('r')) throw bad_format("invalid 'true' literal: expected 'r'", *input);
                if (!input.eat('u')) throw bad_format("invalid 'true' literal: expected 'u'", *input);
                if (!input.eat('e')) throw bad_format("invalid 'true' literal: expected 'e'", *input);
                return json_t::bool_t{true};

            case 'f': // false
                if (!input.eat('f')) throw bad_format("invalid 'false' literal: expected 'f'", *input);
                if (!input.eat('a')) throw bad_format("invalid 'false' literal: expected 'a'", *input);
                if (!input.eat('l')) throw bad_format("invalid 'false' literal: expected 'l'", *input);
                if (!input.eat('s')) throw bad_format("invalid 'false' literal: expected 's'", *input);
                if (!input.eat('e')) throw bad_format("invalid 'false' literal: expected 'e'", *input);
                return json_t::bool_t{false};

            case '+':
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return read_number();

            case '"':
                return read_string();

            case '[':
                return read_array();

            case '{':
                return read_object();

            default:
                break;
            }

            throw bad_format("invalid json format: expected an element", *input);
        }

        json_t read_number()
        {
            constexpr auto is_digit = [](int_t i)-> bool { return i >= '0' && i <= '9'; }; // locale free is_digit

            char_t buffer[128]{};
            char_t* p = buffer;
            char_t* const integer_limit = buffer + 32;
            char_t* const fraction_limit = buffer + 64;
            char_t* const decimal_limit = buffer + 128;

            long long exp_offset = 0;
            bool integer_type = true;

            {
                // parse integer part
                if (input.eat('-')) { *p++ = '-'; }

                if (input.eat('0')) { *p++ = '0'; } // put '0', leading zeros are not allowed in JSON.
                else if (is_digit(*input))          // not zero
                {
                    while (is_digit(*input))
                        if (p < integer_limit) *p++ = static_cast<char_t>(*input++);                  // put digit
                        else if (exp_offset < std::numeric_limits<int>::max()) ++exp_offset, ++input; // drop digit
                        else throw bad_format("invalid number format: too long integer sequence");
                }
                else throw bad_format("invalid number format: expected a digit", *input);
            }

            if (input.eat('.')) // accept fraction point part
            {
                assert(p < fraction_limit);
                *p++ = '.'; // put decimal point
                integer_type = false;

                if (is_digit(*input))
                {
                    // if integer part is zero (parse '-0.0000...ddd')
                    if ((buffer[0] == '0') || (buffer[0] == '-' && buffer[1] == '0'))
                    {
                        while (*input == '0')
                            if (exp_offset > std::numeric_limits<int>::lowest()) --exp_offset, ++input; // drop '0'
                            else throw bad_format("invalid number format: too long integer sequence");
                    }

                    while (is_digit(*input))
                    {
                        if (p < fraction_limit) *p++ = static_cast<char_t>(*input++); // put digit
                        else ++input;                                                 // drop digit
                    }
                }
                else throw bad_format("invalid number format: expected a digit", *input);
            }

            if (*input == 'e' || *input == 'E') // accept exponential part
            {
                ++input; // 'e'
                integer_type = false;

                char_t exp_part[32]{};
                char_t* exp = exp_part;
                char_t* const exp_limit = exp_part + std::size(exp_part);

                // read

                if (input.eat('-')) { *exp++ = '-'; }
                else if (input.eat('+')) { } // drop '+'

                if (is_digit(*input))
                {
                    while (is_digit(*input))
                    {
                        if (exp < exp_limit) *exp++ = static_cast<char_t>(*input++); // put digit
                        else ++input;                                                // drop digit (it must be overflow, handle later)
                    }
                }
                else throw bad_format("invalid number format: expected a digit", *input);

                // parse

                long long exp_value{};
                auto [ptr, ec] = std::from_chars(exp_part, exp, exp_value, 10);

                if (ec == std::errc{} && ptr == exp)
                {
                    // OK
                    constexpr auto add_sat = [](auto a, auto b)-> decltype(a + b)
                    {
                        if (a > 0 && b > std::numeric_limits<decltype(a + b)>::max() - a) return std::numeric_limits<decltype(a + b)>::max();
                        if (a < 0 && b < std::numeric_limits<decltype(a + b)>::lowest() - a) return std::numeric_limits<decltype(a + b)>::lowest();
                        return a + b;
                    };

                    exp_offset = add_sat(exp_offset, exp_value);
                }
                else if (ec == std::errc::result_out_of_range && ptr == exp)
                {
                    // overflow
                    exp_offset = exp_part[0] == '-'
                                     ? std::numeric_limits<decltype(exp_offset)>::lowest()
                                     : std::numeric_limits<decltype(exp_offset)>::max();
                }
                else
                {
                    assert(false);
                    throw bad_format("invalid number format: unexpected parse error "); // unexpected
                }
            }

            if (exp_offset != 0)
            {
                integer_type = false;

                assert(p < decimal_limit);
                if (p < decimal_limit) *p++ = 'e';

                auto [ptr, ec] = std::to_chars(p, decimal_limit, exp_offset, 10);
                if (ec != std::errc{}) throw bad_format("invalid number format: unexpected error.");
                p = ptr;
            }

            // try to parse as integer type
            if (integer_type)
            {
                json_t::integer_t ret{};
                auto [ptr, ec] = std::from_chars(buffer, p, ret, 10);
                if (ec == std::errc{} && ptr == p) return ret; // integer OK
            }

            // try to parse as floating type
            {
                json_t::floating_t ret{};
                auto [ptr, ec] = std::from_chars(buffer, p, ret);
                if (ptr == p)
                {
                    if (ec == std::errc{}) return ret; // floating OK
                    if (ec == std::errc::result_out_of_range)
                    {
                        if (exp_offset < 0) return buffer[0] == '-' ? json_t::floating_t{-0.0} : json_t::floating_t{+0.0};                                          // underflow
                        else return buffer[0] == '-' ? -std::numeric_limits<json_t::floating_t>::infinity() : +std::numeric_limits<json_t::floating_t>::infinity(); // overflow
                    }
                }
            }

            throw bad_format("invalid number format: failed to parse");
        }

        json_t::string_t read_string()
        {
            // check quote character
            assert(*input == '"');
            int_t quote = *input++; // '"'

            json_t::string_t ret;
            while (true)
            {
                if (input.eat('\\')) // escape sequence
                {
                    if (*input == 'n') ret += '\n';
                    else if (*input == 't') ret += '\t';
                    else if (*input == 'b') ret += '\b';
                    else if (*input == 'f') ret += '\f';
                    else if (*input == 'r') ret += '\r';
                    else if (*input == '\\') ret += '\\';
                    else if (*input == '/') ret += '/';
                    else if (*input == '\"') ret += '\"';
                    else if (*input == '\'') ret += '\'';
                    else if (*input == 'u')
                    {
                        auto hex = [&](int_t chr)
                        {
                            if (chr >= '0' && chr <= '9') { return chr - '0'; }
                            if (chr >= 'A' && chr <= 'F') { return chr - 'A' + 10; }
                            if (chr >= 'a' && chr <= 'f') { return chr - 'a' + 10; }
                            throw bad_format("invalid string format: expected hexadecimal digit for \\u????", chr);
                        };

                        int code = 0;
                        code = code << 4 | hex(*++input);
                        code = code << 4 | hex(*++input);
                        code = code << 4 | hex(*++input);
                        code = code << 4 | hex(*++input);

                        if (code < 0x80) // 7 bit
                        {
                            ret += static_cast<char>((code >> 0 & 0x7F) | 0x00);
                        }
                        else if (code < 0x0800) // 11 bit
                        {
                            ret += static_cast<char>((code >> 6 & 0x1f) | 0xC0);
                            ret += static_cast<char>((code >> 0 & 0x3f) | 0x80);
                        }
                        else if ((code & 0xF800) == 0xD800) // surrogate pair
                        {
                            // assume next surrogate is following.
                            if (*++input != '\\') throw bad_format("invalid string format: expected surrogate pair", *input);
                            if (*++input != 'u') throw bad_format("invalid string format: expected surrogate pair", *input);

                            int code2 = 0;
                            code2 = code2 << 4 | hex(*++input);
                            code2 = code2 << 4 | hex(*++input);
                            code2 = code2 << 4 | hex(*++input);
                            code2 = code2 << 4 | hex(*++input);

                            if ((code & 0xFC00) == 0xDC00 && (code2 & 0xFC00) == 0xD800)
                                std::swap(code, code2);

                            if ((code & 0xFC00) == 0xD800 && (code2 & 0xFC00) == 0xDC00)
                                code = ((code & 0x3FF) << 10 | (code2 & 0x3FF)) + 0x10000; // 21 bit
                            else
                                throw bad_format("invalid string format: invalid surrogate pair sequence");

                            ret += static_cast<char>((code >> 18 & 0x07) | 0xF0);
                            ret += static_cast<char>((code >> 12 & 0x3f) | 0x80);
                            ret += static_cast<char>((code >> 6 & 0x3f) | 0x80);
                            ret += static_cast<char>((code >> 0 & 0x3f) | 0x80);
                        }
                        else // 16 bit
                        {
                            ret += static_cast<char>((code >> 12 & 0x0f) | 0xE0);
                            ret += static_cast<char>((code >> 6 & 0x3f) | 0x80);
                            ret += static_cast<char>((code >> 0 & 0x3f) | 0x80);
                        }
                    }
                    else
                    {
                        throw bad_format("invalid string format: invalid escape sequence");
                    }
                }
                else if (input.eat(quote)) break; // end of string.
                else if (*input == EOF) throw bad_format("invalid string format: unexpected eof");
                else if (*input < 0x20 || *input == 0x7F) throw bad_format("invalid string format: control character is not allowed", *input);
                else if (*input == '/' && !has_option(option::allow_unescaped_forward_slash)) throw bad_format("invalid string format: unescaped '/' is not allowed");
                else ret += static_cast<char_t>(*input); // OK. normal character.
                ++input;
            }

            return ret;
        }

        json_t::array_t read_array()
        {
            if (!input.eat('[')) throw bad_format("logic error");
            eat_whitespaces();

            if (input.eat(']')) return json_t::array_t{}; // empty array

            json_t::array_t ret{};
            while (true)
            {
                // read value
                ret.push_back(read_element());

                eat_whitespaces();

                if (input.eat(','))
                {
                    eat_whitespaces();
                    if (has_option(option::allow_trailing_comma) && input.eat(']')) break;
                    else if (*input == ']') throw bad_format("invalid array format: expected an element (trailing comma not allowed)", *input);
                }
                else if (input.eat(']')) break;
                else throw bad_format("invalid array format: ',' or ']' expected", *input);
            }
            return ret;
        }

        json_t::object_t read_object()
        {
            if (!input.eat('{')) throw bad_format("logic error");

            eat_whitespaces();

            if (input.eat('}')) return json_t::object_t{}; // empty object_t

            json_t::object_t ret;
            while (true)
            {
                // read key
                json_t::string_t key{};

                if (*input == '"') key = read_string();
                else if (has_option(option::allow_unquoted_object_key))
                {
                    while (*input != EOF && *input > ' ' && *input != ':')
                        key += static_cast<char_t>(*input++);
                }
                else throw bad_format("invalid object format: expected object key", *input);

                eat_whitespaces();

                if (!input.eat(':'))
                    throw bad_format("invalid object format: expected a ':'", *input);

                eat_whitespaces();

                // read value
                ret[key] = read_element();

                eat_whitespaces();

                // read ',' or '}'
                if (input.eat(','))
                {
                    eat_whitespaces();
                    if (has_option(option::allow_trailing_comma) && input.eat('}')) break;
                    else if (*input == '}') throw bad_format("invalid object format: expected an element (trailing comma not allowed)", *input);
                }
                else if (input.eat('}')) break;
                else throw bad_format("invalid object format: expected ',' or '}'", *input);
            }

            return ret;
        }

        void eat_whitespaces()
        {
            constexpr auto is_space = [](int_t i)-> bool
            {
                return i == ' ' || i == '\t' || i == '\r' || i == '\n';
            };

            while (true)
            {
                while (is_space(*input)) ++input;

                if (has_option(option::allow_comment) && input.eat('/'))
                {
                    if (input.eat('*')) // start of block comment
                    {
                        while (*input != EOF)
                        {
                            if (*input++ == '*' && input.eat('/'))
                                break;
                        }
                    }
                    else if (input.eat('/')) // start of line comment
                    {
                        while (*input != EOF)
                        {
                            if (*input++ == '\n')
                                break;
                        }
                    }

                    continue;
                }

                break;
            }
        }

        [[nodiscard]] nanojson2::bad_format bad_format(std::string_view reason, std::optional<int_t> but_encountered = std::nullopt) const
        {
            std::stringstream message;
            message << "bad_format: ";
            message << reason;
            if (but_encountered)
            {
                message << " but encountered ";
                if (*but_encountered == EOF)
                {
                    message << "EOF";
                }
                else if (*but_encountered >= 0x20 && but_encountered < 0x7F)
                {
                    message << "'";
                    message << static_cast<char_t>(*but_encountered);
                    message << "'";
                }
                else
                {
                    message << "(char)";
                    message << std::hex << std::setfill('0') << std::setw(2) << *but_encountered;
                }
            }
            message << " at line ";
            message << (input.pos_line + 1);
            message << " column ";
            message << (input.pos_column + 1);
            message << ".";

            return nanojson2::bad_format{message.str()};
        }
    };

    static inline constexpr json_t::json_reader::option operator |(json_t::json_reader::option lhs, json_t::json_reader::option rhs) { return static_cast<json_t::json_reader::option>(static_cast<std::underlying_type_t<json_t::json_reader::option>>(lhs) | static_cast<std::underlying_type_t<json_t::json_reader::option>>(rhs)); }
    static inline constexpr json_t::json_reader::option operator &(json_t::json_reader::option lhs, json_t::json_reader::option rhs) { return static_cast<json_t::json_reader::option>(static_cast<std::underlying_type_t<json_t::json_reader::option>>(lhs) & static_cast<std::underlying_type_t<json_t::json_reader::option>>(rhs)); }
    static inline constexpr json_t::json_reader::option operator ^(json_t::json_reader::option lhs, json_t::json_reader::option rhs) { return static_cast<json_t::json_reader::option>(static_cast<std::underlying_type_t<json_t::json_reader::option>>(lhs) ^ static_cast<std::underlying_type_t<json_t::json_reader::option>>(rhs)); }
    static inline constexpr json_t::json_reader::option& operator |=(json_t::json_reader::option& lhs, json_t::json_reader::option rhs) { return lhs = lhs | rhs; }
    static inline constexpr json_t::json_reader::option& operator &=(json_t::json_reader::option& lhs, json_t::json_reader::option rhs) { return lhs = lhs & rhs; }
    static inline constexpr json_t::json_reader::option& operator ^=(json_t::json_reader::option& lhs, json_t::json_reader::option rhs) { return lhs = lhs ^ rhs; }

    /// json_writer
    class json_t::json_writer
    {
    public:
        using json_ostream = json_t::json_ostream;

        // To stream
        static void write_json(json_ostream& destination, const json_t& val, bool pretty = false, bool debug_dump = false)
        {
            std::basic_string<json_t::char_t> indent_stack;
            write_element(destination, val, pretty || debug_dump, indent_stack, debug_dump);
        }

        // To string
        static json_t::json_string_t to_json_string(const json_t& val, bool pretty = false, bool debug_dump = false)
        {
            std::basic_ostringstream<json_t::char_t> oss;
            write_json(oss, val, pretty, debug_dump);
            return oss.str();
        }

    private:
        static void write_element(json_ostream& out, const json_t& value, bool pretty, std::basic_string<json_t::char_t>& indent_stack, bool debug_dump)
        {
            if (value->is_undefined())
            {
                if (debug_dump) out << "/***  UNDEFINED  ***/ undefined /* not allowed */";
                else throw bad_value("undefined is not allowed");
            }

            if (value->as_null())
            {
                if (debug_dump) out << "/***  NULL  ***/ ";
                out << "null";
            }

            if (const auto val = value->as_bool())
            {
                if (debug_dump) out << "/***  BOOLEAN  ***/ ";
                out << (*val ? "true" : "false");
            }

            if (const auto val = value->as_integer())
            {
                if (debug_dump) out << "/***  INTEGER  ***/ ";
                json_t::integer_t v = *val;
                json_t::char_t v_str[128]{};
                auto [ptr, ec] = std::to_chars(std::begin(v_str), std::end(v_str), v, 10);
                if (ec != std::errc{} || *ptr != '\0') throw bad_value("failed to to_chars(integer)");
                out << v_str;
            }

            if (const auto val = value->as_floating())
            {
                if (debug_dump) out << "/***  FLOATING  ***/ ";
                json_t::floating_t v = *val;
                if (std::isnan(v))
                {
                    if (debug_dump) out << "NaN /* not allowed */";
                    else throw bad_value("NaN is not allowed");
                }

                if (std::isinf(v))
                {
                    out << (v > 0 ? "1.0e999999999" : "-1.0e999999999");
                    return;
                }


                std::chars_format format = std::chars_format::general;
                const auto precision = std::clamp<int>(static_cast<int>(out.precision()), 0, 64);
                const auto overflow_limit = std::pow(static_cast<json_t::floating_t>(10), precision);
                const auto underflow_limit = std::pow(static_cast<json_t::floating_t>(10), precision);

                if (const auto abs = std::abs(v); abs < overflow_limit && abs > underflow_limit)
                {
                    const auto mode = out.flags() & std::ios_base::floatfield;
                    if (mode == std::ios_base::fixed) format = std::chars_format::fixed;
                    if (mode == std::ios_base::scientific) format = std::chars_format::scientific;
                }

                json_t::char_t v_str[128]{};
                auto [ptr, ec] = std::to_chars(std::begin(v_str), std::end(v_str), v, format, precision);
                if (ec != std::errc{} || *ptr != '\0') throw bad_value("failed to to_chars(floating)");

                out << v_str;
            }

            constexpr auto export_string = [](std::ostream& dst, const json_t::string_t& val)
            {
                dst << '"';
                for (const char c : val)
                {
                    switch (c)
                    {
                    case '\n':
                        dst << "\\n";
                        break;
                    case '\t':
                        dst << "\\t";
                        break;
                    case '\b':
                        dst << "\\b";
                        break;
                    case '\f':
                        dst << "\\f";
                        break;
                    case '\r':
                        dst << "\\r";
                        break;
                    case '\\':
                        dst << "\\\\";
                        break;
                    case '/':
                        dst << "\\/";
                        break;
                    case '"':
                        dst << "\\\"";
                        break;
                    default:
                        if (static_cast<unsigned char>(c) < ' ' || (static_cast<unsigned char>(c) == 0xFF))
                        {
                            std::ostringstream oss;
                            oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(static_cast<unsigned char>(c));
                            dst << oss.str();
                        }
                        else
                        {
                            dst << c;
                        }
                        break;
                    }
                }
                dst << '"';
            };

            if (const auto val = value->as_string())
            {
                if (debug_dump) out << "/***  STRING  ***/ ";
                export_string(out, *val);
            }

            if (const auto val = value->as_array())
            {
                if (val->empty())
                {
                    if (debug_dump) out << "/***  ARRAY[0]  ***/ ";
                    out << "[]";
                }
                else
                {
                    if (debug_dump) out << "/***  ARRAY[" << val->size() << "]  ***/ ";
                    out << "[";
                    if (pretty) out << "\n";
                    indent_stack.push_back(' ');
                    indent_stack.push_back(' ');
                    for (auto it = val->begin(); it != val->end(); ++it)
                    {
                        if (it != val->begin()) out << ',' << (pretty ? "\n" : "");
                        if (pretty) out << indent_stack;
                        write_element(out, *it, pretty, indent_stack, debug_dump);
                    }
                    indent_stack.pop_back();
                    indent_stack.pop_back();
                    if (pretty) out << "\n" << indent_stack;
                    out << "]";
                }
            }

            if (const auto val = value->as_object())
            {
                if (val->empty())
                {
                    if (debug_dump) out << "/***  OBJECT[0]  ***/ ";
                    out << "{}";
                }
                else
                {
                    if (debug_dump) out << "/***  OBJECT[" << val->size() << "]  ***/  ";
                    out << "{";
                    if (pretty) out << "\n";

                    indent_stack.push_back(' ');
                    indent_stack.push_back(' ');
                    for (auto it = val->begin(); it != val->end(); ++it)
                    {
                        if (it != val->begin()) out << ',' << (pretty ? "\n" : "");
                        if (pretty) out << indent_stack;
                        export_string(out, it->first);
                        out << ":";
                        if (pretty) out << " ";
                        write_element(out, it->second, pretty, indent_stack, debug_dump);
                    }
                    indent_stack.pop_back();
                    indent_stack.pop_back();
                    if (pretty) out << "\n" << indent_stack;
                    out << "}";
                }
            }
        }
    };

    inline json_t json_t::read_json_string(json_istream& src)
    {
        return json_reader::read_json(src);
    }

    inline void json_t::write_json_string(json_ostream& dst, bool pretty) const
    {
        json_writer::write_json(dst, *this, pretty);
    }


    ////
    //// json_t::json_ext specializations
    ////

    namespace json_ext_ctor_helper
    {
        struct prevent_conversion { };

        template <class T>
        struct static_cast_to_value
        {
            template <class U>
            static T serialize(U val)
            {
                return static_cast<T>(val);
            }
        };

        template <class T>
        struct pass_to_value_ctor
        {
            template <class U>
            static T serialize(U&& val)
            {
                return T{std::forward<U>(val)};
            }
        };

        template <class T>
        struct pass_iterator_pair_to_value_ctor
        {
            template <class U>
            static T serialize(U&& val)
            {
                return T(std::begin(val), std::end(val));
            }
        };
    }

    // built-in ext ctor for primitives.
    template <class T> struct json_t::json_ext<T, std::enable_if_t<std::is_pointer_v<T>>> : json_ext_ctor_helper::prevent_conversion { };                      // prevent `bool` constructor by pointer types
    template <class T> struct json_t::json_ext<T, std::enable_if_t<std::is_integral_v<T>>> : json_ext_ctor_helper::static_cast_to_value<integer_t> { };        // map all integral types (`char`, `int`, `unsigned long`, ...) to `int_t`: hint: this is also true for `bool`, but concrete constructor `json_t(bool)` is selected in the real calling situation
    template <class T> struct json_t::json_ext<T, std::enable_if_t<std::is_floating_point_v<T>>> : json_ext_ctor_helper::static_cast_to_value<floating_t> { }; // map all floating point types (`float`, `double`, ...) to `float_t` (`std::is_floating_point<T>`)
    template <> struct json_t::json_ext<const json_t::char_t*> : json_ext_ctor_helper::pass_to_value_ctor<string_t> { };                                       // map `const char*` to `string_t`
    template <> struct json_t::json_ext<json_t::string_view_t> : json_ext_ctor_helper::pass_to_value_ctor<string_t> { };                                       // map `std::string_view` to `string_t`

    // built-in ext ctor for array_t: map some STL container`<T>` to `array_t`, if `T` is convertible json_t.
    template <class U> struct json_t::json_ext<std::initializer_list<U>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { };                           // map `std::initializer_list<T>` to `array_t`
    template <class U, size_t S> struct json_t::json_ext<std::array<U, S>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { };                         // map `std::vector<T>` to `array_t`
    template <class U, class A> struct json_t::json_ext<std::vector<U, A>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { };                         // map `std::array<T, n>` to `array_t`
    template <class U, class A> struct json_t::json_ext<std::set<U, A>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { };                            // map `std::set<T>` to `array_t`
    template <class U, class A> struct json_t::json_ext<std::multiset<U, A>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { };                       // map `std::multiset<T>` to `array_t`
    template <class U, class P, class A> struct json_t::json_ext<std::unordered_set<U, P, A>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { };      // map `std::unordered_set<T>` to `array_t`
    template <class U, class P, class A> struct json_t::json_ext<std::unordered_multiset<U, P, A>, std::enable_if_t<std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<array_t> { }; // map `std::unordered_multiset<T>` to `array_t`

    template <class...U> struct json_t::json_ext<std::tuple<U...>, std::enable_if_t<std::conjunction_v<std::is_constructible<json_t, U>...>>> // map `std::tuple<T...>` to `array_t`
    {
        static array_t serialize(const std::tuple<U...>& val) { return std::apply([](auto&& ...x) { return array_t{{json_t(std::forward<decltype(x)>(x))...}}; }, std::forward<decltype(val)>(val)); }
        static array_t serialize(std::tuple<U...>&& val) { return std::apply([](auto&& ...x) { return array_t{{json_t(std::forward<decltype(x)>(x))...}}; }, std::forward<decltype(val)>(val)); }
    };

    // built-in ext ctor for object_t: map some STL container`<K, T>` to `object_t`, if `K` is convertible string and `T` is convertible json_t.
    template <class K, class U, class P, class A> struct json_t::json_ext<std::map<K, U, P, A>, std::enable_if_t<std::is_convertible_v<K, json_t::object_t::key_type> && std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<object_t> { };           // map `std::map<K, T>` to `object_t`
    template <class K, class U, class P, class A> struct json_t::json_ext<std::unordered_map<K, U, P, A>, std::enable_if_t<std::is_convertible_v<K, json_t::object_t::key_type> && std::is_constructible_v<json_t, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<object_t> { }; // map `std::unordered_map<K, T>` to `object_t`

    // built-in ext ctor for to_json types: some types which has `to_json()` function to `json_t`

    // member function `string to_json() const;`
    template <class T>
    struct json_t::json_ext<T, std::enable_if_t<std::is_invocable_r_v<json_t::json_string_t, decltype(&T::to_json), const T&>>>
    {
        static json_t serialize(const T& val) { return json_t::parse(val.to_json()); }
    };

    // member function `json_t to_json() const;`
    template <class T>
    struct json_t::json_ext<T, std::enable_if_t<std::is_same_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json_t>>>
    {
        static json_t serialize(const T& val) { return val.to_json(); }
    };

    // function `string to_json(T);` by global/ADL
    template <class T>
    struct json_t::json_ext<T, std::enable_if_t<!std::is_invocable_r_v<json_t::json_string_t, decltype(&T::to_json), const T&> && std::is_invocable_r_v<json_t::json_string_t, decltype(to_json(std::declval<T>())), const T&>>>
    {
        static json_t serialize(const T& val) { return json_t::parse(to_json(val)); }
    };

    // function `json_t to_json(T);` by global/ADL
    template <class T>
    struct json_t::json_ext<T, std::enable_if_t<!std::is_same_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json_t> && std::is_same_v<std::invoke_result_t<decltype(to_json(std::declval<T>())), const T&>, json_t>>>
    {
        static json_t serialize(const T& val) { return to_json(val); }
    };


    ////
    //// iostream manipulators/operators
    ////

    namespace ios_helper
    {
        inline int json_ios_flags_index()
        {
            static int index = std::ios_base::xalloc();
            return index;
        }

        enum flags : long
        {
            json_ios_flag_pretty = 1 << 1,
            json_ios_flag_loose = 1 << 2,
        };

        inline json_t::json_istream& json_ios_loose(json_t::json_istream& is)
        {
            is.iword(json_ios_flags_index()) |= (flags::json_ios_flag_loose);
            return is;
        }

        inline json_t::json_ostream& json_ios_pretty(json_t::json_ostream& os)
        {
            os.iword(json_ios_flags_index()) |= (flags::json_ios_flag_pretty);
            return os;
        }

        inline json_t::json_istream& operator >>(json_t::json_istream& is, json_t& j)
        {
            const long flags = std::exchange(is.iword(json_ios_flags_index()), 0);
            j = json_t::json_reader::read_json(
                is,
                flags & flags::json_ios_flag_loose
                    ? json_t::json_reader::option::all
                    : json_t::json_reader::option::none);
            return is;
        }

        inline json_t::json_ostream& operator <<(json_t::json_ostream& os, const json_t& j)
        {
            const long flags = std::exchange(os.iword(json_ios_flags_index()), 0);
            json_t::json_writer::write_json(os, j, flags & flags::json_ios_flag_pretty);
            return os;
        }
    }

    using ios_helper::json_ios_loose;
    using ios_helper::json_ios_pretty;
    using ios_helper::operator <<;
    using ios_helper::operator >>;

    // type exports

    using null_t = json_t::null_t;
    using integer_t = json_t::integer_t;
    using floating_t = json_t::floating_t;
    using string_t = json_t::string_t;
    using bool_t = json_t::bool_t;
    using array_t = json_t::array_t;
    using object_t = json_t::object_t;

    using json_string_t = json_t::json_string_t;
    using json_reader = json_t::json_reader;
    using json_writer = json_t::json_writer;
}
#endif
