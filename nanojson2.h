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
#pragma message("nanojson needs C++17 Elementary string conversions (P0067R5) including Floating-Point (FP) values support. See (https://en.cppreference.com/w/cpp/compiler_support/17#:~:text=Elementary%20string%20conversions) This time, falling back to an implementation with stringstream instead.")
#endif

namespace nanojson2
{
    inline namespace exceptions
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
    }

    enum struct json_type_index
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

    /// json_t: represents a json element
    class json_t final
    {
    public:
        using type_index = json_type_index;

        using char_t = char;

        using undefined_t = std::monostate;
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

        using json_string_t = std::basic_string<char_t>;
        using json_string_view_t = std::basic_string_view<char_t>;

        /// json_value_t: holds a json element value
        class json_value_t final
        {
        private:
            friend class json_t;

            static inline constexpr undefined_t value_of_undefined = undefined_t{};
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
        constexpr json_t(undefined_t) noexcept : value_{} { return; }
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
        [[nodiscard]] static const json_t& make_undefined_reference() noexcept
        {
            static json_t undefined{undefined_t{}};
            return undefined;
        }

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
        const json_t& operator[](array_index_t key) const noexcept
        {
            if (const auto a = value().as_array())
                if (key < a->size())
                    return a->operator[](key);

            return make_undefined_reference();
        }

        const json_t& operator[](object_key_view_t key) const noexcept
        {
            if (const auto o = value().as_object())
                if (const auto it = o->find(key); it != o->end())
                    return it->second;

            return make_undefined_reference();
        }

        bool operator ==(const json_t& rhs) const noexcept { return this->value() == rhs.value(); }
        bool operator !=(const json_t& rhs) const noexcept { return this->value() != rhs.value(); }

        struct json_node_ref_t;
        using node_ref = json_node_ref_t;
        using const_node_ref = const json_t&;

        struct json_node_ref_t final
        {
            using null_pointer = std::monostate;
            using normal_pointer = json_t*;
            using const_pointer = const json_t*;
            using array_write_pointer = std::pair<array_t*, array_t::size_type>;
            using object_write_pointer = std::pair<object_t*, object_t::key_type>;
            using pointer = std::variant<null_pointer, normal_pointer, array_write_pointer, object_write_pointer>;

        private:
            pointer pointer_{};

        public:
            json_node_ref_t(pointer p) : pointer_(std::move(p)) { }

            json_node_ref_t(const json_node_ref_t& other) = delete;
            json_node_ref_t(json_node_ref_t&& other) noexcept = delete;
            json_node_ref_t& operator=(const json_node_ref_t& other) = delete;
            json_node_ref_t& operator=(json_node_ref_t&& other) noexcept = delete;
            ~json_node_ref_t() = default;

            [[nodiscard]] const json_value_t& value() const noexcept
            {
                if (auto p = std::get_if<normal_pointer>(&pointer_))
                    return (*p)->value();

                return make_undefined_reference().value();
            }

            [[nodiscard]] json_value_t& value() noexcept
            {
                if (auto p = std::get_if<normal_pointer>(&pointer_))
                    return (*p)->value();

                // assumes the value type can't be changed from json_value_t interface.
                return const_cast<json_value_t&>(make_undefined_reference().value());
            }

            const json_value_t& operator *() const noexcept { return value(); }
            const json_value_t* operator ->() const noexcept { return &value(); }
            json_value_t& operator *() noexcept { return value(); }
            json_value_t* operator ->() noexcept { return &value(); }

            const_node_ref operator [](array_index_t key) const noexcept
            {
                if (const const_pointer* p = std::get_if<normal_pointer>(&pointer_))
                    return (*p)->operator[](key);

                return make_undefined_reference();
            }

            const_node_ref operator [](object_key_view_t key) const noexcept
            {
                if (const const_pointer* p = std::get_if<normal_pointer>(&pointer_))
                    return (*p)->operator[](key);

                return make_undefined_reference();
            }

            node_ref operator [](array_index_t key) noexcept
            {
                if (const normal_pointer* p = std::get_if<normal_pointer>(&pointer_))
                {
                    if (const auto a = (*p)->value().as_array())
                    {
                        if (key < a->size())
                            return json_node_ref_t{normal_pointer{&a->operator[](key)}}; // normal reference
                        else
                            return json_node_ref_t{array_write_pointer{a.pointer_, key}}; // write only virtual reference
                    }
                }

                return json_node_ref_t{null_pointer{}};
            }

            node_ref operator [](object_key_view_t key) noexcept
            {
                if (const normal_pointer* p = std::get_if<normal_pointer>(&pointer_))
                {
                    if (const auto o = (*p)->value().as_object())
                    {
                        if (const auto it = o->find(key); it != o->end())
                            return json_node_ref_t{normal_pointer{&it->second}}; // normal reference
                        else
                            return json_node_ref_t{object_write_pointer(o.pointer_, object_t::key_type(key))}; // write only virtual reference
                    }
                }

                return json_node_ref_t{null_pointer{}};
            }

            json_t& operator =(json_t value)
            {
                if (const normal_pointer* p = std::get_if<normal_pointer>(&pointer_))
                {
                    return **p = json_t(std::move(value));
                }

                if (const array_write_pointer* p = std::get_if<array_write_pointer>(&pointer_))
                {
                    auto& [array, index] = *p;

                    if (index >= array->size())
                        array->resize(index + 1);

                    json_t& r = (*array)[index] = json_t(std::move(value));
                    pointer_ = normal_pointer{&r}; // update pointer to real instance
                    return r;
                }

                if (const object_write_pointer* p = std::get_if<object_write_pointer>(&pointer_))
                {
                    auto& [object, key] = *p;

                    json_t& r = (*object)[key] = json_t(std::move(value));
                    pointer_ = normal_pointer{&r}; // update pointer to real instance
                    return r;
                }

                throw bad_access();
            }

            bool operator ==(const node_ref& rhs) const noexcept { return this->value() == rhs.value(); }
            bool operator !=(const node_ref& rhs) const noexcept { return this->value() != rhs.value(); }
            bool operator ==(const_node_ref rhs) const noexcept { return this->value() == rhs.value(); }
            bool operator !=(const_node_ref rhs) const noexcept { return this->value() != rhs.value(); }
        };

        node_ref operator[](array_index_t index) noexcept
        {
            auto ref = json_node_ref_t{json_node_ref_t::normal_pointer{this}};
            return ref[index];
        }

        node_ref operator[](object_key_view_t key) noexcept
        {
            auto ref = json_node_ref_t{json_node_ref_t::normal_pointer{this}};
            return ref[key];
        }

        bool operator ==(const json_node_ref_t& rhs) const noexcept { return this->value() == rhs.value(); }
        bool operator !=(const json_node_ref_t& rhs) const noexcept { return this->value() != rhs.value(); }

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
    };

    namespace json_parser
    {
        enum struct option : unsigned long
        {
            none = 0,
            allow_utf8_bom = 1ul << 0,
            allow_unescaped_forward_slash = 1ul << 1,
            allow_comment = 1ul << 2,
            allow_trailing_comma = 1ul << 3,
            allow_unquoted_object_key = 1ul << 4,
            allow_number_with_plus_sign = 1ul << 5,
            all = ~0u,

            // default_option
            default_option = allow_utf8_bom | allow_unescaped_forward_slash,
        };

        static inline constexpr option operator |(option lhs, option rhs) { return static_cast<option>(static_cast<std::underlying_type_t<option>>(lhs) | static_cast<std::underlying_type_t<option>>(rhs)); }
        static inline constexpr option operator &(option lhs, option rhs) { return static_cast<option>(static_cast<std::underlying_type_t<option>>(lhs) & static_cast<std::underlying_type_t<option>>(rhs)); }
        static inline constexpr option operator ^(option lhs, option rhs) { return static_cast<option>(static_cast<std::underlying_type_t<option>>(lhs) ^ static_cast<std::underlying_type_t<option>>(rhs)); }
        static inline constexpr option& operator |=(option& lhs, option rhs) { return lhs = lhs | rhs; }
        static inline constexpr option& operator &=(option& lhs, option rhs) { return lhs = lhs & rhs; }
        static inline constexpr option& operator ^=(option& lhs, option rhs) { return lhs = lhs ^ rhs; }

        template <class character_input_iterator>
        class reader
        {
        public:
            static json_t read_json(
                character_input_iterator begin,
                character_input_iterator end,
                option loose = option::default_option)
            {
                return reader(begin, end, loose).execute();
            }

        private:
            using char_traits = std::char_traits<json_t::char_t>;
            using char_type = typename char_traits::char_type;
            using int_type = typename char_traits::int_type;

            struct source_reader
            {
                character_input_iterator it_;
                character_input_iterator const end_;

                size_t current_position_char_{};
                int current_position_line_{};
                int current_position_column_{};

                [[nodiscard]] int_type peek() const
                {
                    return it_ != end_
                               ? char_traits::to_int_type(*it_)
                               : char_traits::eof();
                }

                [[nodiscard]] int_type eat()
                {
                    const int_type i = peek();
                    if (it_ != end_) ++it_;

                    if (i != EOF)
                    {
                        current_position_char_++;

                        current_position_column_++;
                        if (i == '\n')
                        {
                            current_position_line_++;
                            current_position_column_ = 0;
                        }
                    }

                    return i;
                }

                [[nodiscard]] bool eat(int_type chr)
                {
                    return peek() == chr
                               ? (void)eat(), true
                               : false;
                }

                int_type operator *() const
                {
                    return peek();
                }

                source_reader& operator ++()
                {
                    return (void)eat(), *this;
                }

                auto operator ++(int)
                {
                    struct iter
                    {
                        int_type value;
                        int_type operator *() const { return value; }
                    };

                    return iter{eat()};
                }
            } input_;

            option option_bits_{};

            reader(character_input_iterator begin, character_input_iterator end, option option)
                : input_{begin, end}
                , option_bits_(option) { }

            [[nodiscard]] json_t execute()
            {
                eat_utf8bom();
                eat_whitespaces();
                return read_element();
            }

            [[nodiscard]] bool has_option(option bit) const noexcept
            {
                return (option_bits_ & bit) != option::none;
            }

            json_t read_element()
            {
                switch (*input_)
                {
                case 'n': // null
                    if (!input_.eat('n')) throw bad_format("invalid 'null' literal: expected 'n'", *input_);
                    if (!input_.eat('u')) throw bad_format("invalid 'null' literal: expected 'u'", *input_);
                    if (!input_.eat('l')) throw bad_format("invalid 'null' literal: expected 'l'", *input_);
                    if (!input_.eat('l')) throw bad_format("invalid 'null' literal: expected 'l'", *input_);
                    return json_t::null_t{};

                case 't': // true
                    if (!input_.eat('t')) throw bad_format("invalid 'true' literal: expected 't'", *input_);
                    if (!input_.eat('r')) throw bad_format("invalid 'true' literal: expected 'r'", *input_);
                    if (!input_.eat('u')) throw bad_format("invalid 'true' literal: expected 'u'", *input_);
                    if (!input_.eat('e')) throw bad_format("invalid 'true' literal: expected 'e'", *input_);
                    return json_t::bool_t{true};

                case 'f': // false
                    if (!input_.eat('f')) throw bad_format("invalid 'false' literal: expected 'f'", *input_);
                    if (!input_.eat('a')) throw bad_format("invalid 'false' literal: expected 'a'", *input_);
                    if (!input_.eat('l')) throw bad_format("invalid 'false' literal: expected 'l'", *input_);
                    if (!input_.eat('s')) throw bad_format("invalid 'false' literal: expected 's'", *input_);
                    if (!input_.eat('e')) throw bad_format("invalid 'false' literal: expected 'e'", *input_);
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

                throw bad_format("invalid json format: expected an element", *input_);
            }

            json_t read_number()
            {
                constexpr auto is_digit = [](int_type i)-> bool { return i >= '0' && i <= '9'; }; // locale-independent is_digit

                char_type buffer[128]{};
                char_type* p = buffer;
                char_type* const integer_limit = buffer + 48;
                char_type* const fraction_limit = buffer + 64;
                char_type* const decimal_limit = buffer + 128;
                long long exp_offset = 0;
                bool integer_type = true;

                // parse integer part
                {
                    if (input_.eat('-')) { *p++ = '-'; }                                       // minus sign
                    if (has_option(option::allow_number_with_plus_sign) && input_.eat('+')) {} // ignore plus sign

                    if (input_.eat('0')) { *p++ = '0'; } // put '0', leading zeros are not allowed in JSON.
                    else if (is_digit(*input_))          // not zero
                    {
                        while (is_digit(*input_))
                            if (p < integer_limit) *p++ = static_cast<char_type>(*input_++);               // put digit
                            else if (exp_offset < std::numeric_limits<int>::max()) ++exp_offset, ++input_; // drop digit
                            else throw bad_format("invalid number format: too long integer sequence");
                    }
                    else throw bad_format("invalid number format: expected a digit", *input_);
                }

                if (input_.eat('.')) // accept fraction point part
                {
                    assert(p < fraction_limit);
                    *p++ = '.'; // put decimal point
                    integer_type = false;

                    if (is_digit(*input_))
                    {
                        // if integer part is zero (parse '-0.0000...ddd')
                        if ((buffer[0] == '0') || (buffer[0] == '-' && buffer[1] == '0'))
                        {
                            while (*input_ == '0')
                                if (exp_offset > std::numeric_limits<int>::lowest()) --exp_offset, ++input_; // drop '0'
                                else throw bad_format("invalid number format: too long integer sequence");
                        }

                        while (is_digit(*input_))
                        {
                            if (p < fraction_limit) *p++ = static_cast<char_type>(*input_++); // put digit
                            else ++input_;                                                    // drop digit
                        }
                    }
                    else throw bad_format("invalid number format: expected a digit", *input_);
                }

                if (*input_ == 'e' || *input_ == 'E') // accept exponential part
                {
                    ++input_; // 'e'
                    integer_type = false;

                    char_type exp_part[32]{};
                    char_type* exp = exp_part;
                    char_type* const exp_limit = exp_part + std::size(exp_part);

                    // read

                    if (input_.eat('-')) { *exp++ = '-'; }
                    else if (input_.eat('+')) {} // drop '+'

                    if (is_digit(*input_))
                    {
                        while (is_digit(*input_))
                        {
                            if (exp < exp_limit) *exp++ = static_cast<char_type>(*input_++); // put digit
                            else ++input_;                                                   // drop digit (it must be overflow, handle later)
                        }
                    }
                    else throw bad_format("invalid number format: expected a digit", *input_);

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

                // try to parse as floating type (should succeed)
                {
                    json_t::floating_t ret{};
#if (defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L) // compiler has floating-point from_chars
                    auto [ptr, ec] = std::from_chars(buffer, p, ret);
#else // use fallback implementation
                    std::istringstream tmp{std::string(buffer, p)};
                    tmp.imbue(std::locale::classic());
                    tmp >> ret;
                    std::errc ec = !tmp ? std::errc::result_out_of_range : std::errc{}; // if fails, assume it must be out of range.
                    const char* ptr = !tmp ? p : tmp.eof() ? p : buffer + tmp.tellg();
#endif

                    assert(ec == std::errc{} || ec == std::errc::result_out_of_range);
                    assert(ptr == p);

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

                assert(false);
                throw bad_format("invalid number format: failed to parse");
            }

            json_t::string_t read_string()
            {
                // check quote character
                assert(*input_ == '"');
                int_type quote = *input_++; // '"'

                json_t::string_t ret;
                while (true)
                {
                    if (input_.eat('\\')) // escape sequence
                    {
                        if (*input_ == 'n') ret += '\n';
                        else if (*input_ == 't') ret += '\t';
                        else if (*input_ == 'b') ret += '\b';
                        else if (*input_ == 'f') ret += '\f';
                        else if (*input_ == 'r') ret += '\r';
                        else if (*input_ == '\\') ret += '\\';
                        else if (*input_ == '/') ret += '/';
                        else if (*input_ == '\"') ret += '\"';
                        else if (*input_ == '\'') ret += '\'';
                        else if (*input_ == 'u')
                        {
                            auto hex = [&](int_type chr)
                            {
                                if (chr >= '0' && chr <= '9') { return chr - '0'; }
                                if (chr >= 'A' && chr <= 'F') { return chr - 'A' + 10; }
                                if (chr >= 'a' && chr <= 'f') { return chr - 'a' + 10; }
                                throw bad_format("invalid string format: expected hexadecimal digit for \\u????", chr);
                            };

                            int code = 0;
                            code = code << 4 | hex(*++input_);
                            code = code << 4 | hex(*++input_);
                            code = code << 4 | hex(*++input_);
                            code = code << 4 | hex(*++input_);

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
                                if (*++input_ != '\\') throw bad_format("invalid string format: expected surrogate pair", *input_);
                                if (*++input_ != 'u') throw bad_format("invalid string format: expected surrogate pair", *input_);

                                int code2 = 0;
                                code2 = code2 << 4 | hex(*++input_);
                                code2 = code2 << 4 | hex(*++input_);
                                code2 = code2 << 4 | hex(*++input_);
                                code2 = code2 << 4 | hex(*++input_);

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
                    else if (input_.eat(quote)) break; // end of string.
                    else if (*input_ == EOF) throw bad_format("invalid string format: unexpected eof");
                    else if (*input_ < 0x20 || *input_ == 0x7F) throw bad_format("invalid string format: control character is not allowed", *input_);
                    else if (*input_ == '/' && !has_option(option::allow_unescaped_forward_slash)) throw bad_format("invalid string format: unescaped '/' is not allowed");
                    else ret += static_cast<char_type>(*input_); // OK. normal character.
                    ++input_;
                }

                return ret;
            }

            json_t::array_t read_array()
            {
                if (!input_.eat('[')) throw bad_format("logic error");
                eat_whitespaces();

                if (input_.eat(']')) return json_t::array_t{}; // empty array

                json_t::array_t ret{};
                while (true)
                {
                    // read value
                    ret.push_back(read_element());

                    eat_whitespaces();

                    if (input_.eat(','))
                    {
                        eat_whitespaces();
                        if (has_option(option::allow_trailing_comma) && input_.eat(']')) break;
                        else if (*input_ == ']') throw bad_format("invalid array format: expected an element (trailing comma not allowed)", *input_);
                    }
                    else if (input_.eat(']')) break;
                    else throw bad_format("invalid array format: ',' or ']' expected", *input_);
                }
                return ret;
            }

            json_t::object_t read_object()
            {
                if (!input_.eat('{')) throw bad_format("logic error");

                eat_whitespaces();

                if (input_.eat('}')) return json_t::object_t{}; // empty object_t

                json_t::object_t ret;
                while (true)
                {
                    // read key
                    json_t::string_t key{};

                    if (*input_ == '"') key = read_string();
                    else if (has_option(option::allow_unquoted_object_key))
                    {
                        while (*input_ != EOF && *input_ > ' ' && *input_ != ':')
                            key += static_cast<char_type>(*input_++);
                    }
                    else throw bad_format("invalid object format: expected object key", *input_);

                    eat_whitespaces();

                    if (!input_.eat(':'))
                        throw bad_format("invalid object format: expected a ':'", *input_);

                    eat_whitespaces();

                    // read value
                    ret[key] = read_element();

                    eat_whitespaces();

                    // read ',' or '}'
                    if (input_.eat(','))
                    {
                        eat_whitespaces();
                        if (has_option(option::allow_trailing_comma) && input_.eat('}')) break;
                        else if (*input_ == '}') throw bad_format("invalid object format: expected an element (trailing comma not allowed)", *input_);
                    }
                    else if (input_.eat('}')) break;
                    else throw bad_format("invalid object format: expected ',' or '}'", *input_);
                }

                return ret;
            }

            void eat_utf8bom()
            {
                if (has_option(option::allow_utf8_bom) && input_.eat(0xEF)) // as beginning of UTF-8 BOM.
                {
                    if (!input_.eat(0xBB)) throw bad_format("invalid json format: UTF-8 BOM sequence expected... 0xBB", *input_);
                    if (!input_.eat(0xBF)) throw bad_format("invalid json format: UTF-8 BOM sequence expected... 0xBF", *input_);
                }
                else if (input_.eat(0xEF)) // beginning of UTF-8 BOM?
                {
                    throw bad_format("invalid json format: expected an element. (UTF-8 BOM not allowed)", *input_);
                }
            }

            void eat_whitespaces()
            {
                constexpr auto is_space = [](int_type i)-> bool
                {
                    return i == ' ' || i == '\t' || i == '\r' || i == '\n';
                };

                while (true)
                {
                    while (is_space(*input_)) ++input_;

                    if (has_option(option::allow_comment) && input_.eat('/'))
                    {
                        if (input_.eat('*')) // start of block comment
                        {
                            while (*input_ != EOF)
                            {
                                if (*input_++ == '*' && input_.eat('/'))
                                    break;
                            }
                        }
                        else if (input_.eat('/')) // start of line comment
                        {
                            while (*input_ != EOF)
                            {
                                if (*input_++ == '\n')
                                    break;
                            }
                        }

                        continue;
                    }

                    break;
                }
            }

            [[nodiscard]] nanojson2::bad_format bad_format(std::string_view reason, std::optional<int_type> but_encountered = std::nullopt) const
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
                        message << static_cast<char_type>(*but_encountered);
                        message << "'";
                    }
                    else
                    {
                        message << "(char)";
                        message << std::hex << std::setfill('0') << std::setw(2) << *but_encountered;
                    }
                }
                message << " at line ";
                message << (input_.current_position_line_ + 1);
                message << " column ";
                message << (input_.current_position_column_ + 1);
                message << ".";

                return nanojson2::bad_format{message.str()};
            }
        };

        template <class character_input_iterator>
        static json_t parse_json(character_input_iterator begin, character_input_iterator end, option loose = option::default_option)
        {
            return reader<character_input_iterator>::read_json(std::move(begin), std::move(end), loose);
        }

        template <
            class istream,
            std::enable_if_t<std::is_constructible_v<std::istreambuf_iterator<json_t::char_t>, istream&>>* = nullptr>
        static json_t read_json(istream& source, option loose = option::default_option)
        {
            return parse_json(
                std::istreambuf_iterator<json_t::char_t>(source),
                std::istreambuf_iterator<json_t::char_t>(),
                loose);
        }
    }

    /// json_reader
    class json_t::json_reader
    {
    public:
        using option = json_parser::option;

        // From stream
        static json_t read_json(json_istream& source, option opt = option::default_option)
        {
            return json_parser::read_json(source, opt);
        }

        // From string
        static json_t parse_json(json_string_view_t source, option opt = option::default_option)
        {
            return json_parser::parse_json(source.begin(), source.end(), opt);
        }
    };

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

#if (defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L) // compiler has floating-point to_chars
                json_t::char_t v_str[128]{};
                auto [ptr, ec] = std::to_chars(std::begin(v_str), std::end(v_str), v, format, precision);
                if (ec != std::errc{} || *ptr != '\0') throw bad_value("failed to to_chars(floating)");
#else // use fallback implementation
                std::ostringstream tmp{};
                tmp.imbue(std::locale::classic());
                if (format == std::chars_format::fixed) tmp << std::fixed;
                if (format == std::chars_format::scientific) tmp << std::scientific;
                tmp << std::setprecision(precision);
                tmp << v;
                auto v_str = tmp.str();
#endif

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

    using undefined_t = json_t::undefined_t;
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
