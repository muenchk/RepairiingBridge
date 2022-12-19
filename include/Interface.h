#pragma once

#ifndef H_INTERFACE
#define H_INTERFACE

#include <cstdint>
#include <string>
#include <functional>
#include <InputData.h>
#include <tuple>
#include <map>
#include <chrono>
#include <ConfArgs.h>
#include <filesystem>

typedef int ExitCode;
typedef std::string ProgramOutput;

namespace Bridge
{
	class Interface;

	enum ArgsType
	{
		/// <summary>
		/// A single argument is a parameter for a program that stands alone and does not relate to any other parameters that
		/// need to be used for successful program execution.
		/// example: 
		///		program execution: "ls -l -a"
		///		"-l" is a single parameter since it does not have any dependencies on other parameters
		/// </summary>
		Single = 0,
		/// <summary>
		/// A combined argument is a string that consists of blank separated parameters. They are split before executing the program in question.
		///	If there are blanks that need to remain, for instance in filepathes make sure to enclose the parameter in quotation marks.
		/// example:
		///		program execution: "alhazen -s 0 bug.py work/"
		///		"bug.py", "work/" are single parameters
		///		"-s 0" is a combined argument consisting of the parameters "-s" and "0" that are placement dependend and existence depended.
		/// </summary>
		Combined = 1,
	};

	enum InputType
	{
		InputFile = 0,
		CmdArgs = 1,
	};

	class StageData
	{
	private:
		// vector with all possible cmd argument combinations for execution
		std::vector<std::vector<int>> argslist;
		// index of the next argument
		int argsidx = 0;

	public:

		// delegate for functions that execute stages
		typedef std::function<std::pair<bool, std::string>(Interface*, InputData*, StageData*)> RunStage;
		// delegate for cleaning up stage fragments
		typedef std::function<void(Interface*)> CleanStage;

		RunStage Run = nullptr;
		CleanStage Clean = nullptr;

		// start time for execution of the stage
		std::chrono::system_clock::time_point start;
		// end time for execution of the stage
		std::chrono::system_clock::time_point end;

		// number of times the stage was executed
		int iterations = 0;

		// unique name of the stage
		std::string name;

		// executable of stage
		std::string executable;

		// fixed command line args (overrides those below)
		std::vector<std::string> argsfixed;
		// main cmd args for the stage
		std::vector<std::string> argsMain;
		// optional cmd args for the stage
		std::vector<std::tuple<std::string, ArgsType>> argsOpt;
		// overriding arguments, used on next execution
		std::vector<std::string> argsOverride;

		// timeout for the stage execution
		int timeout = 0;

		// <Module, executable, finished (not aborted), majoriteratio, minoriteraton, start, end, duration in nanoseconds, execcommand> for this stage only
		// Module = stagename -> execution inside a stage runner
		// Module = Modulename -> time of major function or function block
		// executable = file -> inside a stage the executed external call
		// executable = stage name -> function that calls a specific stage, outside a runner function
		// executable = "" -> main function
		// majoriteration = iteration of framework
		// minoriteration = iterations of stage -> -1 for frameworkfunctions
		std::vector<std::tuple<std::string, std::string, bool, int/*major iteration*/, int /*minor iteration*/, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point, uint64_t, std::string>> times;

		/// <summary>
		/// Computes the Cmd args for the next iteration.
		/// </summary>
		/// <returns></returns>
		std::vector<std::string> GetNextCmdArgsVec();

		/// <summary>
		/// Generates the cmd args for all programs
		/// </summary>
		void GenerateCmdArgs();

		/// <summary>
		/// Returns how many more Iterations are possible based on the argumentslist
		/// </summary>
		/// <returns></returns>
		int ArgumentsUnused()
		{
			return (int)argslist.size() - argsidx;
		}

		/// <summary>
		/// Returns how many different argument combinations are present
		/// </summary>
		/// <returns></returns>
		int ArgumentsTotal()
		{
			return (int)argslist.size();
		}
	};

	class Interface 
	{
	private:
		/// <summary>
		/// timepoint the object is created and the program has been started
		/// </summary>
		std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();

		/// <summary>
		/// Generates the output for each input of the program under test
		/// </summary>
		/// <param name="input">either the cmd args for the PUT or the file name of the input file for the PUT, depending on PUTInputType</param>
		/// <returns>aborted?, exitcode, stdout</returns>
		std::tuple<bool, int, std::string> GeneratePUTOutput(std::string input);

		// whether stage needs PUT outputs
		bool PUTOutputNeeded = false;
		// the timeout for PUT output generation
		int PUTtimeout = 0;
		// input type of the PUT
		InputType PUTInputType = InputType::CmdArgs;

		// stores inputs and outputs of PUT
		InputData* data = new InputData();

		// maps key, value pairs of arguments
		std::map<std::string, std::string> confargs;

		// maps stageident and stagedata for stages
		std::vector<StageData*> stages;

		// start time for execution of program
		std::chrono::system_clock::time_point exestart = std::chrono::system_clock::now();

		// <stage, executable, finished (not aborted), iteration, start, end, duration in nanoseconds, execcommand>
		std::vector<std::tuple<std::string, std::string, bool, int/*major iteration*/, int /*minor iteration*/, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point, uint64_t, std::string>> times;

		/// <summary>
		/// number of iterations the framework was run
		/// </summary>
		int iteration = 0;

		StageData* FindStage(std::string stage)
		{
			for (int i = 0; i < stages.size(); i++)
			{
				if (stages[i] && stages[i]->name == stage)
					return stages[i];
			}
			return nullptr;
		}

		int FindStageIndex(std::string stage)
		{
			for (int i = 0; i < stages.size(); i++)
			{
				if (stages[i] && stages[i]->name == stage)
					return i;
			}
			return -1;
		}

		/// <summary>
		/// Generates the cmd args for all programs
		/// </summary>
		void GenerateCmdArgs();

	public:
		/// <summary>
		/// returns the current framework iteration
		/// </summary>
		/// <returns></returns>
		int GetIteration()
		{
			return iteration;
		}
		/// <summary>
		/// Returns the version of the class implementing the interface
		/// </summary>
		/// <returns></returns>
		static std::string Version() { return "1-0-0"; }

		static Interface* GetSingleton() {
			static Interface* inter = new Interface();
			return inter;
		}

		/// <summary>
		/// Rwturns the directory stage working files should be archived in
		/// </summary>
		/// <param name="stage"></param>
		/// <returns></returns>
		std::filesystem::path Stage_GetArchiveDir(std::string stage);

		void Stage_Create(std::string stage, std::string executable = "", std::vector<std::string> argsfixed = std::vector<std::string>{}, std::vector<std::string>* argsMain = nullptr, std::vector<std::tuple<std::string, ArgsType>>* argsOpt = nullptr, int timeout = 0, StageData::RunStage runner = nullptr)
		{
			StageData* stg = new StageData();
			stg->name = stage;
			stg->argsfixed = argsfixed;
			stg->argsOverride = std::vector<std::string>{};
			if (argsMain != nullptr)
				stg->argsMain = *argsMain;
			if (argsOpt != nullptr)
				stg->argsOpt = *argsOpt;
			stg->timeout = timeout;
			stg->Run = runner;
			stg->executable = executable;
			stages.push_back(stg);
		}

		/// <summary>
		/// Sets the executable for a stage
		/// </summary>
		/// <param name="stage"></param>
		/// <param name="executable"></param>
		void Stage_SetExecutable(std::string stage, std::string executable);

		/// <summary>
		/// Sets one string as the command line args
		/// </summary>
		void Stage_SetCommandLineArgs(std::string stage, std::vector<std::string> args);

		/// <summary>
		/// Sets a set of obligatory command line args [args] and a set of alternating command line args [optargs].
		/// The [optargs] are used randomly when the normal [args] do not produce a result.
		/// </summary>
		void Stage_SetCommandLineArgs_Options(std::string stage, std::vector<std::string> args, std::vector<std::tuple<std::string, ArgsType>> optargs);

		/// <summary>
		/// Sets the function delegate for the execution of the second program
		/// </summary>
		void Stage_SetRunner(std::string stage, StageData::RunStage runner)
		{
			auto stg = FindStage(stage);
			if (stg)
				stg->Run = runner;
		}

		/// <summary>
		/// Sets a function delegate that is executed before the stage and cleans previous runs
		/// </summary>
		void Stage_SetCleaner(std::string stage, StageData::CleanStage cleaner)
		{
			auto stg = FindStage(stage);
			if (stg)
				stg->Clean = cleaner;
		}

		/// <summary>
		/// Sets the timeout for a stage in seconds
		/// </summary>
		/// <param name="seconds"></param>
		void Stage_SetTimeout(std::string stage, int seconds)
		{
			auto stg = FindStage(stage);
			if (stg)
				stg->timeout = seconds;
		}

		/// <summary>
		/// Fixes the command line args for a stage to the given string. This function sets the command line args, which are used for 
		/// the generation of PUT outputs. If this is not used the standard command line args are used.
		/// </summary>
		/// <param name=""></param>
		void Stage_FixArguments(std::string stage, std::vector<std::string> args)
		{
			auto stg = FindStage(stage);
			if (stg)
				stg->argsOverride = args;
		}

		/// <summary>
		/// Unsets the command line arguments override for a stage and instead the standard args will be used.
		/// </summary>
		void Stage_UnfixArguments(std::string stage)
		{
			auto stg = FindStage(stage);
			if (stg)
				stg->argsOverride = std::vector<std::string>{};
		}


		/// <summary>
		/// Sets whether the stage exec needs the output of the program under test
		/// </summary>
		/// <param name="needsoutput"></param>
		void SetPUTOutputNeeded(bool needsoutput)
		{
			PUTOutputNeeded = true;
		}

		/// <summary>
		/// Sets the timeout for PUT output generation
		/// </summary>
		/// <param name="seconds"></param>
		void SetPUTTimeout(int seconds)
		{
			PUTtimeout = seconds;
		}

		/// <summary>
		/// Sets the input type of the PUT
		/// </summary>
		/// <param name="type"></param>
		void SetPUTInputType(InputType type)
		{
			PUTInputType = type;
		}

		/// <summary>
		/// Adds a new program under test input generated to the list of inputs and whether it is a passing input. Optionally adds an associated PUT output.
		/// If no output is given and exec2 needs the output, it is generated on the spot.
		/// </summary>
		void AddNewInput(std::string input, bool passing, std::string output = "");


		/// <summary>
		/// Execute the Pipeline
		/// </summary>
		void Execute();



		/// <summary>
		/// Runs a stage with the specified command line args in addition to [additionalargs]
		/// </summary>
		/// <param name="additionalargs"></param>
		/// <returns>finished execution before timeout?, ExitCode and program output</returns>
		std::tuple<bool, ExitCode, ProgramOutput> Exec(std::string stage, std::vector<std::string> additionalargs);



		/// <summary>
		/// Adds a configuration [key] [value] pair
		/// </summary>
		void AddConfArg(std::string key, std::string value);
		/// <summary>
		/// Deletes the argument with [key]
		/// </summary>
		/// <param name="key"></param>
		void DeleteConfArg(std::string key);
		/// <summary>
		/// Returns the value of the configuration argument for [key], otherwise empty string
		/// </summary>
		/// <returns></returns>
		std::string GetConfArg(std::string key);

		/// <summary>
		/// Returns the working directory for a stage
		/// </summary>
		/// <returns></returns>
		std::string GetWorkDir(std::string stage);

		// path to configuration file
		std::filesystem::path pathconf;
		// file to executable
		std::filesystem::path pathself;
		// working dir at the beginning of program execution
		std::string working_dir;
		// temporary files archive
		std::filesystem::path archivedir;
		// working files archive
		std::filesystem::path archivedirfiles;
		// whether to save temporary outputs
		bool archivefiles = false;
		bool archiveworkingfiles = true;

		/// <summary>
		/// determines the path to an executable defined in the configuration argument [StageName]
		/// </summary>
		/// <param name="ArgumentName">identifier of the argument holding the executable to find</param>
		/// <returns>path to the executable</returns>
		std::string FindExecutable(std::string StageName);

		/// <summary>
		/// determines the path to a file defined in the configuration argument [key]
		/// </summary>
		/// <param name="ArgumentName">identifier of the argument holding the file to find</param>
		/// <returns>path to the file</returns>
		std::string FindFile(std::string key);

		/// <summary>
		/// Sets the executable for a stage
		/// </summary>
		void SetExec(std::string stage, std::string executable)
		{
			auto stg = FindStage(stage);
			if (stg)
				stg->executable = executable;
		}

		/// <summary>
		/// Resets all inputs calculated or read by the framwork
		/// </summary>
		void ResetInputs()
		{
			data->inputs.clear();
			data->labels.clear();
			data->outputs.clear();
		}

		/// <summary>
		/// Resets all stages currently loaded
		/// </summary>
		void ResetStages()
		{
			for (int i = 0; i < stages.size(); i++)
			{
				delete stages[i];
			}
			stages.clear();
		}

		void SaveOverallRuntime()
		{

			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			times.push_back({
				/*module*/			std::string("Program"),
				/*executable*/		"Execution Timepoint",
				/*finished*/		"",
				/*majoriteration*/	0,
				/*minoriteration*/	0,
				/*start*/			begin,
				/*end*/				end,
				/*time in ns*/		(uint64_t)((end - begin).count()),
				/*execcommand*/		""
				});
		}

		/// <summary>
		/// Returns the execution time information for all iterations of executable 1 and executable 2
		/// </summary>
		std::vector<std::tuple<std::string, std::string, bool, int, int, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point, uint64_t, std::string>> GetRuntimes() { return times; }


	};
}

#endif