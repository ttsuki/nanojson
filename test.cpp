/**
* nanojson test code
*
* Copyright (c) 2016-2022 ttsuki
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "nanojson.h"
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); // set codepage to utf-8.
#endif

	#define LF "\n"
	std::stringstream ss = std::stringstream(std::string(
		u8"[123, {"                                    LF
		u8"a : \"$£ह€한𐍈\\u0024\\u00A3\\u0939\\u20AC\\uD55C\\uD800\\uDF48\\uD83D\\uDE03\", "                 LF
		u8"  \t b : true, "                            LF
		u8"     c : null, "                            LF
		u8" /* start comment ..."                      LF
		u8"     x : here is in block comment"          LF
		u8"                  ... end comment */"       LF
		u8"\"d\\u0001\" : false, "                     LF
		u8"   // e is a test integer."                 LF
		u8"   // f is a test floating."                LF
		u8"e : 1234567890123456789, "                  LF
		u8"f : -123.4567e+89, "	                     LF
		u8"}  ]"));

	std::cout << "input json: " << std::endl;
	std::cout << ss.str() << std::endl;

	std::cout << "parsed json:" << std::endl;
	nanojson::element e = nanojson::element::from_string(ss.str());
	std::cout << e.to_json_string() << std::endl;

	std::cout << "values:" << std::endl;
	if (e[1]["a"].is_defined()) { std::cout << "e[1][\"a\"] = " << e[1]["a"].to_string() << std::endl; }
	if (e[1]["f"].is_defined()) { std::cout << "e[1][\"f\"] = " << e[1]["f"].to_string() << std::endl; }
	if (e[1]["x"].is_defined()) { std::cout << "e[1][\"x\"] = " << e[1]["x"].to_string() << std::endl; }
	
	std::cout << "e.size() = " << e.size() << std::endl;
	if (e[1].is_defined()) { std::cout << "e[1].size() = " << e[1].size() << std::endl; } 
	if (e[1]["a"].is_defined()) { std::cout << "e[1][\"a\"].size() = " << e[1]["a"].size() << std::endl; } 

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
