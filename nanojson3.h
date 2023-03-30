/** @file
 * nanojson: A Simple JSON Reader/Writer For C++17
 * Copyright (c) 2016-2023 ttsuki
 * This software is released under the MIT License.
 */

// https://github.com/ttsuki/nanojson
// This project is an experiment, proof-of-concept, implementation of json library on standard C++17 functions.

#pragma once
#ifndef NANOJSON3_H_INCLUDED
#define NANOJSON3_H_INCLUDED

#include <cstddef>
#include <cassert>
#include <cmath>

#include <type_traits>
#include <limits>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <memory>

#include <variant>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <vector>
#include <iterator>

#include <istream>
#include <ostream>
#include <iomanip>
#include <charconv>
#include <sstream>

#include <initializer_list>
#include <array>
#include <tuple>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#if !((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
#error "nanojson needs C++17 support."
#endif

#if !(defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L)
#pragma message("nanojson needs C++17 Elementary string conversions (P0067R5) including Floating-Point (FP) values support. See (https://en.cppreference.com/w/cpp/compiler_support/17#:~:text=Elementary%20string%20conversions) This time, falling back to an implementation with stringstream instead.")
#endif

namespace nanojson3
{
    namespace containers
    {
        template <class KeyType, class ValueType>
        using key_value_pair = std::pair<KeyType, ValueType>;

        // simple key-value storage
        template <class KeyType, class ValueType, class KeyEqualityComparer = std::equal_to<KeyType>, class Allocator = std::allocator<key_value_pair<KeyType, ValueType>>>
        class linear_kvs : std::vector<key_value_pair<KeyType, ValueType>, Allocator>
        {
        public:
            using key_type = KeyType;
            using mapped_type = ValueType;
            using pair_type = key_value_pair<KeyType, ValueType>;
            using key_compare = KeyEqualityComparer;
            using allocator_type = Allocator;

        private:
            using base_container = std::vector<pair_type, allocator_type>;
            key_compare key_equal_comparer_{};

        public:
            using value_type = typename base_container::value_type;
            using size_type = typename base_container::size_type;
            using difference_type = typename base_container::difference_type;
            using reference = typename base_container::reference;
            using const_reference = typename base_container::const_reference;
            using pointer = typename base_container::pointer;
            using const_pointer = typename base_container::const_pointer;
            using iterator = typename base_container::iterator;
            using const_iterator = typename base_container::const_iterator;
            using reverse_iterator = typename base_container::reverse_iterator;
            using const_reverse_iterator = typename base_container::const_reverse_iterator;

            linear_kvs() = default;
            explicit linear_kvs(const key_compare& comp, const allocator_type& alloc = allocator_type{}) : base_container(alloc), key_equal_comparer_(comp) {}
            explicit linear_kvs(const allocator_type& alloc) : base_container(alloc) {}
            template <class InputIterator> linear_kvs(InputIterator first, InputIterator last, const key_compare& comp = key_compare{}, const allocator_type& alloc = allocator_type{}) : base_container(first, last, alloc), key_equal_comparer_(comp) {}
            template <class InputIterator> linear_kvs(InputIterator first, InputIterator last, const allocator_type& alloc) : base_container(first, last, alloc) {}
            linear_kvs(const linear_kvs& other) = default;
            linear_kvs(const linear_kvs& other, const allocator_type& allocator) : base_container(other, allocator), key_equal_comparer_(other.comparer_) { }
            linear_kvs(linear_kvs&& other) noexcept = default;
            linear_kvs(linear_kvs&& other, const allocator_type& allocator) : base_container(std::move(other), allocator), key_equal_comparer_(other.comparer_) { }
            linear_kvs(std::initializer_list<pair_type> init, const key_compare& comp = key_compare{}, const allocator_type& alloc = allocator_type{}) : base_container(init, alloc), key_equal_comparer_(comp) {}
            linear_kvs(std::initializer_list<pair_type> init, const allocator_type& alloc) : base_container(init, alloc) {}
            ~linear_kvs() = default;

            linear_kvs& operator=(const linear_kvs& other) = default;
            linear_kvs& operator=(linear_kvs&& other) noexcept = default;

            bool operator ==(const linear_kvs& other) const noexcept { return static_cast<const base_container&>(*this) == static_cast<const base_container&>(other); }
            bool operator !=(const linear_kvs& other) const noexcept { return static_cast<const base_container&>(*this) != static_cast<const base_container&>(other); }

        public:
            using base_container::get_allocator;

            using base_container::begin;
            using base_container::end;
            using base_container::cbegin;
            using base_container::cend;
            using base_container::rbegin;
            using base_container::rend;

            using base_container::empty;
            using base_container::size;
            using base_container::max_size;
            using base_container::capacity;
            using base_container::reserve;

        private:
            enum struct enabled { };

            template <class... Predicates> using enable_if_t = std::enable_if_t<std::conjunction_v<Predicates...>, enabled>;

            template <class Key> using is_key_view_type = std::bool_constant<std::is_invocable_r_v<bool, KeyEqualityComparer, const Key&, key_type>>;
            template <class... Args> using key_is_constructible_from = std::bool_constant<std::is_constructible_v<key_type, Args...>>;
            template <class... Args> using value_is_constructible_from = std::bool_constant<std::is_constructible_v<mapped_type, Args...>>;
            template <class... Args> using kvp_is_constructible_from = std::bool_constant<std::is_constructible_v<value_type, Args...>>;

            template <class... Args>
            std::pair<iterator, bool> emplace_back(Args&&... args)
            {
                base_container::emplace_back(std::forward<Args>(args)...);
                return std::pair<iterator, bool>{base_container::end() - 1, true};
            }

        public:
            // O(n)
            template <class Key, enable_if_t<is_key_view_type<Key>>  = enabled{}>
            [[nodiscard]] size_type count(const Key& key) const noexcept
            {
                return find(key) != end() ? 1 : 0;
            }

            // O(n)
            template <class Key, enable_if_t<is_key_view_type<Key>>  = enabled{}>
            [[nodiscard]] iterator find(const Key& key) noexcept
            {
                for (auto it = begin(); it != end(); ++it)
                    if (key_equal_comparer_(key, it->first))
                        return it;
                return end();
            }

            // O(n)
            template <class Key, enable_if_t<is_key_view_type<Key>>  = enabled{}>
            [[nodiscard]] const_iterator find(const Key& key) const noexcept
            {
                for (auto it = begin(); it != end(); ++it)
                    if (key_equal_comparer_(key, it->first))
                        return it;
                return end();
            }

            // O(n)
            template <class KeyView, enable_if_t<is_key_view_type<KeyView>>  = enabled{}>
            [[nodiscard]] mapped_type& at(const KeyView& key)
            {
                if (auto it = find(key); it != end()) return it->second;
                else throw std::out_of_range("no such key");
            }

            // O(n)
            template <class KeyView, enable_if_t<is_key_view_type<KeyView>>  = enabled{}>
            [[nodiscard]] const mapped_type& at(const KeyView& key) const
            {
                if (auto it = find(key); it != end()) return it->second;
                else throw std::out_of_range("no such key");
            }

            // O(n)
            template <class KeyValuePair, enable_if_t<kvp_is_constructible_from<KeyValuePair>>  = enabled{}>
            std::pair<iterator, bool> insert(KeyValuePair&& value)
            {
                if (auto it = find(value.first); it != end()) return {it, false};
                return emplace_back(std::forward<decltype(value)>(value));
            }

            // O(n)
            template <class... Args, enable_if_t<kvp_is_constructible_from<Args...>>  = enabled{}>
            std::pair<iterator, bool> emplace(Args&&... args)
            {
                auto ret = emplace_back(std::forward<Args>(args)...);
                if (auto it = find(ret.first->first); it != ret.first)
                {
                    base_container::pop_back();
                    return {it, false};
                }
                return ret;
            }

            // O(n)
            template <class Key, class Value, enable_if_t<key_is_constructible_from<Key>, value_is_constructible_from<Value>>  = enabled{}>
            std::pair<iterator, bool> insert_or_assign(Key&& key, Value&& val)
            {
                if (auto it = find(key); it != end())
                {
                    it->second = std::forward<Value>(val);
                    return {it, false};
                }
                else
                {
                    return emplace_back(
                        std::piecewise_construct,
                        std::forward_as_tuple(std::forward<decltype(key)>(key)),
                        std::forward_as_tuple(std::forward<decltype(val)>(val)));
                }
            }

            // O(n)
            template <class Key, class... ValueArgs, enable_if_t<key_is_constructible_from<Key>, value_is_constructible_from<ValueArgs...>>  = enabled{}>
            std::pair<iterator, bool> try_emplace(Key&& key, ValueArgs&&... args)
            {
                if (auto it = find(key); it != end())
                    return {it, false};
                else
                    return emplace_back(
                        std::piecewise_construct,
                        std::forward_as_tuple(std::forward<decltype(key)>(key)),
                        std::forward_as_tuple(std::forward<decltype(args)>(args)...));
            }

            // O(n)
            template <class Key, enable_if_t<key_is_constructible_from<Key>>  = enabled{}>
            mapped_type& operator [](const Key& key)
            {
                if (auto it = find(key); it != end())
                    return it->second;
                else
                    return emplace_back(
                        std::piecewise_construct,
                        std::forward_as_tuple(std::forward<decltype(key)>(key)),
                        std::forward_as_tuple()).first->second;
            }

            // O(1)
            using base_container::clear;

            // O(n)
            using base_container::erase;

            // O(n)
            template <class Key, enable_if_t<is_key_view_type<Key>>  = enabled{}>
            size_type erase(const Key& key)
            {
                if (auto it = find(key); it != end())
                {
                    base_container::erase(it);
                    return 1;
                }
                return 0;
            }
        };
    }

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
        struct bad_format final : nanojson_exception
        {
            using nanojson_exception::nanojson_exception;
        };

        /// bad_value: represents FAILED to encode to json string.
        struct bad_value final : nanojson_exception
        {
            using nanojson_exception::nanojson_exception;
        };
    }

    enum struct json_type_index
    {
        undefined,
        null,
        boolean,
        integer,
        floating,
        string,
        array,
        object,
    };

    template <json_type_index i>
    struct in_place_index_t : std::in_place_index_t<static_cast<size_t>(i)>
    {
        constexpr in_place_index_t() : std::in_place_index_t<static_cast<size_t>(i)>{} {}
    };

    struct in_place_index
    {
        static constexpr inline in_place_index_t<json_type_index::undefined> undefined{};
        static constexpr inline in_place_index_t<json_type_index::null> null{};
        static constexpr inline in_place_index_t<json_type_index::boolean> boolean{};
        static constexpr inline in_place_index_t<json_type_index::integer> integer{};
        static constexpr inline in_place_index_t<json_type_index::floating> floating{};
        static constexpr inline in_place_index_t<json_type_index::string> string{};
        static constexpr inline in_place_index_t<json_type_index::array> array{};
        static constexpr inline in_place_index_t<json_type_index::object> object{};
    };

    enum struct json_parse_option : unsigned long
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

    static inline constexpr json_parse_option operator |(json_parse_option lhs, json_parse_option rhs) { return static_cast<json_parse_option>(static_cast<std::underlying_type_t<json_parse_option>>(lhs) | static_cast<std::underlying_type_t<json_parse_option>>(rhs)); }
    static inline constexpr json_parse_option operator &(json_parse_option lhs, json_parse_option rhs) { return static_cast<json_parse_option>(static_cast<std::underlying_type_t<json_parse_option>>(lhs) & static_cast<std::underlying_type_t<json_parse_option>>(rhs)); }
    static inline constexpr json_parse_option operator ^(json_parse_option lhs, json_parse_option rhs) { return static_cast<json_parse_option>(static_cast<std::underlying_type_t<json_parse_option>>(lhs) ^ static_cast<std::underlying_type_t<json_parse_option>>(rhs)); }
    static inline constexpr json_parse_option& operator |=(json_parse_option& lhs, json_parse_option rhs) { return lhs = lhs | rhs; }
    static inline constexpr json_parse_option& operator &=(json_parse_option& lhs, json_parse_option rhs) { return lhs = lhs & rhs; }
    static inline constexpr json_parse_option& operator ^=(json_parse_option& lhs, json_parse_option rhs) { return lhs = lhs ^ rhs; }

    enum struct json_serialize_option : unsigned long
    {
        none = 0,
        pretty = 1ul << 0,
        debug_dump_type_as_comment = 1ul << 31,

        // default_option
        default_option = none,
    };

    static inline constexpr json_serialize_option operator |(json_serialize_option lhs, json_serialize_option rhs) { return static_cast<json_serialize_option>(static_cast<std::underlying_type_t<json_serialize_option>>(lhs) | static_cast<std::underlying_type_t<json_serialize_option>>(rhs)); }
    static inline constexpr json_serialize_option operator &(json_serialize_option lhs, json_serialize_option rhs) { return static_cast<json_serialize_option>(static_cast<std::underlying_type_t<json_serialize_option>>(lhs) & static_cast<std::underlying_type_t<json_serialize_option>>(rhs)); }
    static inline constexpr json_serialize_option operator ^(json_serialize_option lhs, json_serialize_option rhs) { return static_cast<json_serialize_option>(static_cast<std::underlying_type_t<json_serialize_option>>(lhs) ^ static_cast<std::underlying_type_t<json_serialize_option>>(rhs)); }
    static inline constexpr json_serialize_option& operator |=(json_serialize_option& lhs, json_serialize_option rhs) { return lhs = lhs | rhs; }
    static inline constexpr json_serialize_option& operator &=(json_serialize_option& lhs, json_serialize_option rhs) { return lhs = lhs & rhs; }
    static inline constexpr json_serialize_option& operator ^=(json_serialize_option& lhs, json_serialize_option rhs) { return lhs = lhs ^ rhs; }

    struct json_floating_format_options
    {
        std::chars_format floating_format = std::chars_format::general; // general, fixed, scientific
        int floating_precision = 7;
    };

    /// json: represents a json element
    class json final
    {
    public:
        // types

        using char_type = char;
        using char_traits = std::char_traits<char_type>;
        using allocator_type = std::allocator<char_type>;
        using allocator_traits = std::allocator_traits<allocator_type>;
        template <class U> using allocator_type_for = typename allocator_traits::template rebind_alloc<U>;

        using js_undefined = std::monostate;
        using js_null = std::nullptr_t;
        using js_boolean = bool;
        using js_integer = long long int;
        using js_floating = long double;
        using js_number = js_floating; // integer or floating
        using js_string = std::basic_string<char_type, char_traits, allocator_type_for<char_type>>;
        using js_string_view = std::basic_string_view<char_type, char_traits>;
        using js_array_index = size_t;
        using js_array_index_view = size_t;
        using js_array = std::vector<json, allocator_type_for<json>>;
        using js_object_key = js_string;
        using js_object_key_view = js_string_view;
        using js_object_kvp = std::pair<js_object_key, json>;
        using js_object = containers::linear_kvs<js_object_key, json, std::equal_to<>, allocator_type_for<js_object_kvp>>;

        using js_variant = std::variant<js_undefined, js_null, js_boolean, js_integer, js_floating, js_string, js_array, js_object>;

        using json_string = std::basic_string<char_type, char_traits, allocator_type_for<char_type>>;
        using json_string_view = std::basic_string_view<char_type, char_traits>;

    public:
        /// json_value: holds a json element value
        class json_value final
        {
            js_variant value_{};

        private:
            friend class json;

            json_value() = default;
            json_value(const json_value& other) = default;
            json_value(json_value&& other) noexcept = default;

            template <json_type_index type_index, class... Args>
            json_value(in_place_index_t<type_index> index, Args&&... args)
                : value_(index, std::forward<Args>(args)...) { }

            json_value& operator=(const json_value& other) = default;
            json_value& operator=(json_value&& other) noexcept = default;

            ~json_value() = default;

        public:
            bool operator ==(const json_value& rhs) const noexcept { return value_ == rhs.value_; }
            bool operator !=(const json_value& rhs) const noexcept { return value_ != rhs.value_; }

            [[nodiscard]] json_type_index get_type() const noexcept { return static_cast<json_type_index>(value_.index()); }
            [[nodiscard]] const js_variant& as_variant() const noexcept { return value_; }

            template <class T> [[nodiscard]] bool is() const noexcept { return std::holds_alternative<T>(value_); }
            [[nodiscard]] bool is_defined() const noexcept { return !is<js_undefined>(); }
            [[nodiscard]] bool is_undefined() const noexcept { return is<js_undefined>(); }
            [[nodiscard]] bool is_null() const noexcept { return is<js_null>(); }
            [[nodiscard]] bool is_bool() const noexcept { return is<js_boolean>(); }
            [[nodiscard]] bool is_integer() const noexcept { return is<js_integer>(); }
            [[nodiscard]] bool is_floating() const noexcept { return is<js_floating>(); }
            [[nodiscard]] bool is_string() const noexcept { return is<js_string>(); }
            [[nodiscard]] bool is_array() const noexcept { return is<js_array>(); }
            [[nodiscard]] bool is_object() const noexcept { return is<js_object>(); }

            // returns nullptr if type is mismatch
            template <class T> [[nodiscard]] const T* as() const noexcept { return std::get_if<T>(&value_); }
            [[nodiscard]] const js_null* as_null() const noexcept { return as<js_null>(); }
            [[nodiscard]] const js_boolean* as_bool() const noexcept { return as<js_boolean>(); }
            [[nodiscard]] const js_integer* as_integer() const noexcept { return as<js_integer>(); }
            [[nodiscard]] const js_floating* as_floating() const noexcept { return as<js_floating>(); }
            [[nodiscard]] const js_string* as_string() const noexcept { return as<js_string>(); }
            [[nodiscard]] const js_array* as_array() const noexcept { return as<js_array>(); }
            [[nodiscard]] const js_object* as_object() const noexcept { return as<js_object>(); }

            // returns nullptr if type is mismatch
            template <class T> [[nodiscard]] T* as() noexcept { return std::get_if<T>(&value_); }
            [[nodiscard]] js_null* as_null() noexcept { return as<js_null>(); }
            [[nodiscard]] js_boolean* as_bool() noexcept { return as<js_boolean>(); }
            [[nodiscard]] js_integer* as_integer() noexcept { return as<js_integer>(); }
            [[nodiscard]] js_floating* as_floating() noexcept { return as<js_floating>(); }
            [[nodiscard]] js_string* as_string() noexcept { return as<js_string>(); }
            [[nodiscard]] js_array* as_array() noexcept { return as<js_array>(); }
            [[nodiscard]] js_object* as_object() noexcept { return as<js_object>(); }

            // throws bad_access if type is mismatch
            template <class T> [[nodiscard]] T get() const { return as<T>() ? *as<T>() : throw bad_access(); }
            [[nodiscard]] js_null get_null() const { return get<js_null>(); }
            [[nodiscard]] js_boolean get_bool() const { return get<js_boolean>(); }
            [[nodiscard]] js_integer get_integer() const { return get<js_integer>(); }
            [[nodiscard]] js_floating get_floating() const { return get<js_floating>(); }
            [[nodiscard]] js_string get_string() const { return get<js_string>(); }
            [[nodiscard]] js_array get_array() const { return get<js_array>(); }
            [[nodiscard]] js_object get_object() const { return get<js_object>(); }

            // returns default_value if type is mismatch
            template <class T, class U, std::enable_if_t<std::is_convertible_v<U&&, T>>* = nullptr> [[nodiscard]] T get_or(U&& default_value) const noexcept { return as<T>() ? *as<T>() : static_cast<T>(std::forward<U>(default_value)); }
            [[nodiscard]] js_null get_null_or(js_null default_value) const noexcept { return get_or<js_null>(default_value); }
            [[nodiscard]] js_boolean get_bool_or(js_boolean default_value) const noexcept { return get_or<js_boolean>(default_value); }
            [[nodiscard]] js_integer get_integer_or(js_integer default_value) const noexcept { return get_or<js_integer>(default_value); }
            [[nodiscard]] js_floating get_floating_or(js_floating default_value) const noexcept { return get_or<js_floating>(default_value); }
            [[nodiscard]] js_string get_string_or(const js_string& default_value) const noexcept { return get_or<js_string>(default_value); }
            [[nodiscard]] js_string get_string_or(js_string&& default_value) const noexcept { return get_or<js_string>(std::move(default_value)); }
            [[nodiscard]] js_array get_array_or(const js_array& default_value) const noexcept { return get_or<js_array>(default_value); }
            [[nodiscard]] js_array get_array_or(js_array&& default_value) const noexcept { return get_or<js_array>(std::move(default_value)); }
            [[nodiscard]] js_object get_object_or(const js_object& default_value) const noexcept { return get_or<js_object>(default_value); }
            [[nodiscard]] js_object get_object_or(js_object&& default_value) const noexcept { return get_or<js_object>(std::move(default_value)); }

            // (integer or floating) as floating
            [[nodiscard]] bool is_number() const noexcept
            {
                return is<js_integer>() || is<js_floating>();
            }

            // (integer or floating) as floating
            [[nodiscard]] std::optional<js_number> as_number() const noexcept
            {
                if (const auto num = as<js_integer>()) return std::optional<js_number>(std::in_place, static_cast<js_floating>(*num));
                if (const auto num = as<js_floating>()) return std::optional<js_number>(std::in_place, *num);
                return std::optional<js_number>{};
            }

            // (integer or floating) as floating
            [[nodiscard]] js_number get_number() const
            {
                if (const auto num = as_number()) return *num;
                throw bad_access();
            }

            // (integer or floating) as floating
            template <class U, std::enable_if_t<std::is_convertible_v<U&&, js_number>>* = nullptr>
            [[nodiscard]] js_number get_number_or(U&& default_value) const
            {
                if (const auto num = as_number()) return *num;
                return static_cast<js_number>(std::forward<U>(default_value));
            }
        };

    private:
        json_value value_{};

    public:
        // constructors
        json() { }
        json(const json& other) = default;
        json(json&& other) noexcept = default;
        json& operator=(const json& other) = default;
        json& operator=(json&& other) noexcept = default;

        template <json_type_index type_index, class... Args>
        json(in_place_index_t<type_index> index, Args&&... args) : value_(index, std::forward<Args>(args)...) { }

        json(const js_undefined& value) : json(in_place_index::undefined, std::forward<decltype(value)>(value)) { }
        json(const js_null& value) : json(in_place_index::null, std::forward<decltype(value)>(value)) { }
        json(const js_boolean& value) : json(in_place_index::boolean, std::forward<decltype(value)>(value)) { }
        json(const js_integer& value) : json(in_place_index::integer, std::forward<decltype(value)>(value)) { }
        json(const js_floating& value) : json(in_place_index::floating, std::forward<decltype(value)>(value)) { }
        json(const js_string& value) : json(in_place_index::string, std::forward<decltype(value)>(value)) { }
        json(const js_array& value) : json(in_place_index::array, std::forward<decltype(value)>(value)) { }
        json(const js_object& value) : json(in_place_index::object, std::forward<decltype(value)>(value)) { }

        json(js_undefined&& value) : json(in_place_index::undefined, std::forward<decltype(value)>(value)) { }
        json(js_null&& value) : json(in_place_index::null, std::forward<decltype(value)>(value)) { }
        json(js_boolean&& value) : json(in_place_index::boolean, std::forward<decltype(value)>(value)) { }
        json(js_integer&& value) : json(in_place_index::integer, std::forward<decltype(value)>(value)) { }
        json(js_floating&& value) : json(in_place_index::floating, std::forward<decltype(value)>(value)) { }
        json(js_string&& value) : json(in_place_index::string, std::forward<decltype(value)>(value)) { }
        json(js_array&& value) : json(in_place_index::array, std::forward<decltype(value)>(value)) { }
        json(js_object&& value) : json(in_place_index::object, std::forward<decltype(value)>(value)) { }

        ~json() = default;

    public:
        // undefined_reference
        [[nodiscard]] static const json& undefined_reference() noexcept
        {
            static json undefined{js_undefined{}};
            return undefined;
        }

    public:
        // value access operators
        [[nodiscard]] const json_value& value() const noexcept { return value_; }
        [[nodiscard]] const json_value& operator *() const noexcept { return value(); }
        [[nodiscard]] const json_value* operator ->() const noexcept { return &value(); }
        [[nodiscard]] json_value& value() noexcept { return value_; }
        [[nodiscard]] json_value& operator *() noexcept { return value(); }
        [[nodiscard]] json_value* operator ->() noexcept { return &value(); }

        // array/object index operators

        using const_node_reference = const json&;
        struct node_reference;

        const_node_reference operator[](js_array_index key) const noexcept
        {
            if (const auto a = value().as_array())
                if (key < a->size())
                    return a->operator[](key);

            return undefined_reference();
        }

        const_node_reference operator[](js_object_key_view key) const noexcept
        {
            if (const auto o = value().as_object())
                if (const auto it = o->find(key); it != o->end())
                    return it->second;

            return undefined_reference();
        }

        struct node_reference final
        {
            using undefined_pointer = std::monostate;
            using normal_pointer = json*;
            using array_write_pointer = std::pair<js_array*, js_array::size_type>;
            using object_write_pointer = std::pair<js_object*, js_object::key_type>;
            using pointer = std::variant<undefined_pointer, normal_pointer, array_write_pointer, object_write_pointer>;

            node_reference(pointer p) : pointer_(std::move(p)) { }
            node_reference(json& r) : pointer_(std::in_place_type<normal_pointer>, &r) { }
            node_reference(const node_reference& other) = delete;
            node_reference(node_reference&& other) noexcept = delete;
            node_reference& operator=(const node_reference& other) = delete;
            node_reference& operator=(node_reference&& other) noexcept = delete;
            ~node_reference() = default;

            [[nodiscard]] json_value& value() const noexcept
            {
                if (const normal_pointer* p = std::get_if<normal_pointer>(&pointer_)) return (*p)->value();
                return const_cast<json_value&>(undefined_reference().value()); // assumes the value type can't be changed from json_value_t interface.
            }

            [[nodiscard]] json_value& operator *() const noexcept { return value(); }
            [[nodiscard]] json_value* operator ->() const noexcept { return &value(); }

            [[nodiscard]] node_reference operator [](js_array_index key) const noexcept
            {
                if (const normal_pointer* p = std::get_if<normal_pointer>(&pointer_))
                {
                    if (const auto a = (*p)->value().as_array())
                    {
                        if (key < a->size())
                            return pointer(std::in_place_type<normal_pointer>, &a->operator[](key)); // normal reference
                        else
                            return pointer(std::in_place_type<array_write_pointer>, a, key); // write only reference
                    }
                }

                return pointer{undefined_pointer{}};
            }

            [[nodiscard]] node_reference operator [](js_object_key_view key) const noexcept
            {
                if (const normal_pointer* p = std::get_if<normal_pointer>(&pointer_))
                {
                    if (const auto o = (*p)->value().as_object())
                    {
                        if (const auto it = o->find(key); it != o->end())
                            return pointer(std::in_place_type<normal_pointer>, &it->second); // normal reference
                        else
                            return pointer{std::in_place_type<object_write_pointer>, o, js_object::key_type(key)}; // write only reference
                    }
                }

                return pointer{undefined_pointer{}};
            }

            // assign operator
            json& operator =(json value)
            {
                if (normal_pointer* pn = std::get_if<normal_pointer>(&pointer_))
                {
                    return **pn = std::move(value);
                }
                else if (array_write_pointer* pa = std::get_if<array_write_pointer>(&pointer_))
                {
                    auto [a, i] = *pa;
                    if (i >= a->size()) a->resize(i + 1);
                    json& r = a->operator[](i) = std::move(value);
                    pointer_ = normal_pointer{&r}; // update pointer to real instance
                    return r;
                }
                else if (object_write_pointer* po = std::get_if<object_write_pointer>(&pointer_))
                {
                    auto [o, k] = *po;
                    json& r = o->insert_or_assign(std::move(k), std::move(value)).first->second;
                    pointer_ = normal_pointer{&r}; // update pointer to real instance
                    return r;
                }
                else throw bad_access(); // can't write to undefined node
            }

            bool operator ==(const node_reference& rhs) const noexcept { return this->value() == rhs.value(); }
            bool operator !=(const node_reference& rhs) const noexcept { return this->value() != rhs.value(); }
            bool operator ==(const_node_reference rhs) const noexcept { return this->value() == rhs.value(); }
            bool operator !=(const_node_reference rhs) const noexcept { return this->value() != rhs.value(); }

        private:
            pointer pointer_{};
        };

        [[nodiscard]] node_reference operator[](js_array_index index) noexcept { return node_reference(*this)[index]; }
        [[nodiscard]] node_reference operator[](js_object_key_view key) noexcept { return node_reference(*this)[key]; }

        [[nodiscard]] bool operator ==(const node_reference& rhs) const noexcept { return this->value() == rhs.value(); }
        [[nodiscard]] bool operator !=(const node_reference& rhs) const noexcept { return this->value() != rhs.value(); }
        [[nodiscard]] bool operator ==(const_node_reference rhs) const noexcept { return this->value() == rhs.value(); }
        [[nodiscard]] bool operator !=(const_node_reference rhs) const noexcept { return this->value() != rhs.value(); }

    public:
        // i/o
        template <class CharInputIterator> struct json_reader;
        template <class CharOutputIterator> struct json_writer;

        [[nodiscard]] static json parse(const json_string_view& source, json_parse_option opt = json_parse_option::default_option);
        [[nodiscard]] json_string serialize(json_serialize_option opt = json_serialize_option::none, json_floating_format_options format = json_floating_format_options{}) const;

    public:
        // extensive constructors
        template <class T, class U = void> struct json_ext;

        // copy construct
        template <class T, std::void_t<decltype(json_ext<std::decay_t<const T&>>::serialize(std::declval<const T&>()))>* = nullptr>
        json(const T& value) : json(json_ext<std::decay_t<const T&>>::serialize(std::forward<const T>(value))) { }

        // move construct
        template <class T, std::enable_if_t<std::is_rvalue_reference_v<T>, std::void_t<decltype(json_ext<std::decay_t<T>>::serialize(std::declval<T>()))>>* = nullptr>
        json(T&& value) : json(json_ext<std::decay_t<T>>::serialize(std::forward<T>(value))) { }
    };

    template <class CharInputIterator>
    struct json::json_reader
    {
    public:
        static json read_json(CharInputIterator begin, CharInputIterator end, json_parse_option loose_option)
        {
            return json_reader(begin, end, loose_option).execute();
        }

    private:
        using char_traits = typename json::char_traits;
        using char_type = typename char_traits::char_type;
        using int_type = typename char_traits::int_type;

        struct input_stream
        {
            CharInputIterator it_;
            CharInputIterator const end_;

            size_t current_position_char_{};
            int current_position_line_{};
            int current_position_column_{};

            // peeks current character
            [[nodiscard]] int_type peek() const
            {
                return it_ != end_ ? char_traits::to_int_type(*it_) : char_traits::eof();
            }

            // eats a character
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

            // eats if current character is `chr`
            [[nodiscard]] bool eat(int_type chr)
            {
                return peek() == chr ? (void)eat(), true : false;
            }

            int_type operator *() const
            {
                return peek();
            }

            input_stream& operator ++()
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

        json_parse_option option_bits_{};

        // ctor
        json_reader(CharInputIterator begin, CharInputIterator end, json_parse_option option) : input_{begin, end}, option_bits_(option) { }

        // executes parsing
        [[nodiscard]] json execute()
        {
            eat_utf8bom();
            eat_whitespaces();
            return read_element();
        }

        // gets the option bit enabled.
        [[nodiscard]] bool has_option(json_parse_option bit) const noexcept
        {
            return (option_bits_ & bit) != json_parse_option::none;
        }

        // reads single node from the stream.
        json read_element()
        {
            switch (*input_)
            {
            case 'n': // `null`
                if (!input_.eat('n')) throw bad_format("invalid 'null' literal: expected 'n'", *input_);
                if (!input_.eat('u')) throw bad_format("invalid 'null' literal: expected 'u'", *input_);
                if (!input_.eat('l')) throw bad_format("invalid 'null' literal: expected 'l'", *input_);
                if (!input_.eat('l')) throw bad_format("invalid 'null' literal: expected 'l'", *input_);
                return json{in_place_index::null};

            case 't': // `true`
                if (!input_.eat('t')) throw bad_format("invalid 'true' literal: expected 't'", *input_);
                if (!input_.eat('r')) throw bad_format("invalid 'true' literal: expected 'r'", *input_);
                if (!input_.eat('u')) throw bad_format("invalid 'true' literal: expected 'u'", *input_);
                if (!input_.eat('e')) throw bad_format("invalid 'true' literal: expected 'e'", *input_);
                return json{in_place_index::boolean, true};

            case 'f': // `false`
                if (!input_.eat('f')) throw bad_format("invalid 'false' literal: expected 'f'", *input_);
                if (!input_.eat('a')) throw bad_format("invalid 'false' literal: expected 'a'", *input_);
                if (!input_.eat('l')) throw bad_format("invalid 'false' literal: expected 'l'", *input_);
                if (!input_.eat('s')) throw bad_format("invalid 'false' literal: expected 's'", *input_);
                if (!input_.eat('e')) throw bad_format("invalid 'false' literal: expected 'e'", *input_);
                return json{in_place_index::boolean, false};

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

        // reads integer or floating
        json read_number()
        {
            constexpr auto is_digit = [](int_type i)-> bool { return i >= '0' && i <= '9'; }; // locale-independent is_digit

            char buffer[128]{};
            char* p = buffer;
            char* const integer_limit = buffer + 60;
            char* const fraction_limit = buffer + 64;
            char* const decimal_limit = buffer + 128;

            int exp_offset = 0;
            constexpr auto maximum_exp_offset = (std::numeric_limits<decltype(exp_offset)>::max)();
            constexpr auto minimum_exp_offset = (std::numeric_limits<decltype(exp_offset)>::lowest)();

            bool integer_type = true;

            // parse integer part
            {
                if (input_.eat('-')) { *p++ = '-'; }                                                  // minus sign
                if (has_option(json_parse_option::allow_number_with_plus_sign) && input_.eat('+')) {} // ignore plus sign

                if (input_.eat('0')) { *p++ = '0'; } // put '0', leading zeros are not allowed in JSON.
                else if (is_digit(*input_))          // not zero
                {
                    while (is_digit(*input_))
                        if (p < integer_limit) *p++ = static_cast<char_type>(*input_++);  // put digit
                        else if (exp_offset < maximum_exp_offset) ++exp_offset, ++input_; // drop digit
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
                            if (exp_offset > minimum_exp_offset) --exp_offset, ++input_; // drop '0'
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

            if (*input_ == 'e' || *input_ == 'E') // accept exponent part
            {
                ++input_; // 'e'
                integer_type = false;

                char exp_part[32]{};
                char* exp = exp_part;
                char* const exp_limit = exp_part + std::size(exp_part);

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

                // parse exponent
                int exp_value{};
                auto [ptr, ec] = std::from_chars(exp_part, exp, exp_value, 10);

                if (ec == std::errc{} && ptr == exp)
                {
                    exp_offset = exp_offset > 0 && exp_value > maximum_exp_offset - exp_offset ? maximum_exp_offset : exp_offset < 0 && exp_value < minimum_exp_offset - exp_offset ? minimum_exp_offset : exp_offset + exp_value;
                }
                else if (ec == std::errc::result_out_of_range && ptr == exp)
                {
                    exp_offset = exp_part[0] == '-' ? maximum_exp_offset : minimum_exp_offset;
                }
                else // other error (bug)
                {
                    assert(false);
                    throw bad_format("invalid number format: unexpected parse error "); // unexpected
                }
            }

            if (exp_offset != 0) // append parsed exponent part to buffer
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
                json::js_integer ret{};
                auto [ptr, ec] = std::from_chars(buffer, p, ret, 10);
                if (ec == std::errc{} && ptr == p) return json{in_place_index::integer, ret}; // integer OK
            }

            // try to parse as floating type (should succeed)
            {
                json::js_floating ret{};
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

                if (ptr == p && ec == std::errc{}) return json{in_place_index::floating, ret}; // floating OK

                if (ec == std::errc::result_out_of_range) // overflow or underflow
                {
                    if (exp_offset >= 0) // overflow
                        return json{in_place_index::floating, buffer[0] != '-' ? +std::numeric_limits<json::js_floating>::infinity() : -std::numeric_limits<json::js_floating>::infinity()};
                    else // underflow
                        return json{in_place_index::floating, buffer[0] != '-' ? +static_cast<json::js_floating>(+0.0) : -static_cast<json::js_floating>(-0.0)};
                }
            }

            assert(false);
            throw bad_format("invalid number format: failed to parse");
        }

        // reads quoated string
        json read_string()
        {
            // check quote character
            assert(*input_ == '"');
            int_type quote = *input_++; // '"'

            json result(in_place_index::string);         // make empty string
            json::js_string& ret = *result->as_string(); // and get reference to it.

            while (true)
            {
                if (input_.eat('\\')) // escape sequence found?
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
                    else if (*input_ == 'u') // \uXXXX is converted to UTF-8 sequence
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
                else if (*input_ == '/' && !has_option(json_parse_option::allow_unescaped_forward_slash)) throw bad_format("invalid string format: unescaped '/' is not allowed");
                else ret += static_cast<char_type>(*input_); // OK. normal character.
                ++input_;
            }

            return result;
        }

        // reads array `[...]`
        json read_array()
        {
            if (!input_.eat('[')) throw bad_format("logic error (bug)");

            json result(in_place_index::array);        // make empty array
            json::js_array& ret = *result->as_array(); // and get reference to it.

            eat_whitespaces();

            if (input_.eat(']')) return result; // empty array

            while (true)
            {
                // read value
                ret.push_back(read_element());

                eat_whitespaces();

                if (input_.eat(','))
                {
                    eat_whitespaces();
                    if (has_option(json_parse_option::allow_trailing_comma) && input_.eat(']')) break;
                    else if (*input_ == ']') throw bad_format("invalid array format: expected an element (trailing comma not allowed)", *input_);
                }
                else if (input_.eat(']')) break;
                else throw bad_format("invalid array format: ',' or ']' expected", *input_);
            }

            return result;
        }

        // reads object `{...}`
        json read_object()
        {
            if (!input_.eat('{')) throw bad_format("logic error (bug)");

            json result(in_place_index::object);         // make empty object
            json::js_object& ret = *result->as_object(); // and get reference to it.

            eat_whitespaces();

            if (input_.eat('}')) return result; // empty object_t

            while (true)
            {
                // read key
                json key = [&]
                {
                    if (*input_ == '"') return read_string(); // quoted key (normal)
                    else if (has_option(json_parse_option::allow_unquoted_object_key))
                    {
                        json k(in_place_index::string);                           // make empty string(object key)
                        json::js_string& t = *k->as_string();                     // and get reference to it.
                        while (*input_ != EOF && *input_ > ' ' && *input_ != ':') // until delimiter found
                            t += static_cast<char_type>(*input_++);               // read a character as  object key
                        return k;
                    }
                    else throw bad_format("invalid object format: expected object key", *input_);
                }();

                eat_whitespaces();

                if (!input_.eat(':'))
                    throw bad_format("invalid object format: expected a ':'", *input_);

                eat_whitespaces();

                // read value
                json value = read_element();
                ret.insert_or_assign(std::move(*key->as_string()), std::move(value));

                eat_whitespaces();

                // read ',' or '}'
                if (input_.eat(','))
                {
                    eat_whitespaces();

                    // check '}'
                    if (input_.eat('}'))
                    {
                        if (has_option(json_parse_option::allow_trailing_comma)) break; // OK
                        else throw bad_format("invalid object format: expected an element (trailing comma not allowed)", *input_);
                    }

                    continue; // to next `key: value`
                }
                else if (input_.eat('}')) break;
                else throw bad_format("invalid object format: expected ',' or '}'", *input_);
            }

            return result;
        }

        // eats BOM sequence `EFBBBF` if allowed.
        void eat_utf8bom()
        {
            if (input_.eat(0xEF)) // if stream starts with 0xEF, regard it as beginning of UTF-8 BOM.
            {
                if (!has_option(json_parse_option::allow_utf8_bom)) throw bad_format("invalid json format: expected an element. (UTF-8 BOM not allowed)", *input_);
                if (!input_.eat(0xBB)) throw bad_format("invalid json format: UTF-8 BOM sequence expected... 0xBB", *input_);
                if (!input_.eat(0xBF)) throw bad_format("invalid json format: UTF-8 BOM sequence expected... 0xBF", *input_);
            }
        }

        // eats continuous white spaces and comments `/*...*/`, `//...\n`
        void eat_whitespaces()
        {
            // locale-independent is_space
            constexpr auto is_space = [](int_type i)-> bool { return i == ' ' || i == '\t' || i == '\r' || i == '\n'; };

            while (true)
            {
                while (is_space(*input_)) ++input_;

                if (has_option(json_parse_option::allow_comment) && input_.eat('/'))
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

        // makes a bad_format exception with error message
        [[nodiscard]] exceptions::bad_format bad_format(std::string_view reason, std::optional<int_type> but_encountered = std::nullopt) const
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
                    message << static_cast<char>(*but_encountered);
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

            return exceptions::bad_format{message.str()};
        }
    };


    template <class CharOutputIterator>
    struct json::json_writer
    {
    public:
        static void write_json(CharOutputIterator destination, const json& json, json_serialize_option option, json_floating_format_options format)
        {
            json_writer(destination, option, format).execute(json);
        }

    private:
        struct output_stream
        {
            CharOutputIterator it_;
            explicit output_stream(CharOutputIterator it) : it_(std::move(it)) {}
            output_stream& operator <<(char c) { return *it_++ = c, *this; }
            output_stream& operator <<(std::string_view view) { return std::copy(view.begin(), view.end(), it_), *this; }
        } output_;

        const json_serialize_option option_bits_{};
        const json_floating_format_options floating_format_{};
        std::basic_string<json::char_type> indent_stack_{};

        // ctor
        json_writer(CharOutputIterator& out, json_serialize_option option, json_floating_format_options format) : output_(out), option_bits_(option), floating_format_(format) {}

        // executes serializing
        void execute(const json& value)
        {
            write_element(value);
        }

        // gets the option bit enabled.
        [[nodiscard]] bool has_option(json_serialize_option bit) const noexcept
        {
            return (option_bits_ & bit) != json_serialize_option::none;
        }

        // string
        void write_string(const js_string_view val)
        {
            using namespace std::string_view_literals;

            output_ << '"';
            for (auto c : val)
            {
                static constexpr std::array<std::string_view, 256> char_table_
                {
                    /* 00 */"\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007", "\\b", "\\t", "\\n", "\\u000B", "\\u000C", "\\r", "\\u000E", "\\u000F",
                    /* 10 */"\\u0010", "\\u0011", "\\f", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001A", "\\u001B", "\\u001C", "\\u001D", "\\u001E", "\\u001F",
                    /* 20 */{}, {}, "\\\"", {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, "\\/",
                    /* 30 */{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
                    /* 40 */{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
                    /* 50 */{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, "\\\\", {}, {}, {},
                    /* 60 */{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
                    /* 70 */{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, "\\u007F",
                    /* 80 */{}, // ... continue to 0xFF
                };

                if (auto p = char_table_[static_cast<uint8_t>(c)]; !p.empty())
                    output_ << p;
                else
                    output_ << c;
            }
            output_ << '"';
        }

        // formats a integer `i` into `buffer` and returns it as string_view
        template <size_t N, class Integer, std::enable_if_t<std::is_integral_v<Integer>>* = nullptr>
        static std::string_view integer_to_chars(char (&buffer)[N], Integer i)
        {
            auto [ptr, ec] = std::to_chars(std::begin(buffer), std::end(buffer), i, 10);
            if (ec != std::errc{}) throw bad_value("failed to to_chars(integer)");
            return std::string_view(buffer, ptr - buffer);
        }

        // element
        void write_element(const json& value)
        {
            switch (value.value().get_type())
            {
            case json_type_index::undefined: return write_element(json::js_undefined{});
            case json_type_index::null: return write_element(*value.value().as_null());
            case json_type_index::boolean: return write_element(*value.value().as_bool());
            case json_type_index::integer: return write_element(*value.value().as_integer());
            case json_type_index::floating: return write_element(*value.value().as_floating());
            case json_type_index::string: return write_element(*value.value().as_string());
            case json_type_index::array: return write_element(*value.value().as_array());
            case json_type_index::object: return write_element(*value.value().as_object());
            }
        }

        // undefined
        void write_element(const js_undefined)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment)) output_ << "/***  UNDEFINED  ***/ undefined /* not allowed */"sv;
            else throw bad_value("undefined is not allowed");
        }

        // null
        void write_element(const js_null)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment)) output_ << "/***  NULL  ***/ ";
            output_ << "null"sv;
        }

        // true or false
        void write_element(const js_boolean v)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment)) output_ << "/***  BOOLEAN  ***/ "sv;
            output_ << (v ? "true"sv : "false"sv);
        }

        // integer
        void write_element(const js_integer v)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment)) output_ << "/***  INTEGER  ***/ "sv;
            char buf[64];
            output_ << integer_to_chars(buf, v);
        }

        // floating
        void write_element(const js_floating v)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment)) output_ << "/***  FLOATING  ***/ "sv;

            if (std::isnan(v))
            {
                if (has_option(json_serialize_option::debug_dump_type_as_comment)) output_ << "NaN /* not allowed */"sv;
                else throw bad_value("NaN is not allowed");
            }
            else if (std::isinf(v))
            {
                output_ << (v >= 0 ? "1.0e999999999"sv : "-1.0e999999999"sv);
            }
            else // normal
            {
                std::chars_format format = std::chars_format::general;
                const auto precision = std::clamp(floating_format_.floating_precision, 0, 64);
                const auto overflow_limit = std::pow(static_cast<json::js_floating>(10), precision);
                const auto underflow_limit = std::pow(static_cast<json::js_floating>(10), -precision);

                if (const auto abs = std::abs(v);
                    abs < overflow_limit && abs > underflow_limit)
                    format = floating_format_.floating_format;

#if (defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L) // compiler has floating-point to_chars
                char s[128]{};
                auto [ptr, ec] = std::to_chars(std::begin(s), std::end(s), v, format, precision);
                if (ec != std::errc{} || *ptr != '\0') throw bad_value("failed to to_chars(floating)");
                output_ << s; // json::js_string_view(s, ptr - s);
#else // use fallback implementation
                std::ostringstream s{};
                s.imbue(std::locale::classic());
                if (format == std::chars_format::fixed) s << std::fixed;
                if (format == std::chars_format::scientific) s << std::scientific;
                s << std::setprecision(precision);
                s << v;
                output_ << s.str();
#endif
            }
        }

        // string
        void write_element(const js_string& val)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment))
            {
                char buf[32];
                output_ << "/***  STRING["sv << integer_to_chars(buf, val.size()) << "]  ***/ "sv;
            }

            write_string(val);
        }

        // array
        void write_element(const js_array& val)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment))
            {
                char buf[32];
                output_ << "/***  ARRAY["sv << integer_to_chars(buf, val.size()) << "]  ***/ "sv;
            }

            if (!val.empty())
            {
                output_ << '[';
                if (has_option(json_serialize_option::pretty)) output_ << "\n"sv;
                indent_stack_.push_back(' ');
                indent_stack_.push_back(' ');
                for (auto it = val.begin(); it != val.end(); ++it)
                {
                    if (it != val.begin())
                    {
                        output_ << ',';
                        if (has_option(json_serialize_option::pretty)) output_ << '\n';
                    }
                    if (has_option(json_serialize_option::pretty)) output_ << indent_stack_;
                    write_element(*it);
                }
                indent_stack_.pop_back();
                indent_stack_.pop_back();
                if (has_option(json_serialize_option::pretty)) output_ << '\n' << indent_stack_;
                output_ << ']';
            }
            else
            {
                output_ << "[]"sv;
            }
        }

        // object
        void write_element(const js_object& val)
        {
            using namespace std::string_view_literals;
            if (has_option(json_serialize_option::debug_dump_type_as_comment))
            {
                char buf[32];
                output_ << "/***  OBJECT["sv << integer_to_chars(buf, val.size()) << "]  ***/ "sv;
            }

            if (!val.empty())
            {
                output_ << '{';
                if (has_option(json_serialize_option::pretty)) output_ << '\n';

                indent_stack_.push_back(' ');
                indent_stack_.push_back(' ');
                for (auto it = val.begin(); it != val.end(); ++it)
                {
                    if (it != val.begin())
                    {
                        output_ << ',';
                        if (has_option(json_serialize_option::pretty)) output_ << '\n';
                    }
                    if (has_option(json_serialize_option::pretty)) output_ << indent_stack_;
                    write_string(it->first);
                    output_ << ':';
                    if (has_option(json_serialize_option::pretty)) output_ << ' ';
                    write_element(it->second);
                }
                indent_stack_.pop_back();
                indent_stack_.pop_back();
                if (has_option(json_serialize_option::pretty)) output_ << '\n' << indent_stack_;
                output_ << '}';
            }
            else
            {
                output_ << "{}"sv;
            }
        }
    };

    inline namespace io
    {
        template <class CharInputIterator>
        static json parse_json(CharInputIterator begin, CharInputIterator end, json_parse_option loose = json_parse_option::default_option)
        {
            return json::json_reader<CharInputIterator>::read_json(std::move(begin), std::move(end), loose);
        }

        static json parse_json(json::json_string_view sv, json_parse_option loose = json_parse_option::default_option)
        {
            return io::parse_json<json::json_string_view::const_iterator>(sv.begin(), sv.end(), loose);
        }

        template <class CharOutputIterator>
        static void serialize_json(CharOutputIterator begin, const json& value, json_serialize_option option = json_serialize_option::none, json_floating_format_options floating_format = {})
        {
            return json::json_writer<CharOutputIterator>::write_json(std::move(begin), value, option, floating_format);
        }

        template <class DestinationContainer = json::json_string>
        static inline DestinationContainer serialize_json(const json& value, json_serialize_option option = json_serialize_option::none, json_floating_format_options floating_format = {})
        {
            DestinationContainer string;
            io::serialize_json<std::back_insert_iterator<DestinationContainer>>(std::back_insert_iterator(string), value, option, floating_format);
            return string;
        }
    }

    inline json json::parse(const json_string_view& source, json_parse_option opt) { return io::parse_json(source, opt); }

    inline json::json_string json::serialize(json_serialize_option opt, json_floating_format_options format) const { return io::serialize_json(*this, opt, format); }

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
    template <class T> struct json::json_ext<T, std::enable_if_t<std::is_pointer_v<T>>> : json_ext_ctor_helper::prevent_conversion { };                           // prevent `bool` constructor by pointer types
    template <class T> struct json::json_ext<T, std::enable_if_t<std::is_integral_v<T>>> : json_ext_ctor_helper::static_cast_to_value<js_integer> { };            // map all integral types (`char`, `int`, `unsigned long`, ...) to `int_t`: hint: this is also true for `bool`, but concrete constructor `json_t(bool)` is selected in the real calling situation
    template <class T> struct json::json_ext<T, std::enable_if_t<std::is_floating_point_v<T>>> : json_ext_ctor_helper::static_cast_to_value<js_floating> { };     // map all floating point types (`float`, `double`, ...) to `float_t` (`std::is_floating_point<T>`)
    template <> struct json::json_ext<const json::char_type*> : json_ext_ctor_helper::pass_to_value_ctor<js_string> { };                                          // map `const char*` to `string_t`
    template <> struct json::json_ext<std::basic_string_view<json::char_type, json::char_traits>> : json_ext_ctor_helper::pass_to_value_ctor<js_string> { };      // map `std::string_view` to `string_t`
    template <class A> struct json::json_ext<std::basic_string<json::char_type, json::char_traits, A>> : json_ext_ctor_helper::pass_to_value_ctor<js_string> { }; // map `std::string` to `string_t`

    // built-in ext ctor for array_t: map some STL container`<T>` to `array_t`, if `T` is convertible json_t.
    template <class U> struct json::json_ext<std::initializer_list<U>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { };                           // map `std::initializer_list<T>` to `array_t`
    template <class U, size_t S> struct json::json_ext<std::array<U, S>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { };                         // map `std::vector<T>` to `array_t`
    template <class U, class A> struct json::json_ext<std::vector<U, A>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { };                         // map `std::array<T, n>` to `array_t`
    template <class U, class A> struct json::json_ext<std::set<U, A>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { };                            // map `std::set<T>` to `array_t`
    template <class U, class A> struct json::json_ext<std::multiset<U, A>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { };                       // map `std::multiset<T>` to `array_t`
    template <class U, class P, class A> struct json::json_ext<std::unordered_set<U, P, A>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { };      // map `std::unordered_set<T>` to `array_t`
    template <class U, class P, class A> struct json::json_ext<std::unordered_multiset<U, P, A>, std::enable_if_t<std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_array> { }; // map `std::unordered_multiset<T>` to `array_t`

    template <class... U> struct json::json_ext<std::tuple<U...>, std::enable_if_t<std::conjunction_v<std::is_constructible<json, U>...>>> // map `std::tuple<T...>` to `array_t`
    {
        static js_array serialize(const std::tuple<U...>& val) { return std::apply([](auto&&... x) { return js_array{{json(std::forward<decltype(x)>(x))...}}; }, std::forward<decltype(val)>(val)); }
        static js_array serialize(std::tuple<U...>&& val) { return std::apply([](auto&&... x) { return js_array{{json(std::forward<decltype(x)>(x))...}}; }, std::forward<decltype(val)>(val)); }
    };

    // built-in ext ctor for object_t: map some STL container`<K, T>` to `object_t`, if `K` is convertible string and `T` is convertible json_t.
    template <class K, class U, class P, class A> struct json::json_ext<std::map<K, U, P, A>, std::enable_if_t<std::is_convertible_v<K, json::js_object_key_view> && std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_object> { };           // map `std::map<K, T>` to `object_t`
    template <class K, class U, class P, class A> struct json::json_ext<std::unordered_map<K, U, P, A>, std::enable_if_t<std::is_convertible_v<K, json::js_object_key_view> && std::is_constructible_v<json, U>>> : json_ext_ctor_helper::pass_iterator_pair_to_value_ctor<js_object> { }; // map `std::unordered_map<K, T>` to `object_t`

    // built-in ext ctor for to_json types: some types which has `to_json()` function to `json_t`

    // If `T` has a member function `json_t to_json() const;`:
    template <class T>
    struct json::json_ext<
            T,
            std::enable_if_t<
                std::is_same_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json>
            >>
    {
        static json serialize(const T& val)
        {
            return val.to_json();
        }
    };

    // else If `T` has the member function `R to_json() const;` where `R` is convertible to json_string_view:
    template <class T>
    struct json::json_ext<
            T,
            std::enable_if_t<
                !std::is_same_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json> &&
                std::is_convertible_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json::json_string_view>
            >>
    {
        static json serialize(const T& val)
        {
            return json::parse(val.to_json());
        }
    };

    // else If there is the global/ADL function `json_t to_json(T);`:
    template <class T>
    struct json::json_ext<
            T,
            std::enable_if_t<
                !std::is_same_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json> &&
                !std::is_convertible_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json::json_string_view> &&
                std::is_same_v<std::invoke_result_t<decltype(to_json(std::declval<T>())), const T&>, json>
            >>
    {
        static json serialize(const T& val)
        {
            return to_json(val);
        }
    };

    // else If there is the global/ADL function `R to_json(T);` where `R` is convertible to json_string_view:
    template <class T>
    struct json::json_ext<
            T,
            std::enable_if_t<
                !std::is_same_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json> &&
                !std::is_convertible_v<std::invoke_result_t<decltype(&T::to_json), const T&>, json::json_string_view> &&
                !std::is_same_v<std::invoke_result_t<decltype(to_json(std::declval<T>())), const T&>, json> &&
                std::is_convertible_v<std::invoke_result_t<decltype(to_json(std::declval<T>()))>, json::json_string_view>
            >>
    {
        static json serialize(const T& val)
        {
            return json::parse(to_json(val));
        }
    };

    // i/o stream operators
    namespace ios
    {
        template <class F>
        struct stream_manipulator : F
        {
            constexpr stream_manipulator(F&& f) : F(f) {}

            template <class Char, class Traits>
            inline friend auto operator >>(std::basic_istream<Char, Traits>& istream, const stream_manipulator& manipulator) noexcept(noexcept(manipulator(istream))) -> decltype(istream)
            {
                return (void)manipulator(istream), istream;
            }

            template <class Char, class Traits>
            inline friend auto operator <<(std::basic_ostream<Char, Traits>& ostream, const stream_manipulator& manipulator) noexcept(noexcept(manipulator(ostream))) -> decltype(ostream)
            {
                return (void)manipulator(ostream), ostream;
            }
        };

        // allocates json parse option word index
        [[nodiscard]] inline int json_istream_parse_option_index()
        {
            static int index = std::ios_base::xalloc();
            return index;
        }

        // allocates json serialize option word index
        [[nodiscard]] inline int json_ostream_serialize_option_index()
        {
            static int index = std::ios_base::xalloc();
            return index;
        }

        // makes json input manipulator
        [[nodiscard]] static inline constexpr auto json_ios_option(json_parse_option opt)
        {
            return stream_manipulator([opt](std::istream& os) { os.iword(json_istream_parse_option_index()) = static_cast<long>(opt); });
        }

        // istream& operator >>(istream&, json&)
        static inline auto operator >>(std::basic_istream<json::char_type>& istream, json& j) -> decltype(istream)
        {
            const auto opt = static_cast<json_parse_option>(istream.iword(json_istream_parse_option_index()));
            j = io::parse_json<std::istreambuf_iterator<json::char_type>>(istream, {}, opt);
            return istream;
        }

        // makes json output manipulator
        [[nodiscard]] static inline constexpr auto json_ios_option(json_serialize_option opt)
        {
            return stream_manipulator([opt](std::ostream& os) { os.iword(json_ostream_serialize_option_index()) = static_cast<long>(opt); });
        }

        // gets json floating format from stream
        [[nodiscard]] static json_floating_format_options json_floating_format_options_from_stream(const std::ios_base& stream)
        {
            json_floating_format_options r{};
            const auto mode = stream.flags() & std::ios_base::floatfield;
            if (mode == std::ios_base::fixed) r.floating_format = std::chars_format::fixed;
            if (mode == std::ios_base::scientific) r.floating_format = std::chars_format::scientific;
            r.floating_precision = std::clamp(static_cast<int>(stream.precision()), 0, 64);
            return r;
        }

        // ostream& operator <<(ostream&, const json&)
        inline auto operator <<(std::basic_ostream<json::char_type>& ostream, const json& j) -> decltype(ostream)
        {
            // opt
            const auto opt = static_cast<json_serialize_option>(ostream.iword(json_ostream_serialize_option_index()));
            const auto fmt = json_floating_format_options_from_stream(ostream);
            io::serialize_json<std::ostreambuf_iterator<json::char_type>>(ostream, j, opt, fmt);
            return ostream;
        }
    }

    // json stream operators and manipulators
    // usage: `std::cin >> json_set_option(json_parse_option::default) << json;`
    // usage: `std::cout << json_set_option(json_serialize_option::pretty) << json;`
    using ios::json_ios_option;
    using ios::operator <<;
    using ios::operator >>;
    static constexpr auto json_in_strict = json_ios_option(json_parse_option::none);
    static constexpr auto json_in_default = json_ios_option(json_parse_option::default_option);
    static constexpr auto json_in_loose = json_ios_option(json_parse_option::all);
    static constexpr auto json_out_pretty = json_ios_option(json_serialize_option::pretty);

    // type exports

    using js_undefined = json::js_undefined;
    using js_null = json::js_null;
    using js_boolean = json::js_boolean;
    using js_integer = json::js_integer;
    using js_floating = json::js_floating;
    using js_number = json::js_number;
    using js_string = json::js_string;
    using js_array = json::js_array;
    using js_object = json::js_object;

    using json_string = json::json_string;
}
#endif
