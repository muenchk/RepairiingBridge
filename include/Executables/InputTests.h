#pragma once

#ifndef H_INPUTTESTS
#define H_INPUTTESTS

#include "Log.h"
#include "Interface.h"

namespace InputTests
{
#define InputTests_ std::string("InputTests")
#define InputTests_Input std::string("InputTests_input")

	std::pair<bool, std::string> InputTests(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		// get working dir. We expect the input files there
		std::string dir = inter->FindFile(InputTests_Input);
		if (dir == "")
		{
			logger::error("InputTests", "InputTests", "Working directory cannot be retrieved");
			return { false, "" };
		}
		// find csv file
		std::string filescsv = (std::filesystem::path(dir) / "files.csv").string();
		if (!std::filesystem::exists(filescsv))
		{
			logger::error("InputTests", "InputTests", "Csv file defining input files and label couldn't be found\n");
			return { false, "" };
		}
		// read csv file and extract files and labels
		std::ifstream infile(std::filesystem::path(filescsv).string());
		if (infile.is_open())
		{
			std::string line;
			std::string inputfilename;
			std::string outputfilename;
			std::string labels;
			size_t pos = 0;
			while (std::getline(infile, line))
			{
				std::string inputfilename = "";
				std::string outputfilename = "";
				std::string labels = "";
				// skip empty lines, or comment lines
				if (line.empty() || line[0] == ';')
					continue;
				// get inputfilename
				if ((pos = line.find(',')) == std::string::npos)
				{
					logger::warn("InputTests", "InputTests", "files.csv: line has unexpected format. Skipping line.");
					continue;
				}
				inputfilename = line.substr(0, pos);
				line.erase(0, pos + 1);
				// get labels
				if ((pos = line.find(',')) == std::string::npos)
				{
					// we do not have an output file, so the rest of the line is the label
					labels = line;
				} else {
					labels = line.substr(0, pos);
					line.erase(0, pos + 1);
					// the remainder is the outputfilename
					outputfilename = line;
					if (outputfilename != "" && (outputfilename[outputfilename.length() - 1] == '\n' || outputfilename[outputfilename.length() - 1] == '\r')) {
						outputfilename = outputfilename.substr(0, outputfilename.length() - 1);
					}
				}
				inputfilename = (std::filesystem::path(dir) / inputfilename).string();
				if (inputfilename == "" || !std::filesystem::exists(inputfilename))
				{
					logger::error("InputTests", "InputTests", "files.csv: inputfile cannot be found");
					continue;
				}
				if (outputfilename != "")
				{
					outputfilename = (std::filesystem::path(dir) / outputfilename).string();
					if (outputfilename == "" || std::filesystem::exists(outputfilename) == false)
					{
						logger::error("InputTests", "InputTests", "files.csv: outputfile cannot be found\n" + outputfilename);
						continue;
					}
				}
				bool label = false;
				try
				{
					label = (bool)std::stoi(labels);
				}
				catch (std::exception&)
				{
					logger::error("InputTests", "InputTests", "files.csv: cannot convert field label to boolean");
					continue;
				}
				std::string input = "";
				std::string output = "";
				std::ifstream iifile(inputfilename);
				std::stringstream buff;
				buff << iifile.rdbuf();
				input = buff.str();
				iifile.close();
				if (outputfilename != "")
				{
					std::ifstream iofile(outputfilename);
					std::stringstream buff;
					buff << iofile.rdbuf();
					output = buff.str();
					iofile.close();
				}
				inter->AddNewInput(input, label);//, output);
			}
		}

		return { true, "" };
	}

	/// <summary>
	/// Registers the stage in Interface
	/// </summary>
	void SetTargets(int stageid)
	{
		auto inter = Bridge::Interface::GetSingleton();
		inter->Stage_Create(InputTests_);
		inter->Stage_SetRunner(InputTests_, InputTests);
	}
}

#endif