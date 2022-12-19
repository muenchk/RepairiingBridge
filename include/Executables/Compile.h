#pragma once

#ifndef H_COMPILE
#define H_COMPILE

#include <Interface.h>
#include <Log.h>

namespace Compile
{
#define Compile_ "Compile"

	/// <summary>
	/// Function Handling the execution of GenProg
	/// </summary>
	/// <param name="inter"></param>
	/// <param name="data"></param>
	/// <returns></returns>
	std::pair<bool, std::string> Compile(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		if (stage->executable == "")
		{
			logger::error("Compile", "Compile", "executable is empty");
			return { false, "" };
		}
		inter->Exec(Compile_, std::vector<std::string>{});
		logger::info("Compile", "Compile", "executables compiled");
		return { true, "" };
	}


	/// <summary>
	/// Sets functions and values in Interface
	/// </summary>
	void SetTargets(int stageid)
	{
		// set generation of PUT output to true
		auto inter = Bridge::Interface::GetSingleton();
		inter->Stage_Create(Compile_);
		inter->Stage_SetRunner(Compile_, Compile);
		inter->Stage_SetExecutable(Compile_, inter->FindFile(PUTCompileSH));
		inter->Stage_SetTimeout(Compile_, 0);
	}
}

#endif