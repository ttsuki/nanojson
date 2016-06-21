# nanojson - Simple json reader/writer

:star2: Single C++ header only.

:red_circle: Caution! this library allows:
- last comma of array: [1,2,3,]
- last comma of object: { "a":1, "b":2, }
- non-quoated object-key: { a:1, b:2 }
- block/line comment: [ /* value of hoge */ 1, ] // <- array of fuga
  - for the purpose of parsing json-like configuration file with help comment.

# License
MIT

# Sample Code
~~~cpp
#include "nanojson.h"
#include <iostream>

int main()
{
	#define LF "\n"
	std::stringstream ss = std::stringstream(std::string(
		"[123, {"                                    LF
		"a : \"あいう\\\\n\\tえお\", "                 LF
		"  \t b : true, "                            LF
		"     c : null, "                            LF
		" /* start comment ..."                      LF
		"     x : here is in block comment"          LF
		"                  ... end comment */"       LF
		"\"d\\u0001\" : false, "                     LF
		"   // e is a test integer."                 LF
		"   // f is a test floating."                LF
		"e : 1234567890123456789, "                  LF
		"f : -123.4567e+89, "	                     LF
		"}  ]"));

	std::cout << "input json: " << std::endl;
	std::cout << ss.str() << std::endl;

	std::cout << "parsed json:" << std::endl;
	nanojson::element e = nanojson::element::from_string(ss.str());
	std::cout << e.to_json_string() << std::endl;

	std::cout << "values:" << std::endl;
	if (e[1]["a"].is_defined()) { std::cout << "e[1][\"a\"] = " << e[1]["a"].to_string() << std::endl; }
	if (e[1]["f"].is_defined()) { std::cout << "e[1][\"f\"] = " << e[1]["f"].to_string() << std::endl; }
	if (e[1]["x"].is_defined()) { std::cout << "e[1][\"x\"] = " << e[1]["x"].to_string() << std::endl; }
	
	std::cout << "input test json:" << std::endl;
	try {
		std::cin >> e;
	}
	catch (nanojson::nanojson_exception &)
	{
		std::cout << "il-formed json data." << std::endl;
		e = nanojson::element::undefined();
	}
	std::cout << "parsed json:" << std::endl;
	std::cout << e << std::endl;

	return 0;
}
~~~
