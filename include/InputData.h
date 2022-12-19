#pragma once

#ifndef H_INPUTDATA
#define H_INPUTDATA

#include <vector>
#include <string>

namespace Bridge
{
	struct InputData
	{
	public:
		/// <summary>
		/// stores PUT inputs
		/// </summary>
		std::vector<std::string> inputs;
		/// <summary>
		/// stores PUT outputs
		/// </summary>
		std::vector<std::string> outputs;
		/// <summary>
		/// stores whether the inputs are passing or failing;
		/// [true]		-> passing;
		/// [false]		-> failing
		/// </summary>
		std::vector<bool> labels;
	};
}

#endif