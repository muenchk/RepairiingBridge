#pragma once

#ifndef H_GENPROG
#define H_GENPROG

#include <unistd.h>
#include "Interface.h"
#include "Log.h"
#include "Processes.h"

namespace GenProg
{
#define GenProg_ std::string("GenProg")

#define GenProg_Timeout std::string("GenProg_Timeout")
#define GenProg_TestScriptFunction std::string("GenProg_TestScriptFunction")

#define GenProg_MainArgs std::string("GenProg_MainArgs")
#define GenProg_VarArgs std::string("GenProg_VarArgs")
#define GenProg_Dir std::string("GenProg_Dir")
#define GenProg_TestTimeout std::string("GenProg_TestTimeOut") // seconds
#define GenProg_DisableFileBackup std::string("GenProg_DisableFileBackup")

	enum class GenProgMode
	{
		SingleFile,
		MultiFile,
	};

	GenProgMode mode = GenProgMode::SingleFile;
	std::string dir;
	std::string dirtmp;
	int testtimeout = 0;
	bool testfunction = false;
	std::string testfunctions = "";

	void DeleteGenProgFolders(Bridge::Interface* inter)
	{
		try {
			std::filesystem::remove_all(std::filesystem::path(inter->GetWorkDir(GenProg_)));
		}
		catch (std::filesystem::filesystem_error&)
		{
			logger::error("GenProg", "DeleteGenProgFolders", "cannot remove working dir folder");
		}
	}

	/// <summary>
	/// Copies the repair that has been found from the working dir to the output folder
	/// </summary>
	/// <param name="inter"></param>
	void CopyRepairToOutput(Bridge::Interface* inter)
	{
		std::string output = inter->FindFile(OUTPUTDIR);
		std::filesystem::path genout = std::filesystem::path(output) / ("genprog" + std::to_string(inter->GetIteration()));
		std::filesystem::path tmp = std::filesystem::path(dirtmp);

		// copy repair
		switch (mode)
		{
		case GenProgMode::SingleFile :
			try {
				std::filesystem::create_directories(genout);
				std::filesystem::copy_file(tmp / "repair.c", genout / "repair.c");
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("Genprog", "CopyRepairToOutput", (std::string)"cannot copy repair.c to output folder. error: " + e.what());
			}
			break;
		case GenProgMode::MultiFile:
			try
			{
				std::filesystem::create_directories(genout / "repair");
				std::filesystem::copy(tmp / "repair", genout / "repair", std::filesystem::copy_options::recursive);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("GenProg", "CopyRepairToOutput", (std::string)"cannot copy repair to output folder. error: " + e.what());
			}
			break;
		}

	}

	/// <summary>
	/// Converts all symbols in a string into lower case.
	/// </summary>
	/// <param name="s"></param>
	/// <returns></returns>
	static std::string ToLower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) { return (unsigned char)std::tolower(c); }  // correct
		);
		return s;
	}

	/// <summary>
	/// Function Handling the execution of GenProg
	/// </summary>
	/// <param name="inter"></param>
	/// <param name="data"></param>
	/// <returns></returns>
	std::pair<bool, std::string> GenProg(Bridge::Interface* inter, Bridge::InputData* data, Bridge::StageData* stage)
	{
		if (stage->executable == "")
		{
			logger::error("GenProg", "GenProg", "executable is empty");
			return { false, "" };
		}

		// check whether we have negative test cases
		bool failing = false;
		for (int i = 0; i < data->labels.size(); i++)
		{
			failing |= !data->labels[i];
		}
		if (failing == false)
		{
			logger::error("GenProg", "GenProg", "there are no negative test cases");
			return { false, "" };
		}

		// read command args
		std::string mainargs = inter->GetConfArg(GenProg_MainArgs);
		// read variable args
		std::string varargs = inter->GetConfArg(GenProg_VarArgs);
		// args have the form "arg1,arg2,arg3,..." so we need to split them
		// split mainargs
		std::vector<std::string> args;
		size_t pos;
		while (mainargs != "")
		{
			if ((pos = mainargs.find(',')) == std::string::npos) {
				// found last opt arguments
				args.push_back({ mainargs});
				break;
			}
			// found other than last opt arg
			std::string arg = mainargs.substr(0, pos);
			if (arg.empty() == false && (arg[arg.length() - 1] == '\n' || arg[arg.length() - 1] == '\r'))
				arg = arg.substr(0, arg.length() - 1);
			args.push_back({ arg});
			mainargs.erase(0, pos + 1);
		}
		// split varargs
		std::vector<std::tuple<std::string, Bridge::ArgsType>> optargs;
		pos;
		while (varargs != "")
		{
			if ((pos = varargs.find(',')) == std::string::npos) {
				// found last opt arguments
				optargs.push_back({ varargs,Bridge::ArgsType::Combined });
				break;
			}
			// found other than last opt arg
			std::string arg = varargs.substr(0, pos);
			if (arg.empty() == false && (arg[arg.length() - 1] == '\n' || arg[arg.length() - 1] == '\r'))
				arg = arg.substr(0, arg.length() - 1);
			optargs.push_back({ arg, Bridge::ArgsType::Combined });
			varargs.erase(0, pos + 1);
		}
		// set arguments
		logger::info("GenProg", "GenProg", "Setting command line args");
		inter->Stage_SetCommandLineArgs_Options(GenProg_, args, optargs);
		logger::info("GenProg", "GenProg", "Generating command line args");
		stage->GenerateCmdArgs();

		// find genprog folders
		dir = inter->FindFile(GenProg_Dir);
		if (dir == "")
		{
			logger::error("GenProg", "GenProg", "genprog dir does not exist");
			return { false, "" };
		}
		logger::info("GenProg", "GenProg", "genprog dir evaluated to: \"" + dir + "\"");
		dirtmp = (std::filesystem::path(inter->GetWorkDir(GenProg_)) / "working").string();
		try
		{
			std::filesystem::create_directories(dirtmp);
			std::filesystem::create_directories(std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata");
		}
		catch (std::filesystem::filesystem_error& e)
		{
			logger::error("GenProg", "GenProg", "cannot create temporary folders");
			return { false, "" };
		}
		logger::info("GenProg", "GenProg", "genprog tmp dir set to: \"" + dirtmp + "\"");

		// find out whether genprog is working on one or multiple files and the the operation mode for the output operation after
		// we found a repair
		if (std::filesystem::exists(std::filesystem::path(dir) / "multi.txt"))
		{
			// multiple files
			mode = GenProgMode::MultiFile;
		}
		else
		{
			mode = GenProgMode::SingleFile;
		}

		// build the test.sh file in the workdir
		logger::info("GenProg", "GenProg", "Generate test.sh");
		std::ofstream testsh((std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata" / "test.sh").string());
		int postests = 0;
		int negtests = 0;
		if (testsh.is_open())
		{

			// write generic stuff
			testsh << "#!/bin/bash" << "\n";
			testsh << "# $1 = EXE" << "\n";
			testsh << "# $2 = test name" << "\n";
			testsh << "# $3 = port" << "\n";
			testsh << "# $4 = source name" << "\n";
			testsh << "# $5 = single-fitness-file name" << "n";
			testsh << "# exit 0 = success" << "\n";
			// write time limit
			testsh << "ulimit -t " << testtimeout << "\n";
			// generic
			testsh << "echo $1 $2 $3 $4 $5 >> testruns.txt" << "\n";
			if (testfunction)
				testsh << "\n" << testfunctions << "\n";
			testsh << "case $2 in" << "\n";
			// write positive and negative test cases
			if (testfunction == false)
			{
				for (int c = 0; c < data->inputs.size(); c++)
				{
					std::string tmpofile = (std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata" / ("output" + std::to_string(c))).string();
					std::string tmpofilename = "output" + std::to_string(c);
					if (data->labels[c] == true)
					{
						// passing
						postests++;
						// form: "  p1) $1 args | diff outfile - && exit 0 ;;"
						testsh << "  p" << postests << ") $1 " << data->inputs[c] << " | diff " << tmpofilename << " - && exit 0 ;;" << "\n";
					}
					else
					{
						// failing
						negtests++;
						// form: "  n1) $1 args | diff outfile - && exit 0 ;;"
						testsh << "  n" << negtests << ") $1 " << data->inputs[c] << " | diff " << tmpofilename << " - && exit 0 ;;" << "\n";
					}
					// write the program output to file
					std::ofstream tmpofiles(tmpofile);
					if (tmpofiles.is_open())
					{
						tmpofiles << data->outputs[c];
						tmpofiles.close();
					}
					else
					{
						logger::error("GenProg", "GenProg", "cannot write output file");
						return { false, "" };
					}
					// finished this round
				}
			}
			else
			{
				// with testfunction
				for (int c = 0; c < data->inputs.size(); c++)
				{
				    std::string tmpofile = (std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata" / ("output" + std::to_string(c))).string();
					std::string tmpofilename = "output" + std::to_string(c);
					if (data->labels[c] == true)
					{
						// passing
						postests++;
						// form: "  p1) $1 args | diff outfile - && exit 0 ;;"
						testsh << "  p" << postests << ") run_test_pos $1 " << data->inputs[c] << " " << tmpofilename << " && exit 0;; " << "\n";
					}
					else
					{
						// failing
						negtests++;
						// form: "  n1) $1 args | diff outfile - && exit 0 ;;"
						testsh << "  n" << negtests << ") run_test_neg $1 " << data->inputs[c] << " " << tmpofilename << " && exit 0;; " << "\n";
					}
					// write the program output to file
					std::ofstream tmpofiles(tmpofile);
					if (tmpofiles.is_open())
					{
						tmpofiles << data->outputs[c];
						tmpofiles.flush();
						tmpofiles.close();
					}
					else
					{
						logger::error("GenProg", "GenProg", "cannot write output file");
						return { false, "" };
					}
					// finished this round
				}
			}
			testsh << "  \n";
			// generic
			/*testsh << "  s) # single-valued fitness" << "\n";
			testsh << "  let fit=0" << "\n";
			// write test case stuff
			for (int c = 0; c < data->inputs.size(); c++)
			{
				std::string tmpofilename = "output" + std::to_string(c);
				if (data->labels[c] == true)
				{
					// passing
					// form: "  $1 args | diff outfile - && let fit=$fit+1"
					testsh << "  $1 " << data->inputs[c] << " | diff " << tmpofilename << " - && let fit=$fit+1" << "\n";
				}
				else
				{
					// failing
					// form: "  ($1 args | diff outfile -) && let fit=$fit+1"
					testsh << "  ($1 " << data->inputs[c] << " | diff " << tmpofilename << " -) && let fit=$fit+1" << "\n";
				}
			}
			testsh << "  let passed_all_so_stop_search=\"$fit >= " << data->inputs.size() << "\"" << "\n";
			testsh << "  echo $ft > $5" << "\n";
			testsh << "  if [ $passed_all_so_stop_search -eq 1 ] ; then" << "\n";
			testsh << "    exit 0" << "\n";
			testsh << "  else" << "\n";
			testsh << "    exit 1" << "\n";
			testsh << "  fi;;" << "\n";
			testsh << "\n";
			testsh << "\n";*/
			testsh << "esac" << "\n";
			testsh << "exit 1";
			testsh.close();
		}
		else
		{
			logger::error("GenProg", "GenProg", "cannot create file: test.sh");
			return { false, "" };
		}
		// use chmod to make it executable
		logger::info("GenProg", "GenProg", "making test.sh executabele");
		auto [finished, ext] = fork_exec("/bin/bash", { "-c", "chmod 777 " + (std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata" / "test.sh").string() },0, "");
		logger::info("GenProg", "GenProg", "exitcode: " + std::to_string(ext));

		// now that we have all necessary files, generate command line args

		// run iterations of executions

		bool runstandard = false;
		int currentiterations = 0;

		while (stage->ArgumentsUnused() > 0 || runstandard == false)
		{
			if (stage->ArgumentsUnused() == 0)
				runstandard = true;
			stage->iterations++;
			currentiterations++;
			logger::info("GenProg", "GenProg", "Beginning Iteration " + std::to_string(stage->ArgumentsTotal() - stage->ArgumentsUnused()) + "/" + std::to_string(stage->ArgumentsTotal()));
			// create dir tmp for current iteration
			try
			{
				std::filesystem::create_directories(dirtmp);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("GenProg", "GenProg", "cannot create temporary folders");
				return { false, "" };
			}
			// copy files from dir to tmpdir
			try
			{
				std::filesystem::copy(std::filesystem::path(dir), std::filesystem::path(dirtmp), std::filesystem::copy_options::recursive);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("GenProg", "GenProg", "cannot copy content of directory: " + dir + "\t to: " + dirtmp + "\terror: " + e.what());
				DeleteGenProgFolders(inter);
				return { false, "" };
			}
			// copy files from workingdir/gendata to tmpdir
			try
			{
				std::filesystem::copy((std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata"), std::filesystem::path(dirtmp), std::filesystem::copy_options::recursive);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("GenProg", "GenProg", "cannot copy content of directory: " + (std::filesystem::path(inter->GetWorkDir(GenProg_)) / "gendata").string() + "\t to: " + dirtmp + "\terror: " + e.what());
				DeleteGenProgFolders(inter);
				return { false, "" };
			}
			logger::info("GenProg", "GenProg", "copied files to workingdir/working");

			if (chdir(dirtmp.c_str()) != 0)
			{
				logger::warn("GenProg", "GenProg", "Couldn't set working directory- May lead to crash of genprog");
			}

			// run iteration
			auto [execfinished, execexit, execoutput] = inter->Exec(GenProg_, std::vector<std::string>{"--pos-tests", std::to_string(postests), "--neg-tests", std::to_string(negtests)}); 
			logger::info("GenProg", "GenProg", "finished execution");

			// reset working dir
			if (chdir(inter->working_dir.c_str()) != 0)
			{
				logger::warn("GenProg", "GenProg", "Couldn't set working directory- May lead to crash of genprog");
			}

			if (inter->archivefiles && inter->archiveworkingfiles && ToLower(inter->GetConfArg(GenProg_DisableFileBackup)) != "true")
			{
				auto targetpath = inter->Stage_GetArchiveDir(GenProg_) / ("iteration" + std::to_string(currentiterations));
				try {
					std::filesystem::create_directories(targetpath);
					std::filesystem::copy(inter->GetWorkDir(GenProg_), targetpath, std::filesystem::copy_options::recursive);
				}
				catch (std::filesystem::filesystem_error& e)
				{
					logger::error("GenProg", "GenProg", "cannot backup working directory: " + inter->GetWorkDir(GenProg_) + "to: " + targetpath.string() + " error message: " + std::string(e.what()));
				}
				logger::info("GenProg", "GenProg", "saved working dir backup");
			}

			if (!execfinished)
			{
				logger::warn("GenProg", "GenProg", "GenProg has exceeded its timelimit. Execution has been aborted.");
				// remove tmp folder
				try {
					//std::filesystem::remove_all(std::filesystem::path(dirtmp));
				}
				catch (std::filesystem::filesystem_error&)
				{
					logger::error("GenProg", "genProg", "cannot remove tmp folder");
					DeleteGenProgFolders(inter);
					return { false, "" };
				}
				continue;
			}

			// evaluate result
			pos = execoutput.find("Repair Found");
			if (pos != std::string::npos)
			{
				// repair has been found: exit
				logger::info("GenProg", "GenProg", "Repair has been found");

				CopyRepairToOutput(inter);
				DeleteGenProgFolders(inter);
				return { true, "" };
			}

			// delete workingdir/working for next iteration
			try {
				std::filesystem::remove_all(std::filesystem::path(dirtmp));
			}
			catch (std::filesystem::filesystem_error&)
			{
				logger::error("GenProg", "GenProg", "cannot remove workingdir/working folder for next iteration");
			}
			
			//change work dirs before starting / after finishing genprog

		}

		logger::info("GenProg", "GenProg", "All iterations did not succeed in finding a solution");

		// iterate stage until a result is found, or until there are no further arguments
		DeleteGenProgFolders(inter);
		return { false, "" };
	}


	/// <summary>
	/// Registers the stage in Interface
	/// </summary>
	void SetTargets(int stageid)
	{
		// set generation of PUT output to true
		auto inter = Bridge::Interface::GetSingleton();
		std::string testffile = inter->FindFile(GenProg_TestScriptFunction);
		if (testffile != "") {
			testfunction = true;
			std::ifstream infunc(testffile);
			if (infunc.is_open())
			{
				std::stringstream ss;
				ss << infunc.rdbuf();
				testfunctions = ss.str();
			}
			else
				testfunction = false;
		}
		inter->Stage_Create("GenProg");
		inter->Stage_SetRunner(GenProg_, GenProg);
		inter->Stage_SetCleaner(GenProg_, DeleteGenProgFolders);
		inter->Stage_SetExecutable(GenProg_, inter->FindExecutable(GenProg_));
		int timeout = 0;
		std::string tmp = inter->GetConfArg(GenProg_Timeout);
		if (tmp != "")
		{
			try {
				timeout = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("GenProg", "SetTargets", "Cannot get timeout for stage " + GenProg_);
			}
			catch (std::invalid_argument&) {
				logger::warn("GenProg", "SetTargets", "Cannot get timeout for stage " + GenProg_);
			}
		}
		inter->Stage_SetTimeout(GenProg_, timeout);
		tmp = inter->GetConfArg(GenProg_TestTimeout);
		if (tmp != "")
		{
			try {
				timeout = std::stoi(tmp);
			}
			catch (std::out_of_range&) {
				logger::warn("GenProg", "SetTargets", "Cannot get test timeout for stage " + GenProg_);
			}
			catch (std::invalid_argument&) {
				logger::warn("GenProg", "SetTargets", "Cannot get test timeout for stage " + GenProg_);
			}
		}
		testtimeout = timeout;


		//////////////
		/////////////
		////////////
		// set arguments remains
	}
}

#endif