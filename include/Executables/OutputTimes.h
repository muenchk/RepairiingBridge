#pragma once

#ifndef H_OUTPUTTIMES
#define H_OUTPUTTIMES

#include "Log.h"
#include "Interface.h"

namespace OutputTimes
{
#define OutputTimes_ std::string("OutputTimings")

	void DeleteFolders(Bridge::Interface* inter)
	{
		/*std::filesystem::path output = inter->FindFile(OUTPUTDIR);
		std::filesystem::path outout = output / "times";
		try
		{
			std::filesystem::remove_all(outout);
		}
		catch (std::filesystem::filesystem_error&)
		{
		}*/
	}

	std::pair<bool, std::string> OutputTimes(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		std::filesystem::path outout = Bridge::Interface::GetSingleton()->pathconf / std::filesystem::path("runtimes");

		// create output folder
		try
		{
			std::filesystem::create_directories(outout);
		}
		catch (std::filesystem::filesystem_error& e)
		{
			logger::error("OutputTimes", "OutputTimes", "cannot create outputdir/times. error: " + (std::string)e.what());
			logger::error("OutputTimes", "OutputTimes", "Aborting stage. Continuing to next stage.");
			DeleteFolders(inter);
			return { true, "" };
		}
		//<Module, executable, finished(not aborted), majoriteratio, minoriteraton, start, end, duration in nanoseconds, execcommand>

		// open files.csv
		std::ofstream out(outout / ("times_" + logger::GetTimeString() + ".csv"));
		if (out.is_open())
		{
			// write header
			out << "Module" << ","
				<< "executed function/call" << ","
				<< "successfully finished" << ","
				<< "framework iteration" << ","
				<< "stage iteration" << ","
				<< "begin of execution" << ","
				<< "end of execution" << ","
				<< "duration in ns" << ","
				<< "command executed" << "\n";

			auto times = inter->GetRuntimes();
			for (int i = 0; i < times.size(); i++)
			{
				out << std::get<0>(times[i]) << "," 
					<< std::get<1>(times[i]) << "," 
					<< std::get<2>(times[i]) << "," 
					<< std::get<3>(times[i]) << "," 
					<< std::get<4>(times[i]) << "," 
					<< (std::get<5>(times[i])).time_since_epoch().count() << ","
					<< (std::get<6>(times[i])).time_since_epoch().count() << ","
					<< std::get<7>(times[i]) << "," 
					<< std::get<8>(times[i]) << "\n";
			}
			out.close();
		}

		return { true, "" };
	}


	/// <summary>
	/// Registers the stage in Interface
	/// </summary>
	void SetTargets(int stageid)
	{
		auto inter = Bridge::Interface::GetSingleton();
		inter->Stage_Create(OutputTimes_);
		inter->Stage_SetRunner(OutputTimes_, OutputTimes);
		inter->Stage_SetCleaner(OutputTimes_, DeleteFolders);
	}
}

#endif