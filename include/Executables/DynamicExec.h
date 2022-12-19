#pragma once

#ifndef H_DYNAMICEXEC
#define H_DYNAMICEXEC

#include <Interface.h>
#include <Log.h>

namespace Dynamic_Exec
{
#define Dynamic_Exec_ std::string("Dynamic_Exec")
#define Dynamic_Exec_Exec std::string("Dynamic_Exec_StageExec")
#define Dynamic_Exec_Timeout std::string("Dynamic_Exec_Timeout")

	/// <summary>
	/// Function Handling the execution of GenProg
	/// </summary>
	/// <param name="inter"></param>
	/// <param name="data"></param>
	/// <returns></returns>
	std::pair<bool, std::string> Dynamic_Exec(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		if (stage->executable == "")
		{
			logger::error("Compile", "Compile", "executable is empty");
			return { false, "" };
		}
		inter->Exec(stage->name, std::vector<std::string>{});
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
		inter->Stage_Create(Dynamic_Exec_ + std::to_string(stageid));
		inter->Stage_SetRunner(Dynamic_Exec_ + std::to_string(stageid), Dynamic_Exec);
		inter->Stage_SetExecutable(Dynamic_Exec_ + std::to_string(stageid), inter->FindFile(Dynamic_Exec_Exec + std::to_string(stageid)));
		int timeout = 0;
		std::string tmp = inter->GetConfArg(Dynamic_Exec_Timeout + std::to_string(stageid));
		if (tmp != "")
		{
			try {
				timeout = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("Dynamic_Exec", "SetTargets", "Cannot get timeout for stage " + Dynamic_Exec_ + std::to_string(stageid));
			}
			catch (std::invalid_argument&) {
				logger::warn("Dynamic_Exec", "SetTargets", "Cannot get timeout for stage " + Dynamic_Exec_ + std::to_string(stageid));
			}
		}
		inter->Stage_SetTimeout(Dynamic_Exec_ + std::to_string(stageid), timeout);
	}
}

#endif