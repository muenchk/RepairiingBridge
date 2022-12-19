#include <Interface.h>

#include <unistd.h>
#include <Processes.h>
#include <Log.h>
#include <vector>

namespace Bridge
{
	std::string Sum(std::vector<std::string> vec)
	{
		std::string s = "";
		if (vec.size() > 0)
		{
			s = vec[0];
		}
		for (int i = 1; i < vec.size(); i++)
		{
			s += " " + vec[i];
		}
		return s;
	}
	std::string Sum(std::vector<std::tuple<std::string, ArgsType>> vec)
	{
		std::string s = "";
		if (vec.size() > 0)
		{
			s = std::get<0>(vec[0]);
		}
		for (int i = 1; i < vec.size(); i++)
		{
			s += " " + std::get<0>(vec[i]);
		}
		return s;
	}

	std::vector<std::string> SplitArguments(std::string par)
	{
		std::vector<std::string> argus;
		size_t pos = std::string::npos;
		std::string arg = "";
		while ((pos = par.find(' ')) != std::string::npos) {
			// we have found a blank. If the first sign is a quotation mark, extract the parameter and add it
			if (par[0] == '\"') {
				pos = par.substr(1, par.length() - 1).find('\"');
				if (pos != std::string::npos) {
					// found second quotation mark
					arg = par.substr(1, pos); // pos is 1-based
					par.erase(0, pos + 2);
					argus.push_back(arg);
					arg = "";
				} else {
					return std::vector<std::string>{};
				}
			}
			else if (par[0] == '\'') {
				pos = par.substr(1, par.length() - 1).find('\'');
				if (pos != std::string::npos) {
					// found second quotation mark
					arg = par.substr(1, pos); // pos is 1-based
					par.erase(0, pos + 2);
					argus.push_back(arg);
					arg = "";
				}
				else {
					return std::vector<std::string>{};
				}
			}
			else if (par.length() > 0) {
				argus.push_back(par.substr(0, pos));
				par.erase(0, pos + 1);
			}
		}
		// handle last argument if existing
		if (par != "") {
			if (par[0] == '\"') {
				if (par[par.length() - 1] == '\"')
					argus.push_back(par.substr(1, par.length() - 2));
				else
					return std::vector<std::string>{};
			}
			else if (par[0] == '\'') {
				if (par[par.length() - 1] == '\'')
					argus.push_back(par.substr(1, par.length() - 2));
				else
					return std::vector<std::string>{};
			}
			else
				argus.push_back(par);
		}
		// delete possible empty arguments
		auto itr = argus.begin();
		while (itr != argus.end()) {
			if (*itr == "") {
				argus.erase(itr);
				continue;
			}
			itr++;
		}
		return argus;
	}

	std::vector<std::string> StageData::GetNextCmdArgsVec()
	{
		if (argsidx < argslist.size())
		{
			std::vector<std::string> args;
			args.insert(args.end(), argsMain.begin(), argsMain.end());
			for (int i = 0; i < argslist[argsidx].size(); i++)
			{
				std::tuple<std::string, ArgsType> tup = argsOpt[argslist[argsidx][i]];
				if (std::get<1>(tup) == ArgsType::Single) {
					args.push_back(std::get<0>(tup));
				}
				else
				{
					std::vector<std::string> argus;
					size_t pos = std::string::npos;
					std::string par = std::get<0>(tup);
					// find first blank
					while ((pos = par.find(' ')) != std::string::npos)
					{
						// we have found a blank. If the first sign is a quotation mark, extract the parameter and add it
						if (par[0] == '\"')
						{
							pos = par.substr(1, par.length() - 1).find('\"');
							if (pos != std::string::npos)
							{
								// we have found the second quotation mark
								std::string arg = par.substr(1, pos); // pos is 1-based
								par.erase(0, pos + 2); // pos is index of the pos+1th char in substr and we need one more to capture the first char
								args.push_back(arg);
							}
							else
							{
								// if we cannot find the second quotation mark, we have a problem
								// since the current arg is not correct, delete all probable combinations with this
								// from the argslist. Since this error occurs on the first occurence of the argument
								// we only need to change argsidx
								int idx = argslist[argsidx][i];
								auto aiitr = argslist.begin();
								while (aiitr != argslist.end())
								{
									for (int x = 0; x < (*aiitr).size(); x++)
									{
										if ((*aiitr)[x] == idx) {
											argslist.erase(aiitr);
											continue;
										}

									}
									aiitr++;
								}
								// now that we have erased problematic arguments, generate the next args and return them
								// this is effectively a recursive call, but should not lead to stack issues since each 
								// failure iteration removes large amounts of arguments
								return GetNextCmdArgsVec();
							}
						}
						else if (par[0] == '\'')
						{
							pos = par.substr(1, par.length() - 1).find('\'');
							if (pos != std::string::npos)
							{
								// we have found the second quotation mark
								std::string arg = par.substr(1, pos); // pos is 1-based
								par.erase(0, pos + 2); // pos is index of the pos+1th char in substr and we need one more to capture the first char
								args.push_back(arg);
							}
							else
							{
							GetNextCMdArgsVecErrorHandler:
								// if we cannot find the second quotation mark, we have a problem
								// since the current arg is not correct, delete all probable combinations with this
								// from the argslist. Since this error occurs on the first occurence of the argument
								// we only need to change argsidx
								int idx = argslist[argsidx][i];
								auto aiitr = argslist.begin();
								while (aiitr != argslist.end())
								{
									for (int x = 0; x < (*aiitr).size(); x++)
									{
										if ((*aiitr)[x] == idx) {
											argslist.erase(aiitr);
											continue;
										}

									}
									aiitr++;
								}
								// now that we have erased problematic arguments, generate the next args and return them
								// this is effectively a recursive call, but should not lead to stack issues since each 
								// failure iteration removes large amounts of arguments
								return GetNextCmdArgsVec();
							}
						}
						else if (par.length() > 0)
						{
							// normal mode, just split at next empty space
							args.push_back(par.substr(0, pos));
							par.erase(0, pos + 1);
						}
					}
					// we are past the last blank, so the remainder is the last argument
					if (par != "")
						args.push_back(par);
					if (par != "") {
						if (par[0] == '\"') {
							if (par[par.length() - 1] == '\"')
								args.push_back(par.substr(1, par.length() - 2));
							else
								goto GetNextCMdArgsVecErrorHandler; // go to the error handler we used above, sicne that code fragment
																	// will exit the function in all cases we do not need to be concerned about problems
						}
						else if (par[0] == '\'') {
							if (par[par.length() - 1] == '\'')
								args.push_back(par.substr(1, par.length() - 2));
							else
								goto GetNextCMdArgsVecErrorHandler; // go to the error handler we used above, sicne that code fragment
							// will exit the function in all cases we do not need to be concerned about problems
						}
						else
							args.push_back(par);
					}
					
				}
			}
			// delete possible empty arguments
			auto itr = args.begin();
			while (itr != args.end()) {
				if (*itr == "") {
					args.erase(itr);
					continue;
				}
				itr++;
			}
			// return
			argsidx++;
			return args;
		}
		else
		{
			std::vector<std::string> args;
			args.insert(args.end(), argsMain.begin(), argsMain.end());
			return args;
		}
	}

	void combine(std::vector<int> &combination, std::vector<int> args, std::vector<std::vector<int>> & results, int offset, int k) {
		if (k == 0) {
			std::vector<int> comb;
			for (int i = 0; i < combination.size(); i++)
			{
				comb.push_back(combination[i]);
			}
			results.push_back(comb);
			return;
		}
		for (int i = offset; i <= args.size() - k; ++i) {
			combination.push_back(args[i]);
			combine(combination, args, results, i + 1, k - 1);
			combination.pop_back();
		}
	}

	void StageData::GenerateCmdArgs()
	{
		argslist.clear();
		argsidx = 0;
		// generate arguments for stage 1
		if (argsfixed.empty() == false) {
		}
		else
		{
			static std::vector<int> combination;
			if (argsOpt.size() > 0)
			{
				// generate all possible variants
				// iteration over the number of arguments
				std::vector<int> xargs;
				for (int i = 0; i < argsOpt.size(); i++)
					xargs.push_back(i);
				for (int c = 1; c < argsOpt.size(); c++)
				{
					// now create combinations
					combine(combination, xargs, argslist, 0, c);
				}
				// now we later just have to choose the right arguments
			}
		}
	}

	std::tuple<bool, int, std::string> Interface::GeneratePUTOutput(std::string input)
	{
		logger::info("Interface", "GeneratePUTOutput", "Start PUT output generation");
		std::string tmpfile = "RepBridOutputStage_PUT_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".txt";
		if (archivefiles)
			tmpfile = (archivedir / std::filesystem::path(tmpfile)).string();
		else
			tmpfile = (pathconf / std::filesystem::path(tmpfile)).string();
		auto start = std::chrono::system_clock::now(); 
		if (std::filesystem::exists(tmpfile.c_str()))
		{
			std::remove(tmpfile.c_str());
		}
		std::vector<std::string> command;
		std::string app = "";
		if (GetConfArg(PUTExec) != "")
			app = GetConfArg(PUTExec);
		else
			app = GetConfArg(PUT);
		if (PUTInputType == InputType::InputFile) {
			command = { GetConfArg(PUTFIXEDARGS_BEFORE), GetConfArg(PUTFILEARG) + input, " " + GetConfArg(PUTFIXEDARGS_AFTER), " &> " + tmpfile };
		}
		else if (PUTInputType == InputType::CmdArgs) {
			if (GetConfArg(PUTExec) != "")
				command.push_back(GetConfArg(PUT));
			// parse arguments from the input string
			std::vector<std::string> parsedcmd = SplitArguments(input);
			command.insert(command.end(), parsedcmd.begin(), parsedcmd.end());
		}
		logger::info("Interface", "GeneratePUTOutput", "Starting PUT with cmd: " + GetConfArg(PUTExec) + " " + Sum(command));
		ProgramOutput out = "";
		// execute
		// gives us, whether is was aborted due to time constraints and the exitcode
		auto [finished, ext] = fork_exec(app, command, PUTtimeout, tmpfile);

		// read output
		std::ifstream outfile(tmpfile);
		std::stringstream buff;
		buff << outfile.rdbuf();
		out = buff.str();
		auto end = std::chrono::system_clock::now();
		std::string tmp = GetConfArg(PUTExec) + " " + GetConfArg(PUT) + " " + Sum(command);
		if (!tmp.empty() && (tmp[tmp.length() - 1] == '\n' || tmp[tmp.length() - 1] == '\r'))
			tmp = tmp.substr(0, tmp.length() - 1);
		// save runtime
		times.push_back({ "PUT_Output_Generation", "PUTOGEN", finished, iteration, 0, start, end, (end - start).count(), tmp});
		if (!archivefiles && std::filesystem::exists(tmpfile.c_str()))
		{
			try
			{
				std::remove(tmpfile.c_str());
			}
			catch (std::exception&) {}
		}
		logger::info("Interface", "GeneratePUTOutput", "End PUT output generation");
		logger::info("Interface", "GeneratePUTOutput", "execution time: " + std::to_string((end - start).count() / 1000) + "us");
		return { finished, ext, out };
	}


	void Interface::AddNewInput(std::string input, bool passing, std::string output)
	{
		bool addinput = true;
		for (int i = 0; i < data->inputs.size(); i++)
		{
			if (input == data->inputs[i])
				return;
		}
		if (output == "")
		{
			auto [finished, ext, out] = GeneratePUTOutput(input);
			if (!finished)
			{
				addinput = false;
				logger::warn("Interface", "AddNewInput", "Couldn't generate PUT output for input");
				return;
			}
			if (GetConfArg(PUTVALIDEXITCODE) != "")
			{
				int val = 0;
				try
				{
					val = std::stoi(GetConfArg(PUTVALIDEXITCODE));
				}
				catch (std::exception)
				{
				}
				logger::info("Interface", "AddNewInput", "label: " + std::to_string(passing) + "\texitcode: " + std::to_string(ext) + "\texitcodesuccess: " + std::to_string(val));
				if (passing && ext != val || !passing && ext == val)
				{
					addinput = false;
					logger::warn("Interface", "AddNewInput", "PUT output does not match label");
					return;
				}
			}
			else if (GetConfArg(PUTFAILINGEXITCODE) != "")
			{
				int val = 1;
				try
				{
					val = std::stoi(GetConfArg(PUTFAILINGEXITCODE));
				}
				catch (std::exception)
				{
				}
				logger::info("Interface", "AddNewInput", "label: " + std::to_string(passing) + "\texitcode: " + std::to_string(ext) + "\texitcodefail: " + std::to_string(val));
				if (passing && ext == val || !passing && ext != val)
				{
					addinput = false;
					logger::warn("Interface", "AddNewInput", "PUT output does not match label");
					return;
				}
			}
			output = out;
		}
		if (addinput)
		{
			data->inputs.push_back(input);
			data->labels.push_back(passing);
			data->outputs.push_back(output);
			logger::info("Interface", "AddNewInput", "Added new PUT Input");
		}
	}

	void Interface::GenerateCmdArgs()
	{
		auto itr = stages.begin();
		while (itr != stages.end())
		{
			if (*itr != nullptr)
			{
				(*itr)->GenerateCmdArgs();
			}
			itr++;
		}
	}

	std::filesystem::path Interface::Stage_GetArchiveDir(std::string stage)
	{
		return archivedirfiles / ("iteration" + std::to_string(iteration)) / stage;
	}

	void Interface::Stage_SetCommandLineArgs(std::string stage, std::vector<std::string> args)
	{
		auto stg = FindStage(stage);
		if (stg)
		{
			stg->argsfixed = args;
			stg->argsMain.clear();
			stg->argsOpt.clear();
			stg->argsOverride = std::vector<std::string>{};
			logger::info("Interface", "Stage_SetCommandLineArgs " + stage, Sum(args));
			return;
		}
		logger::info("Interface", "Stage_SetCommandLineArgs " + stage, "failed to find stage");
	}

	void Interface::Stage_SetExecutable(std::string stage, std::string executable)
	{
		auto stg = FindStage(stage);
		if (stg) {
			stg->executable = executable;
			logger::info("Interface", "Stage_SetExecutable", "set executable: " + executable + " for stage: " + stage);
		} else
			logger::warn("Interface", "Stage_SetExecutable", "cannot set executable: " + executable + " for stage: " + stage);
	}

	void Interface::Stage_SetCommandLineArgs_Options(std::string stage, std::vector<std::string> args, std::vector<std::tuple<std::string, ArgsType>> optargs)
	{
		auto stg = FindStage(stage);
		if (stg)
		{
			stg->argsfixed = std::vector<std::string>{};
			stg->argsMain = args;
			stg->argsOpt = optargs;
			stg->argsOverride = std::vector<std::string>{};
			logger::info("Interface", "Stage_SetCommandLineArgs_Options " + stage, "args: " + Sum(args) + " opt: " + Sum(optargs));
			return;
		}
		logger::info("Interface", "Stage_SetCommandLineArgs_Options " + stage, "failed to find stage");
	}

	void Interface::AddConfArg(std::string key, std::string value)
	{
		confargs.insert_or_assign(key, value);
	}

	void Interface::DeleteConfArg(std::string key)
	{
		confargs.erase(key);
	}

	std::string Interface::GetConfArg(std::string key)
	{
		auto itr = confargs.find(key);
		if (itr != confargs.end())
		{
			return itr->second;
		}
		else
			return "";
	}

	std::string Interface::GetWorkDir(std::string stage)
	{
		return GetConfArg(WORKINGDIR + "_" + stage);
	}

	std::tuple<bool, ExitCode, ProgramOutput> Interface::Exec(std::string stage, std::vector<std::string> additionalargs)
	{
		// get stage data
		StageData* stg = FindStage(stage);
		if (stg == nullptr) {
			logger::info("Interface", "Exec " + stage, "Cannot find stage");
			return { false, 1, "" };
		}
		// create tmp file for program output
		logger::info("Interface", "Exec " + stage, "Start Program execution");
		std::string tmpfile;
		if (archivefiles)
			tmpfile = (archivedir / std::filesystem::path("RepBridOutputStage_" + stage + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".txt")).string();
		else
			tmpfile = (pathconf / std::filesystem::path("RepBridOutputStage_" + stage + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".txt")).string();
		stg->start = std::chrono::system_clock::now();
		if (std::filesystem::exists(tmpfile.c_str()))
		{
			std::remove(tmpfile.c_str());
		}
		auto xargs = stg->GetNextCmdArgsVec();
		std::vector<std::string> command;
		command.insert(command.end(), xargs.begin(), xargs.end());
		command.insert(command.end(), additionalargs.begin(), additionalargs.end());
		logger::info("Interface", "Exec", "Starting program with cmd: " + stg->executable + Sum(command));
		ProgramOutput out = "";
		// execute
		// gives us, whether is was aborted due to time constraints and the exitcode
		auto [finished, ext] = fork_exec(stg->executable, command, stg->timeout, tmpfile);

		logger::info("Interface", "Exec " + stage, "Finished Program execution");
		if (std::filesystem::exists(tmpfile.c_str()))
		{
			// read output
			std::ifstream outfile(tmpfile);
			std::stringstream buff;
			buff << outfile.rdbuf();
			out = buff.str();
			if (!archivefiles)
			{
				try
				{
					std::remove(tmpfile.c_str());
				}
				catch (std::exception&) {}
			}
		}
		logger::info("Interface", "Exec " + stage, "output handling");
		stg->end = std::chrono::system_clock::now();
		if (command.size() == 0)
		{
			command.push_back("");
		}
		// save runtime
		times.push_back({ stage, stg->executable, finished, iteration, stg->iterations, stg->start, stg->end, (stg->end - stg->start).count(), stg->executable + command[0] + Sum(additionalargs) });
		logger::info("Interface", "Exec " + stage, "times1");
		stg->times.push_back({ stage, stg->executable, finished, iteration, stg->iterations, stg->start, stg->end, (stg->end - stg->start).count(), stg->executable + command[0] + Sum(additionalargs) });
		logger::info("Interface", "Exec " + stage, "End Program Execution");
		logger::info("Interface", "Exec " + stage, "execution time: " + std::to_string((stg->end - stg->end).count() / 1000) + "us");
		return { finished, ext, out };
	}

	void Interface::Execute()
	{
		logger::info("Interface", "Execute", "Beginning execution");
		GenerateCmdArgs();
		logger::info("Interface", "Execute", "Generated Cmd Args for both stages");


		std::chrono::system_clock::time_point startiter = std::chrono::system_clock::now();
		// inc framewor iteration
		iteration++;

		// iterate over all stages in the order they were added
		std::pair<bool,std::string> retval;
		for (int i = 0; i < stages.size(); i++)
		{
			// if the stage is null, then abort since something went terribly wrong
			if (stages[i] == nullptr)
			{
				logger::error("Interface", "Execute", "Stage is NULL, Aborting Execution");
				return;
			}
			// go to working directory, before executing stage
			if (chdir(working_dir.c_str()) != 0)
				logger::warn("Interface", "Execute", "Couldn't reset working directory. May lead to unexpected behaviour");
			// run stage
			logger::info("Interface", "Execute", "Start cleaning of stage " + stages[i]->name);
			// increase number of times the stage was run
			stages[i]->iterations++;
			std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
			if (stages[i]->Clean != nullptr)
				stages[i]->Clean(this);
			logger::info("Interface", "Execute", "Start execution of stage " + stages[i]->name);
			retval = stages[i]->Run(this, this->data, stages[i]);
			// check stage results
			logger::info("Interface", "Execute", "Stage returned:\t" + std::to_string(std::get<0>(retval)) + "\tnext stage: " + std::get<1>(retval));
			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			// save runtime
			times.push_back({
				/*module*/			std::string("Execute"),
				/*executable*/		stages[i]->name,
				/*finished*/		std::get<0>(retval),
				/*majoriteration*/	iteration,
				/*minoriteration*/	-1,
				/*start*/			begin,
				/*end*/				end,
				/*time in ns*/		(uint64_t)((end - begin).count()),
				/*execcommand*/		""
				});
			if (std::get<0>(retval) == false)
			{
				logger::warn("Interface", "Execute", "Aborting execution");
				break;
			}
			if (std::get<1>(retval) != "")
			{
				// if the stage we just executed has told us which stage to execute next (for example returning to an earlier stage)
				// we find the stage and jump to the appropiate index
				int idx = FindStageIndex(std::get<1>(retval));
				if (idx != -1) {
					i = idx - 1; // dec by 1 since at end of loop we will increment i;
					logger::info("Interface", "Execute", "Jumping to stage " + std::get<1>(retval));
				}
				else
				{
					logger::info("Interface", "Execute", "Cannot jump to stage " + std::get<1>(retval) + ". Continuing execution normally.");
				}
			}
		}
		logger::info("Interface", "Execute", "Finished.");
		std::chrono::system_clock::time_point enditer = std::chrono::system_clock::now();
		// save runtime
		times.push_back({
			/*module*/			std::string("Execute"),
			/*executable*/		"",
			/*finished*/		std::get<0>(retval),
			/*majoriteration*/	iteration,
			/*minoriteration*/	-1,
			/*start*/			startiter,
			/*end*/				enditer,
			/*time in ns*/		(uint64_t)((enditer - startiter).count()),
			/*execcommand*/		""
			});
	}

	std::string Interface::FindExecutable(std::string StageName)
	{
		std::string exec = GetConfArg(EXECPATH + "_" + StageName);
		logger::info("Interface", "FindExecutable", "Key:" + EXECPATH + "_" + StageName + "|Value:" + exec);
		if (exec != "")
		{
			std::filesystem::path execpath(exec);
			if (execpath.is_absolute() && std::filesystem::exists(execpath))
			{
				if (GetConfArg(WORKINGDIR + "_" + StageName) == "")
					AddConfArg(WORKINGDIR + "_" + StageName, execpath.string() + "/");
				return execpath.string();
			}
			else if (execpath.is_relative())
			{
				if (std::filesystem::exists(pathself / execpath))
				{
					AddConfArg(EXECPATH + "_" + StageName, (pathself / execpath).string());
					if (GetConfArg(WORKINGDIR + "_" + StageName) == "")
						AddConfArg(WORKINGDIR + "_" + StageName, pathself.string() + "/");
					return (pathself / execpath).string();
				}
				else if (std::filesystem::exists(pathconf / execpath))
				{
					AddConfArg(EXECPATH + "_" + StageName, (pathconf / execpath).string());
					if (GetConfArg(WORKINGDIR + "_" + StageName) == "")
						AddConfArg(WORKINGDIR + "_" + StageName, pathconf.string() + "/");
					return (pathconf / execpath).string();
				}
				else
				{
					logger::error("Interface", "FindExecutable", "Executable for stage " + StageName + " couldn't be located.");
					exit(2);
				}
			}
			else
			{
				logger::error("Interface", "FindExecutable", "Executable for stage " + StageName + " couldn't be located.");
				exit(2);
			}
		}
		return "";
	}

	std::string Interface::FindFile (std::string key)
	{
		std::string exec = GetConfArg(key);
		if (exec != "")
		{
			std::filesystem::path execpath(exec);
			if (execpath.is_absolute() && std::filesystem::exists(execpath))
			{
				return exec;
			}
			else if (execpath.is_relative())
			{
				if (std::filesystem::exists(pathself / execpath))
				{
					return (pathself / execpath).string();
				}
				else if (std::filesystem::exists(pathconf / execpath))
				{
					return (pathconf / execpath).string();
				}
				else
				{
					logger::error("Interface", "FindFile", "File " + key + " couldn't be located.");
					return "";
				}
			}
			else
			{
				logger::error("Interface", "FindFile", "File " + key + " couldn't be located.");
				return "";
			}
		}
		return "";
	}
}