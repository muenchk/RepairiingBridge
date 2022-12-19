#pragma once

#ifndef H_OUTPUTTESTS
#define H_OUTPUTTESTS

#include "Log.h"
#include "Interface.h"

namespace OutputTests
{
#define OutputTests_ std::string("OutputTests")

	void DeleteFolders(Bridge::Interface* inter)
	{
		std::filesystem::path output = inter->FindFile(OUTPUTDIR);
		std::filesystem::path outout = output / ("tests" + std::to_string(inter->GetIteration()));
		try
		{
			std::filesystem::remove_all(outout);
		}
		catch (std::filesystem::filesystem_error&)
		{ }
	}

	/// <summary>
	/// Main handler for this stage.
	/// In contrast to normal operation this stage does not end the program on failure and just aborts the stage itself
	/// </summary>
	/// <param name="inter"></param>
	/// <param name="data"></param>
	/// <param name="stage"></param>
	/// <returns></returns>
	std::pair<bool, std::string> OutputTests(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		std::filesystem::path output = inter->GetConfArg(OUTPUTDIR);
		std::filesystem::path outout = output / "tests";

		// create output folder
		try
		{
			std::filesystem::create_directories(outout);
		}
		catch (std::filesystem::filesystem_error& e)
		{
			logger::error("OutputTests", "OutputTests", "cannot create outputdir/tests. error: " + (std::string)e.what());
			logger::error("OutputTests", "Outputtests", "Aborting stage. Continuing to next stage.");
			DeleteFolders(inter);
			return { true, "" };
		}

		// open files.csv
		std::ofstream filescsv(outout / "files.csv");
		if (filescsv.is_open())
		{
			// write header
			filescsv << "inputfilename,label(int),outputfilename" << "\n";
			// csv is opened. now write input + output and then csv entry
			for (int i = 0; i < data->inputs.size(); i++)
			{
				try
				{
					std::string tmpname = std::to_string(i) + ".txt";
					std::ofstream fin(outout / ("input" + tmpname));
					std::ofstream fout(outout / ("output" + tmpname));
					if (fin.is_open() && fout.is_open())
					{
						// files are open, so write stuff
						fin << data->inputs[i];
						fin.close();
						fout << data->outputs[i];
						fout.close();
						if (data->labels[i] == true)
							filescsv << "input" << tmpname << "," << "1" << "," << "output" << tmpname << "\n";
						else
							filescsv << "input" << tmpname << "," << "0" << "," << "output" << tmpname << "\n";
					}
					else
					{
						// files cannot be opened, so skip
						logger::error("OutputTests", "OutputTests", "cannot open input and output files. Skip entry.");
					}
				}
				catch (std::exception& e)
				{
					logger::error("OutputTests", "OutputTests", "Cannot write entry. error: " + (std::string)e.what());
				}
			}
			// wrote all entries, so we are finished
			filescsv.close();
		}
		else
		{
			logger::error("OutputTests", "OutputTests", "cannot open files.csv.");
			logger::error("OutputTests", "Outputtests", "Aborting stage. Continuing to next stage.");
			DeleteFolders(inter);
			return { true, "" };
		}
		return { true, "" };
	}

	/// <summary>
	/// Registers the stage in Interface
	/// </summary>
	void SetTargets(int stageid)
	{
		auto inter = Bridge::Interface::GetSingleton();
		inter->Stage_Create(OutputTests_);
		inter->Stage_SetCleaner(OutputTests_, DeleteFolders);
		inter->Stage_SetRunner(OutputTests_, OutputTests);
	}
}



#endif