/**
* nanojson - simple json library
*
* Copyright (c) 2016 tu-sa
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

// ** memo **
// this library allows:
// - last comma of array: [1,2,3,]
// - last comma of object: { "a":1, "b":2, }
// - no-quoated object-key: { a:1, b:2 }
// - block/line comment: [ /* value of hoge */ 1, ] // <- array of fuga

#pragma once
#ifndef NANOJSON_H_INCLUDED
#define NANOJSON_H_INCLUDED

#include <vector>
#include <map>
#include <string>

#include <cerrno>
#include <cinttypes>
#include <cstdarg>
#include <cassert>

#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>

// remove explicit from element::ctor()
#ifndef NANOJSON_NOEXPLICIT
#	define NANOJSON_EXPLICIT explicit
#else
#	define NANOJSON_EXPLICIT
#endif

// return undefined instead of throwing exception
#ifndef NANOJSON_NOEXCEPTION
#	define NANOJSON_THROW(exception, ...)	throw exception
#else
#	define NANOJSON_THROW(exception, ...)	return __VA_ARGS__
#endif

namespace nanojson
{

	struct nanojson_exception : public std::exception {};
	struct bad_cast : public nanojson_exception {}; ///< bad-cast
	struct bad_format : public nanojson_exception {}; ///< bad format json data
	struct bad_operation : public nanojson_exception {}; ///< thrown if

	struct element_type
	{
		enum type
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
	};


	struct element
	{
		typedef bool boolean_t;
		typedef std::intmax_t integer_t;
		typedef long double floating_t;
		typedef std::string string_t;
		typedef std::vector<element> array_t;
		typedef std::map<string_t, element> object_t;
		typedef string_t::value_type char_t;

	private:
		element_type::type type;
		struct value_t
		{
			boolean_t as_boolean;
			integer_t as_integer;
			floating_t as_floating;
			string_t as_string;
			array_t as_array;
			object_t as_object;
		} value;
	public:

		element() : type(element_type::null), value() { }
		element(const element &value) : type(value.type), value(value.value) { }
		NANOJSON_EXPLICIT element(const boolean_t &value) : type(element_type::boolean), value() { this->value.as_boolean = value; }
		NANOJSON_EXPLICIT element(const floating_t &value) : type(element_type::floating), value() { this->value.as_floating = value; }
		NANOJSON_EXPLICIT element(const integer_t &value) : type(element_type::integer), value() { this->value.as_integer = value; }
		NANOJSON_EXPLICIT element(const string_t &value) : type(element_type::string), value() { this->value.as_string = value; }
		NANOJSON_EXPLICIT element(const array_t &value) : type(element_type::array), value() { this->value.as_array = value; }
		NANOJSON_EXPLICIT element(const object_t &value) : type(element_type::object), value() { this->value.as_object = value; }


		element_type::type get_type() const { return type; }
		bool is_undefined() const { return get_type() == element_type::undefined; }
		bool is_defined() const { return !is_undefined(); }
		bool is_null() const { return get_type() == element_type::null; }
		bool is_boolean() const { return get_type() == element_type::boolean; }
		bool is_integer() const { return get_type() == element_type::integer; }
		bool is_floating() const { return get_type() == element_type::floating; }
		bool is_number() const { return is_integer() || is_floating(); }
		bool is_string() const { return get_type() == element_type::string; }
		bool is_array() const { return get_type() == element_type::array; }
		bool is_object() const { return get_type() == element_type::object; }

		boolean_t as_boolean() const { return as_boolean_ref(); }
		boolean_t &as_boolean_ref() { return const_cast<boolean_t&>(const_cast<const element&>(*this).as_boolean_ref()); }
		const boolean_t &as_boolean_ref() const { if (is_boolean()) { return value.as_boolean; } NANOJSON_THROW(bad_cast(), value.as_boolean); }

		integer_t as_integer() const { return as_integer_ref(); }
		integer_t &as_integer_ref() { return const_cast<integer_t&>(const_cast<const element&>(*this).as_integer_ref()); }
		const integer_t &as_integer_ref() const { if (is_integer()) { return value.as_integer; } NANOJSON_THROW(bad_cast(), value.as_integer); }

		floating_t as_floating() const { return as_floating_ref(); }
		floating_t &as_floating_ref() { return const_cast<floating_t&>(const_cast<const element&>(*this).as_floating_ref()); }
		const floating_t &as_floating_ref() const { if (is_floating()) { return value.as_floating; } NANOJSON_THROW(bad_cast(), value.as_floating); }

		string_t as_string() const { return as_string_ref(); }
		string_t &as_string_ref() { return const_cast<string_t&>(const_cast<const element&>(*this).as_string_ref()); }
		const string_t &as_string_ref() const { if (is_string()) { return value.as_string; } NANOJSON_THROW(bad_cast(), value.as_string); }

		array_t as_array() const { return as_array_ref(); }
		array_t &as_array_ref() { return const_cast<array_t&>(const_cast<const element&>(*this).as_array_ref()); }
		const array_t &as_array_ref() const { if (is_array()) { return value.as_array; } NANOJSON_THROW(bad_cast(), value.as_array); }

		object_t as_object() const { return as_object_ref(); }
		object_t &as_object_ref() { return const_cast<object_t&>(const_cast<const element&>(*this).as_object_ref()); }
		const object_t &as_object_ref() const { if (is_object()) { return value.as_object; } NANOJSON_THROW(bad_cast(), value.as_object); }


		/// convert to boolean_t
		boolean_t to_boolean() const
		{
			if (is_undefined()) { return false; }
			if (is_null()) { return false; }
			if (is_boolean()) { return value.as_boolean; }
			if (is_floating()) { return value.as_floating != 0; }
			if (is_integer()) { return value.as_integer != 0; }
			if (is_string()) { return value.as_string.size() > 0; }
			if (is_array()) { return true; }
			if (is_object()) { return true; }
			return false;
		}

		/// convert to integer_t
		integer_t to_integer() const
		{
			if (is_null()) { return 0; }
			if (is_integer()) { return as_integer(); }
			if (is_floating()) { return static_cast<integer_t>(as_floating()); }
			NANOJSON_THROW(bad_cast(), 0);
		}

		/// convert to floating_t
		floating_t to_floating() const
		{
			if (is_null()) { return 0; }
			if (is_integer()) { return static_cast<floating_t>(as_integer()); }
			if (is_floating()) { return as_floating(); }
			NANOJSON_THROW(bad_cast(), 0);
		}

		/// convert to string_t
		string_t to_string() const
		{
			if (is_string()) return as_string();
			return to_json_string();
		}

		/// export to json data
		string_t to_json_string(bool one_liner = true, bool no_spaces = false) const
		{
			return element_writer::serialize(*this, one_liner, no_spaces, 0);
		}

		/// cast operators
		operator boolean_t() const { return to_boolean(); }
		operator integer_t() const { return to_integer(); }
		operator floating_t() const { return to_floating(); }
		operator string_t() const { return to_string(); }
		operator array_t() const { return as_array(); }
		operator object_t() const { return as_object(); }

		/// compare operators
		bool operator <(const element &b) const { return compare_to(b) < 0; }
		bool operator >(const element &b) const { return compare_to(b) > 0; }
		bool operator <=(const element &b) const { return compare_to(b) <= 0; }
		bool operator >=(const element &b) const { return compare_to(b) >= 0; }
		bool operator ==(const element &b) const { return equals_to(b); }
		bool operator !=(const element &b) const { return !equals_to(b); }

		/// index operators for array or object
		const element & operator [] (size_t index) const
		{
			if (is_null()) { return undefined(); }
			if (is_undefined()) { return undefined(); }
			if (!is_array()) { NANOJSON_THROW(bad_operation(), undefined()); }
			if (index >= value.as_array.size()) { return undefined(); }
			return value.as_array.at(index);
		}

		const element & operator [] (const string_t &key) const
		{
			if (is_null()) { return undefined(); }
			if (is_undefined()) { return undefined(); }
			if (!is_object()) { NANOJSON_THROW(bad_operation(), undefined()); }
			if (value.as_object.count(key) == 0) { return undefined(); }
			return value.as_object.at(key);
		}

		const element & operator [] (const char_t *pKey) const
		{
			return operator[](string_t(pKey));
		}

		size_t size() const
		{
			if (is_undefined()) { return 0; }
			if (is_null()) { return 0; }
			if (is_array()) { return value.as_array.size(); }
			if (is_object()) { return value.as_object.size(); }
			NANOJSON_THROW(bad_operation(), 0);
		}

		/// undefined
		static const element & undefined()
		{
			struct
			{
				element operator ()() const
				{
					element ret;
					ret.type = element_type::undefined;
					return ret;
				}
			} make_undefined;
			const static element undef = make_undefined();
			return undef;
		}

		static element from_string(const string_t &src)
		{
			return from_iterator(src.cbegin(), src.cend());
		}

		static element from_string(const char_t *src)
		{
			return from_iterator(src, src + std::char_traits<char_t>::length(src));
		}

		template <class T>
		static element from_stream(T &istream)
		{
			return element_reader<T>::parse(istream);
		}

		template <class T>
		static element from_iterator(T first, T last)
		{
			struct reader
			{
				const T first, last;
				T cur;
				reader(T first, T last) : first(first), cur(first), last(last) { }
				int get() { return cur != last ? (int)*cur++ : EOF; }
				void unget() { if (cur != first) { cur--; } }
			};
			return element_reader<reader>::parse(reader(first, last));
		}

	private:
		template <class T>
		static int compare(const T &a, const T &b)
		{
			return a < b ? -1 : b < a ? 1 : 0;
		}

		int compare_to(const element &other) const
		{
			if (is_boolean() && other.is_boolean()) { return compare(as_boolean() ? 1 : 0, other.as_boolean() ? 1 : 0); }
			if (is_integer() && other.is_integer()) { return compare(as_integer(), other.as_integer()); }
			if (is_number() && other.is_number()) { return compare(to_floating(), other.to_floating()); }
			if (is_string() && other.is_string()) { return compare(as_string(), other.as_string()); }
			return compare(to_json_string(), other.to_json_string());
		}

		bool equals_to(const element &other) const
		{
			if (type != other.type)
			{
				// (int vs float) or (float vs int)
				if (is_number() && other.is_number())
				{
					return to_floating() == other.to_floating();
				}
				else
				{
					return false;
				}
			}

			// assume(type == other.type);
			if (is_undefined()) return true;
			if (is_null()) return true;
			if (is_boolean()) return as_boolean() == other.as_boolean();
			if (is_floating()) return as_floating() == other.as_floating();
			if (is_integer()) return as_integer() == other.as_integer();
			if (is_string()) return as_string() == other.as_string_ref();
			if (is_array()) return as_array_ref() == other.as_array_ref();
			if (is_object()) return as_object_ref() == other.as_object_ref();

			// not reachable.
			assert(false);
			return false;
		}

	private:
		class element_writer
		{
		public:

			static string_t serialize(const element &val, bool oneline, bool no_space, int indent = 0)
			{
				return to_string(val, oneline, no_space, indent);
			}

		private:
			static string_t to_string(const element &val, bool oneline, bool no_space, int indent)
			{
				oneline |= no_space;

				if (val.is_undefined()) { return "undefined"; }
				if (val.is_null()) { return "null"; }
				if (val.is_boolean()) { return val.as_boolean() ? "true" : "false"; }
				if (val.is_integer()) { std::stringstream ss; ss << val.as_integer(); return ss.str(); }
				if (val.is_floating())
				{
					floating_t f = val.as_floating();
					if (std::isinf(f))
					{
						f = f > 0 ? std::numeric_limits<floating_t>::max()
							: std::numeric_limits<floating_t>::min();
					}
					std::stringstream ss;
					ss << std::setprecision(std::numeric_limits<floating_t>::max_digits10) << f;
					return ss.str();
				}

				if (val.is_string())
				{
					return string_t('"' + encode_string(val.as_string_ref()) + '"');
				}

				if (val.is_array())
				{
					if (val.size() == 0) { return "[]"; }
					string_t ret;
					ret += '[';
					if (oneline && !no_space) { ret += ' '; }
					if (!oneline) { ret += '\n'; }
					const array_t &a = val.as_array_ref();
					for (array_t::const_iterator it = a.begin(), next = a.begin(); it != a.end(); it++)
					{
						if (!oneline) { ret += string_t(indent + 1, '\t'); }
						ret += to_string(*it, oneline, no_space, indent + 1);
						if (++next != a.end()) { ret += ','; }
						if (oneline && !no_space) { ret += ' '; }
						if (!oneline) { ret += '\n'; }
					}
					if (!oneline) { ret += string_t(indent, '\t'); }
					ret += ']';
					return ret;
				}

				if (val.is_object())
				{
					if (val.size() == 0) { return "{}"; }
					string_t ret;
					ret += '{';
					if (oneline && !no_space) { ret += ' '; }
					if (!oneline) { ret += '\n'; }
					const object_t &o = val.as_object_ref();
					for (object_t::const_iterator it = o.begin(), next = o.begin(); it != o.end(); it++)
					{
						if (!oneline) { ret += string_t(indent + 1, '\t'); }
						ret += '"';
						ret += encode_string(it->first);
						ret += '"';
						if (!no_space) { ret += ' '; }
						ret += ':';
						if (!no_space) { ret += ' '; }
						ret += to_string(it->second, oneline, no_space, indent + 1);
						if (++next != o.end()) { ret += ','; }
						if (oneline && !no_space) { ret += ' '; }
						if (!oneline) { ret += '\n'; }
					}
					if (!oneline) { ret += string_t(indent, '\t'); }
					ret += '}';
					return ret;
				}

				// not reachable.
				assert(false);
				return "";
			}

			static string_t encode_string(const string_t &src)
			{
				string_t encoded;
				for (size_t i = 0; i < src.size();i++)
				{
					switch (src[i])
					{
					case '\n': encoded += "\\n"; break;
					case '\t': encoded += "\\t"; break;
					case '\b': encoded += "\\b"; break;
					case '\f': encoded += "\\f"; break;
					case '\r': encoded += "\\r"; break;
					case '\\': encoded += "\\\\"; break;
					case '"': encoded += "\\\""; break;
					default:
						if ((src[i] & 0xFF) < 0x20)
						{
							const size_t bufsz = 8;
							char_t buf[bufsz] = {};
							snprintf(buf, bufsz, "\\u%04X", static_cast<int>(src[i]) & 0xFF);
							encoded += buf;
						}
						else
						{
							encoded += src[i];
						}
					}
				}
				return encoded;
			}

			template <class TChar>
			static void snprintf(TChar *buffer, size_t bufferCount, char *format, ...);

			template <>
			static void snprintf<char>(char *buffer, size_t bufferCount, char *format, ...)
			{
				va_list ap;
				va_start(ap, format);
				std::vsnprintf(buffer, bufferCount, format, ap);
				va_end(ap);
			}
		};

	private:
		template <class istream>
		class element_reader
		{
		public:
			static element parse(istream &src)
			{
				element_reader parser(src);
				return parser.execute();
			}

		private:
			element result;
			istream &src;
			unsigned char c;
			bool eof;
			unsigned char next()
			{
				int c = src.get();
				if (c == EOF) { eof = true; }
				return this->c = c;
			}

			element_reader(istream &src)
				: src(src)
				, eof(false)
			{
			}

			element execute()
			{
				next();
				skip_whitespaces();
				return read_element();
			}

			element read_element()
			{
				switch (c)
				{
				case 'N':
				case 'n':
					if (next() != 'u') { break; }
					if (next() != 'l') { break; }
					if (next() != 'l') { break; }
					next();
					return element();

				case 'T':
				case 't':
					if (next() != 'r') { break; }
					if (next() != 'u') { break; }
					if (next() != 'e') { break; }
					next();
					return element(true);

				case 'F':
				case 'f':
					if (next() != 'a') { break; }
					if (next() != 'l') { break; }
					if (next() != 's') { break; }
					if (next() != 'e') { break; }
					next();
					return element(false);

				case '+': case '.':
				case '-':
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
				case '8': case '9':
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
				NANOJSON_THROW(bad_format(), undefined());
			}

			element read_number()
			{
				char_t buffer[32] = { '0' };
				int count = 0;
				int exp = 0;
				bool integer_type = true;

				if (c == '-' || c == '+') { buffer[count++] = c; next(); }
				if (c == '0') { buffer[count++] = '0'; }
				while (c == '0') { next(); };
				while (isdigit(c))
				{
					if (count < 21) { buffer[count++] = c; }
					else { integer_type = false; exp++; }
					next();
				}

				if (c == '.')
				{
					next();
					integer_type = false;
					buffer[count++] = '.';
					if (isdigit(c)) { buffer[count++] = c; next(); }
					else { NANOJSON_THROW(bad_format(), undefined()); }
					while (isdigit(c)) { if (count < 24) { buffer[count++] = c; } next(); }
				}

				if (c == 'e' || c == 'E')
				{
					next();
					integer_type = false;
					buffer[count++] = 'e';
					if (c == '-' || c == '+' || isdigit(c)) { buffer[count++] = c; next(); }
					else { NANOJSON_THROW(bad_format(), undefined()); }
					while (isdigit(c)) { if (count < 31) { buffer[count++] = c; } next(); }
				}

				if (integer_type)
				{
					char_t *retp = nullptr;
					errno = 0;
					intmax_t ret = std::strtoimax(buffer, &retp, 10);
					if (errno == 0 && retp == buffer + count
						&& ret >= std::numeric_limits<integer_t>::min()
						&& ret <= std::numeric_limits<integer_t>::max()
						)
					{
						return element(static_cast<integer_t>(ret));
					}
				}

				{
					char_t *retp = nullptr;
					errno = 0;
					long double ret = std::strtold(buffer, &retp);
					if ((errno == 0 || errno == ERANGE) && retp == buffer + count)
					{
						return element(static_cast<floating_t>(ret));
					}
				}

				NANOJSON_THROW(bad_format(), undefined());
			}

			element read_string()
			{
				// local function
				struct
				{
					int operator () (char c) const
					{
						if (c >= '0' && c <= '9') { return c - '0'; }
						if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
						if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
						NANOJSON_THROW(bad_format(), -1);
					}
				} hex;

				unsigned char quote = c; next();
				string_t ret;
				while (true)
				{
					if (c == '\\')
					{
						next();
						switch (c)
						{
						case 'n': ret += '\n'; break;
						case 't': ret += '\t'; break;
						case 'b': ret += '\b'; break;
						case 'f': ret += '\f'; break;
						case 'r': ret += '\r'; break;
						case '\\': ret += '\\'; break;
						case '"': ret += '"'; break;
						case 'u':
						{
							int code = 0;
							next(); code = code << 4 | hex(c);
							next(); code = code << 4 | hex(c);
							next(); code = code << 4 | hex(c);
							next(); code = code << 4 | hex(c);
							if (code < 0) { NANOJSON_THROW(bad_format(), undefined()); }
							else if (code <= 0x7F) { ret += (char)code; }
							else if (code <= 0x07FF) { ret += (char)(code >> 6 & 0x1f | 0xC0); ret += (char)(code & 0x3f | 0x80); }
							else if (code <= 0xFFFF) { ret += (char)(code >> 11 & 0x0f | 0xE0); ret += (char)(code >> 6 & 0x3f | 0x80); ret += (char)(code & 0x3f | 0x80); }
							else { NANOJSON_THROW(bad_format(), undefined()); }
							break;
						}
						default:
							NANOJSON_THROW(bad_format(), undefined());
							break;
						}
					}
					else if (c == quote) { next(); break; }
					else if (eof) { NANOJSON_THROW(bad_format(), undefined()); }
					else if (c < 0x20 || c == 0x7F) { NANOJSON_THROW(bad_format(), undefined()); }
					else { ret += c; }
					next();
				}

				return element(ret);
			}

			element read_array()
			{
				next();
				array_t ret;
				while (true)
				{
					skip_whitespaces();
					if (c == ']') { next(); break; }
					if (eof) { NANOJSON_THROW(bad_format(), undefined()); }
					element e = read_element();
					if (e.is_undefined()) { return undefined(); }
					ret.push_back(e);
					skip_whitespaces();
					if (c == ',') { next(); }
					else if (c == ']') {}
					else { NANOJSON_THROW(bad_format(), undefined()); }
				}
				return element(ret);
			}

			element read_object()
			{
				next();
				object_t ret;
				while (true)
				{
					skip_whitespaces();
					if (c == '}') { next(); break; }
					if (eof) { NANOJSON_THROW(bad_format(), undefined()); }
					string_t key;
					if (c == '"' || c == '\'')
					{
						element key_string = read_string();
						if (!key_string.is_string()) { return undefined(); }
						key = key_string.as_string();
					}
					else
					{
						while (c > ' ' && c != ':')
						{
							key += c;
							next();
						}
					}
					skip_whitespaces();
					if (c == ':') { next(); }
					else { NANOJSON_THROW(bad_format(), undefined()); }
					skip_whitespaces();
					element e = read_element();
					if (e.is_undefined()) { return undefined(); }
					ret[key] = e;
					skip_whitespaces();
					if (c == ',') { next(); }
					else if (c == '}') {}
					else { NANOJSON_THROW(bad_format(), undefined()); }
				}
				return element(ret);
			}

			void skip_whitespaces()
			{
				bool in_block_comment = false;
				bool in_line_comment = false;
				while (!eof)
				{
					if (in_block_comment)
					{
						if (c == '*')
						{
							if (next() == '/') { in_block_comment = false; }
						}
					}
					else if (in_line_comment)
					{
						if (c == '\n') { in_line_comment = false; }
					}
					else
					{
						if (c <= ' ') {}
						else if (c == '/')
						{
							switch (next())
							{
							case '*': in_block_comment = true; break;
							case '/': in_line_comment = true; break;
							default: NANOJSON_THROW(bad_format(), );
							}
						}
						else { break; }
					}
					next();
				}
			}
		};
	};

	template <class Tostream>
	Tostream & operator << (Tostream &ostream, const element &e)
	{
		return ostream << e.to_json_string();
	}

	template <class Tistream>
	Tistream & operator >> (Tistream &istream, element &e)
	{
		return e = element::from_stream(istream), istream;
	}

} // namespace nanojson

#endif

