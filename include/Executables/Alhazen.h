#pragma once

#ifndef H_ALHAZEN
#define H_ALHAZEN

#include <Interface.h>
#include <Log.h>
#include <InputData.h>
#include <random>
#include <climits>
#include <exception>

namespace Alhazen
{
#define Alhazen_L std::string("Alhazen_Learn")
#define Alhazen_G std::string("Alhazen_Generate")
#define Alhazen_P std::string("Alhazen_Predict")

#define Alhazen_L_Timeout std::string("Alhazen_Learn_Timeout")
#define Alhazen_G_Timeout std::string("Alhazen_Generate_Timeout")
#define Alhazen_P_Timeout std::string("Alhazen_Predict_Timeout")

#define Alhazen_Dir std::string("Alhazen_Dir")
#define Alhazen_Bug std::string("Alhazen_Bug")
#define Alhazen_Samples std::string("Alhazen_Samples")
#define Alhazen_Seed std::string("Alhazen_Seed")

#define Alhazen_FailingInputsNeeded std::string("Alhazen_FailingInputsNeeded")
#define Alhazen_MaxIterations std::string("Alhazen_MaxIterations")

	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
	/// <summary>
	/// trims random numbers to 1 to INT_MAX
	/// </summary>
	std::uniform_int_distribution<signed> randMAX(1, INT_MAX);

	std::string dir;
	std::string dirtmp;
	std::string bug_py;
	std::string samples;
	int seed;

	int iterations = 0;
	int failinginputs = 0;
	int maxfailinginputs = 10;
	int maxiterations = 10;

	void DeleteAlhazenFolders(Bridge::Interface* inter)
	{
		// remove predict tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_P)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove tmp Predict folder");
		}
		// remove generate tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_G)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove tmp Generate folder");
		}
		// finally remove learn tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_L)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove tmp Learn folder");
		}
		// delete temporary folder
		if (dir == "")
		{
			dir = inter->FindFile(Alhazen_Dir);
		}
		if (dirtmp == "")
		{
			dirtmp = dir + "_tmp";
		}
		try {
			std::filesystem::remove_all(std::filesystem::path(dirtmp));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove tmp folder");
		}
		// delete automaton folder
		try {
			std::filesystem::remove_all(std::filesystem::path(dir).parent_path() / "automaton");
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove automaton folder");
		}
		// delete generatetree-generate.log.bz2 folder
		try {
			std::filesystem::remove_all(std::filesystem::path(dir).parent_path() / "generatetree-generate.log.bz2");
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove generatetree-genrate.log.bz2 folder");
		}
		// delete generatetree-predict.log.bz2 folder
		try {
			std::filesystem::remove_all(std::filesystem::path(dir).parent_path() / "generatetree-predict.log.bz2");
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "DeleteAlhazenFolders", "cannot remove generatetree-predict.log.bz2 folder");
		}
	}

	/// <summary>
	/// Function executing the learning step of alhazen
	/// </summary>
	/// <param name="inter"></param>
	/// <returns></returns>
	std::pair<bool, std::string> Alhazen_Learn(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		if (stage->executable == "")
		{
			logger::error("Alhazen", "Alhazen_Learn", "executable is empty");
			return { false, "" };
		}
		// reset internal variables
		iterations = 0;
		failinginputs = 0;

		dir = inter->FindFile(Alhazen_Dir);
		if (dir == "")
		{
			logger::error("Alhazen", "Alhazen_Learn", "alhazen dir does not exist");
			return { false, "" };
		}
		logger::info("Alhazen", "Alhazen_Learn", "alhazen dir evaluated to: \"" + dir + "\"");
		dirtmp = dir + "_tmp";
		logger::info("Alhazen", "Alhazen_Learn", "dir_tmp evaluated to: \"" + dirtmp + "\"");
		bug_py = inter->GetConfArg(Alhazen_Bug);
		if (bug_py == "")
		{
			logger::error("Alhazen", "Alhazen_Learn", "bug ís empty");
			return { false, "" };
		}
		logger::info("Alhazen", "Alhazen_Learn", "bug_py evaluated to: \"" + bug_py + "\"");
		std::string tmp = inter->GetConfArg(Alhazen_Seed);
		if (tmp != "")
		{
			try {
				seed = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("Alhazen", "Alhazen_Learn", "Cannot get seed");
			}
			catch (std::invalid_argument&) {
				logger::warn("Alhazen", "Alhazen_Learn", "Cannot get seed");
			}
		}
		else {
			seed = 0;
		}
		logger::info("Alhazen", "Alhazen_Learn", "seed evaluated to: \"" + std::to_string(seed) + "\"");
		samples = inter->FindFile(Alhazen_Samples);
		if (samples == "")
		{
			logger::error("Alhazen", "Alhazen_Learn", "samples dir does not exist");
			return { false, "" };
		}
		logger::info("Alhazen", "Alhazen_Learn", "samples evaluated to: \"" + samples + "\"");
		// copy all necessary files to a temporary folder
		try {
			std::filesystem::copy(std::filesystem::path(dir), std::filesystem::path(dirtmp), std::filesystem::copy_options::recursive);
		}
		catch (std::filesystem::filesystem_error& e)
		{
			logger::error("Alhazen", "Alhazen_Learn", "cannot copy content of directory: " + dir + "\t to: " + dirtmp + "\terror: " + e.what());
			return { false, "" };
		}
		try {
			std::filesystem::copy(std::filesystem::path(samples), std::filesystem::path(dirtmp) / "samples", std::filesystem::copy_options::recursive);
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Learn", "cannot copy directory: " + samples + "\t to: " + dirtmp);
			return { false, "" };
		}
		try {
			std::filesystem::copy(std::filesystem::path(inter->GetConfArg(PUT)), std::filesystem::path(dirtmp));
		}
		catch (std::filesystem::filesystem_error& e)
		{
			logger::error("Alhazen", "Alhazen_Learn", "cannot copy PUT to: " + dirtmp + " error message: " + std::string(e.what()));
			return { false, "" };
		}

		logger::info("Alhazen", "Alhazen_Learn", "Setting command line args");
		inter->Stage_SetCommandLineArgs_Options(Alhazen_L, { std::string("-s"), std::to_string(seed), (std::filesystem::path(dirtmp) / bug_py).string(), inter->GetWorkDir(Alhazen_L) }, {});

		logger::info("Alhazen", "Alhazen_Learn", "generating command line args");
		stage->GenerateCmdArgs();

		logger::info("Alhazen", "Alhazen_Learn", "executing alhazen");
		auto [execfinished, execexit, execoutput] = inter->Exec(Alhazen_L, std::vector<std::string>{});
		logger::info("Alhazen", "Alhazen_Learn", "finished execution");
		if (inter->archivefiles && inter->archiveworkingfiles) {
			try {
				std::filesystem::create_directories(inter->Stage_GetArchiveDir(Alhazen_L));
				std::filesystem::copy(inter->GetWorkDir(Alhazen_L), inter->Stage_GetArchiveDir(Alhazen_L), std::filesystem::copy_options::recursive);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("Alhazen", "Alhazen_Learn", "cannot backup working directory: " + inter->GetWorkDir(Alhazen_L) + "to: " + inter->Stage_GetArchiveDir(Alhazen_L).string() + " error message: " + std::string(e.what()));
			}
			logger::info("Alhazen", "Alhazen_Learn", "saved working dir backup");
		}
		if (!execfinished)
		{
			logger::error("Alhazen", "Alhazen_Learn", "Alhazen aborted due to timeout");
			DeleteAlhazenFolders(inter);
			return { false, "" };
		}
		///////
		//////
		/////
		////
		///
		// maybe do something else here?
		return { true, "" };
	}

	/// <summary>
	/// Function executing the generate step of alhazen
	/// </summary>
	/// <param name="inter"></param>
	/// <returns></returns>
	std::pair<bool, std::string> Alhazen_Generate(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		if (stage->executable == "")
		{
			logger::error("Alhazen", "Alhazen_Generate", "executable is empty");
			DeleteAlhazenFolders(inter);
			return { false, "" };
		}
		// remove old generate tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_G)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Predict", "cannot remove old tmp Generate folder");
		}
		// remove old predict tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_P)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Predict", "cannot remove old tmp Predict folder");
		}

		// increase number of iterations run
		iterations++;

		// create new seed
		seed = randMAX(rand);

		inter->Stage_SetCommandLineArgs_Options(Alhazen_G, { "--bug-module", (std::filesystem::path(dirtmp) / bug_py).string(), inter->GetWorkDir(Alhazen_L), "generate", "--gendir", inter->GetWorkDir(Alhazen_G), "-s", std::to_string(seed) }, std::vector<std::tuple<std::string,Bridge::ArgsType>>{});
		stage->GenerateCmdArgs();
		auto [execfinished, execexit, execoutput] = inter->Exec(Alhazen_G, std::vector<std::string>{});
		logger::info("Alhazen", "Alhazen_Generate", "finished execution");
		if (inter->archivefiles && inter->archiveworkingfiles) {
			auto targetpath = inter->Stage_GetArchiveDir(Alhazen_G) / ("iteration" + std::to_string(iterations));
			try {
				std::filesystem::create_directories(targetpath);
				std::filesystem::copy(inter->GetWorkDir(Alhazen_G), targetpath, std::filesystem::copy_options::recursive);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("Alhazen", "Alhazen_Generate", "cannot backup working directory: " + inter->GetWorkDir(Alhazen_G) + "to: " + targetpath.string() + " error message: " + std::string(e.what()));
			}
			logger::info("Alhazen", "Alhazen_Generate", "saved working dir backup");
		}
		if (!execfinished)
		{
			logger::error("Alhazen", "Alhazen_Generate", "Alhazen_tree aborted due to timeout");
			DeleteAlhazenFolders(inter);
			return { false, "" };
		}
		///////
		//////
		/////
		////
		///
		// maybe do something else here?
		return { true, "" };
	}

	/// <summary>
	/// Function executing the prediction step of alhazen
	/// </summary>
	/// <param name="inter"></param>
	/// <returns></returns>
	std::pair<bool, std::string> Alhazen_Predict(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		if (stage->executable == "")
		{
			logger::error("Alhazen", "Alhazen_Predict", "executable is empty");
			DeleteAlhazenFolders(inter);
			return { false, "" };
		}

		// we are now at the stage, where we predict whether an input is a bug or not
		// afterwards we can ad the output to our InputData
		std::filesystem::create_directory(std::filesystem::path(inter->GetWorkDir(Alhazen_P)));
		inter->Stage_SetCommandLineArgs_Options(Alhazen_P, { "--bug-module", (std::filesystem::path(dirtmp) / bug_py).string(), inter->GetWorkDir(Alhazen_L), "predict", "--samples", (std::filesystem::path(inter->GetWorkDir(Alhazen_G)) / "samples").string(), "--csv", (std::filesystem::path(inter->GetWorkDir(Alhazen_P)) / "results.csv").string()}, std::vector<std::tuple<std::string,Bridge::ArgsType>>{});
		stage->GenerateCmdArgs();
		auto [execfinished, execexit, execoutput] = inter->Exec(Alhazen_P, std::vector<std::string>{});
		logger::info("Alhazen", "Alhazen_Predict", "finished execution");
		if (inter->archivefiles && inter->archiveworkingfiles)
		{
			auto targetpath = inter->Stage_GetArchiveDir(Alhazen_P) / ("iteration" + std::to_string(iterations));
			try {
				std::filesystem::create_directories(targetpath);
				std::filesystem::copy(inter->GetWorkDir(Alhazen_P), targetpath, std::filesystem::copy_options::recursive);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("Alhazen", "Alhazen_Predict", "cannot backup working directory: " + inter->GetWorkDir(Alhazen_P) + "to: " + targetpath.string() + " error message: " + std::string(e.what()));
			}
			logger::info("Alhazen", "Alhazen_Predict", "saved working dir backup");
		}
		if (!execfinished)
		{
			logger::error("Alhazen", "Alhazen_Predict", "Alhazen_tree aborted due to timeout");
			DeleteAlhazenFolders(inter);
			return { false, "" };
		}


		// read "<(std::filesystem::path(inter->GetWorkDir(Alhazen_P)) / "results.csv").string()>" line for line and extract bug or no bug for each of the sample files in "(std::filesystem::path(inter->GetWorkDir(Alhazen_G)) / "samples").string()"

		// open results.csv
		std::ifstream infile((std::filesystem::path(inter->GetWorkDir(Alhazen_P)) / "results.csv").string());
		//std::error_code e;
		//std::filesystem::copy_file((std::filesystem::path(inter->GetWorkDir(Alhazen_P)) / "results.csv"), std::filesystem::path("results.csv"), e);
		if (infile.is_open())
		{
			std::string line;
			std::string filename;
			std::string filepath;
			std::string status;
			std::string bugstatus;
			size_t pos; 
			// skip first line -> header
			std::getline(infile, line);
			// read lines and evaluate the results in it
			while (std::getline(infile, line))
			{
				// skip empty lines
				if (line.empty())
					continue;
				// split string
				std::vector<std::string> splits = [](std::string line) {
					size_t pos = 0;
					std::vector<std::string> vec;
					while ((pos = line.find(',')) != std::string::npos)
					{
						vec.push_back(line.substr(0, pos));
						line.erase(0, pos + 1);
					}
					vec.push_back(line);
					return vec;
				}(line);

				if (splits.size() < 4)
				{
					logger::error("Alhazen", "Alhazen_Predict", "results.csv line has too few fields: " + std::to_string(splits.size()) + " expected at least 4.");
					continue;
				}
				// filename is first field
				filename = splits[0];

				// file path is third to last field
				filepath = splits[splits.size() - 3];

				// status is second to last field
				status = splits[splits.size() - 2];
				if (status != std::string("valid"))
				{
					logger::warn("Alhazen", "Alhazen_Predict", "results.csv line is not valid");
					continue;
				}

				// bug status is last field
				bugstatus = splits[splits.size() - 1];

				if (bugstatus == std::string("BUG"))
				{
					// add as failing test case to data
					std::ifstream input((std::filesystem::path(inter->GetWorkDir(Alhazen_G)) / "samples" / filename).string());
					std::stringstream ss;
					ss << input.rdbuf();
					// make sure the input does not contain any new lines, etc.
					std::string putinput = ss.str();
					if (putinput.length() > 0 && (putinput[putinput.length() - 1] == '\n' || putinput[putinput.length() - 1] == '\r'))
						putinput = putinput.substr(0, putinput.length() - 1);
					inter->AddNewInput(putinput, false); // automatically generates output if needed
					failinginputs++;
					logger::info("Alhazen", "Alhazen_Predict", "Found failing input");
				}
				else if (bugstatus == std::string("NO_BUG"))
				{
					// add as passing test case to data
					std::ifstream input((std::filesystem::path(inter->GetWorkDir(Alhazen_G)) / "samples" / filename).string());
					std::stringstream ss;
					ss << input.rdbuf();
					std::string putinput = ss.str();
					if (putinput.length() > 0 && (putinput[putinput.length() - 1] == '\n' || putinput[putinput.length() - 1] == '\r'))
						putinput = putinput.substr(0, putinput.length() - 1);
					inter->AddNewInput(putinput, true); // automatically generates output if needed
					logger::info("Alhazen", "Alhazen_Predict", "Found passing input");
				}
			}
		}
		// everything is read, so end this stage

		// remove predict tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_P)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Predict", "cannot remove tmp Predict folder");
		}
		// remove generate tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_G)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Predict", "cannot remove tmp Generate folder");
		}


		// check whether we need to iterate once more
		// then skip deletion of other folders until everything is completed
		if (failinginputs < maxfailinginputs && iterations < maxiterations)
		{
			// skip back to generate step
			return { true, Alhazen_G };
		}
		// otherwise we will end alhazen execution here and go to whatever stage comes next

		// delete temporary folder if we do not need any more files
		try {
			std::filesystem::remove_all(std::filesystem::path(dirtmp));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Predict", "cannot remove tmp folder");
		}
		// finally remove learn tmp folder
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(Alhazen_L)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("Alhazen", "Alhazen_Predict", "cannot remove tmp Learn folder");
		}
		DeleteAlhazenFolders(inter);
		return { true, "" };
	}


	/// <summary>
	/// Registers the stage in Interface
	/// </summary>
	void SetTargets(int stageid)
	{
		auto inter = Bridge::Interface::GetSingleton();
		// set stuff for Learn phase
		inter->Stage_Create(Alhazen_L);
		inter->Stage_SetRunner(Alhazen_L, Alhazen_Learn);
		inter->Stage_SetCleaner(Alhazen_L, DeleteAlhazenFolders);
		inter->Stage_SetExecutable(Alhazen_L, inter->FindExecutable(Alhazen_L));
		int timeout = 0;
		std::string tmp = inter->GetConfArg(Alhazen_L_Timeout);
		if (tmp != "")
		{
			try {
				timeout = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("Alhazen", "SetTargets","Cannot get timeout for stage " + Alhazen_L);
			}
			catch (std::invalid_argument&) {
				logger::warn("Alhazen", "SetTargets", "Cannot get timeout for stage " + Alhazen_L);
			}
		}
		inter->Stage_SetTimeout(Alhazen_L, timeout);

		// set stuff for generation phase
		inter->Stage_Create(Alhazen_G);
		inter->Stage_SetRunner(Alhazen_G, Alhazen_Generate);
		inter->Stage_SetExecutable(Alhazen_G, inter->FindExecutable(Alhazen_G));
		timeout = 0;
		tmp = inter->GetConfArg(Alhazen_G_Timeout);
		if (tmp != "")
		{
			try {
				timeout = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("Alhazen", "SetTargets", "Cannot get timeout for stage " + Alhazen_G);
			}
			catch (std::invalid_argument&) {
				logger::warn("Alhazen", "SetTargets", "Cannot get timeout for stage " + Alhazen_G);
			}
		}
		inter->Stage_SetTimeout(Alhazen_P, timeout);

		// set stuff for prediction phase
		inter->Stage_Create(Alhazen_P);
		inter->Stage_SetRunner(Alhazen_P, Alhazen_Predict);
		inter->Stage_SetExecutable(Alhazen_P, inter->FindExecutable(Alhazen_P));
		timeout = 0;
		tmp = inter->GetConfArg(Alhazen_P_Timeout);
		if (tmp != "")
		{
			try {
				timeout = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("Alhazen", "SetTargets", "Cannot get timeout for stage " + Alhazen_P);
			}
			catch (std::invalid_argument&) {
				logger::warn("Alhazen", "SetTargets", "Cannot get timeout for stage " + Alhazen_P);
			}
		}
		inter->Stage_SetTimeout(Alhazen_G, timeout);

		try {
			maxiterations = std::stoi(inter->GetConfArg(Alhazen_MaxIterations));
		}
		catch (std::exception&) {
			logger::warn("Alhazen", "SetTargets", "Couldn't read " + Alhazen_MaxIterations);
		}
		try {
			maxfailinginputs = std::stoi(inter->GetConfArg(Alhazen_FailingInputsNeeded));
		}
		catch (std::exception&) {
			logger::warn("Alhazen", "SetTargets", "Couldn't read" + Alhazen_FailingInputsNeeded);
		}
	}
}

#endif