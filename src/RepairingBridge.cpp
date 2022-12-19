// RepairingBridge.cpp : Defines the entry point for the application.
//

#include <RepairingBridge.h>
#include <Interface.h>
#include <filesystem>
#include <ConfArgs.h>
#include <Log.h>
#include <SelectTargets.h>
#include <unistd.h>

using namespace std;

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

int main(int argc, char* argv[])
{
	auto inter = Bridge::Interface::GetSingleton();
	std::string conf = "";
	int iterations = 1;
	std::string stage1exec = "";
	std::string stage2exec = "";
	// before we can start the logger we need to know where.
	// so we need the location of the configuration file, before we can actually parse it xD
	// so parse it twice i guess
	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]) == "-conf")
		{
			if (i + 1 < argc)
			{
				conf = std::string(argv[i + 1]);
				i++;
			}
		}
	}

	inter->pathconf = std::filesystem::canonical(std::filesystem::path(conf)).parent_path();
	// now actually start the program

	logger::Initialize();

	logger::info("Main", "Main", "Initializing RepairingBridge " + Bridge::Interface::Version());

	logger::info("Main", "CmdArgs", "Parsing command line args");
	// first extract the command line args
	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]) == "-conf")
		{
			if (i + 1 < argc)
			{
				conf = std::string(argv[i + 1]);
				i++;
				logger::info("Main", "CmdArgs", "Found command line argument conf: " + conf);
			}
			else
			{
				cout << "-conf: missing value\n";
				exit(2);
			}
		}
		if (std::string(argv[i]) == "--help")
		{
			cout << "-conf\t\tPath to the configuration file, that contains various parameters for execution\n";
			cout << "\tFor information on support configuration parameters see the program documentation\n";
			cout << "--help\t\tThis help dialogue\n";
		}
	}

	// parse conf
	{
		logger::info("Main", "CmdArgs", "Parsing configuration file");
		std::ifstream file(conf);
		std::string line;
		while (std::getline(file, line))
		{
			size_t pos = 0;
			if (line.empty())
				continue;
			if (line[0] == ';')
				continue;
			if ((pos = line.find('=')) != std::string::npos) {
				std::string key = line.substr(0, pos);
				std::string value = line.erase(0, pos + 1);
				if (value[value.length() - 1] == 13 || value[value.length() - 1] == 10)// enter key
					value = value.substr(0, value.length() - 1);
				inter->AddConfArg(key, value);
				logger::info("Main", "ConfArgs", "Parsed argument. Key: " + key + " Value: " + value);
			}
		}
		logger::info("Main", "CmdArgs", "Parsing pathes");
		auto p1 = std::filesystem::canonical(std::filesystem::path("/proc/self/exe"));
		inter->pathself = p1;
		auto p2 = std::filesystem::canonical(std::filesystem::path(conf)).parent_path();
		inter->pathconf = p2;

		// try to create the outputdir, if defined
		// if it fails delete the argument and just use the pathes in the working directories
		std::string odir = inter->GetConfArg(OUTPUTDIR);
		if (odir == "")
			odir = "output";
		if (odir != "")
		{
			if (std::filesystem::path(odir).is_relative())
				odir = (inter->pathconf / odir).string();
			// delete output
			try {
				std::filesystem::remove_all(std::filesystem::path(odir));
			}
			catch (std::filesystem::filesystem_error& e) {
				logger::warn("Main", "CmdArgs", "output folder couldn't be deleted");
			}
			// create output dir
			if (std::filesystem::create_directories(std::filesystem::path(odir)) == false)
			{
				inter->DeleteConfArg(OUTPUTDIR);
				logger::warn("Main", "CmdArgs", "output folder couldn't be created");
			}
		}
		// create tmp files archive
		std::string arch = inter->GetConfArg(ARCHIVE);
		if (arch != "")
		{
			inter->archivefiles = true;
			if (std::filesystem::path(arch).is_relative())
				inter->archivedir = (inter->pathconf / arch).string();
			else
				inter->archivedir = std::filesystem::path(arch);

			time_t     now = time(0);
			struct tm  tstruct;
			char       buf[80];
			tstruct = *localtime(&now);
			strftime(buf, sizeof(buf), "%Y-%m-%d.%H-%M-%S", &tstruct);
			std::stringstream ss;
			ss << buf;
			inter->archivedir = inter->archivedir / ss.str();

			// create directories
			try
			{
				std::filesystem::create_directories(inter->archivedir);
			}
			catch (std::filesystem::filesystem_error& e)
			{
				logger::error("Main", "CmdArgs", "Cannot create archive dir. Enabling tmp file deletion.");
				inter->archivefiles = false;
			}
			logger::info("Main", "CmdArgs", "ArchiveDir: " + inter->archivedir.string());
			inter->archivedirfiles = inter->archivedir / "files";
			logger::info("Main", "CmdArgs", "ArchiveDirFiles: " + inter->archivedirfiles.string());
		}
		if (ToLower(inter->GetConfArg(DISABLEWORKINGFILEBACKUP)) == "true")
		{
			inter->archiveworkingfiles = false;
		}

		// save working dir
		char buff[512];
		inter->working_dir = std::string(getcwd(buff, 512));
		chdir(inter->working_dir.c_str());
		logger::info("Main", "CmdArgs", "Finding PUT");

		// determine path to PUT
		inter->AddConfArg(PUT, inter->FindFile(PUT));
		inter->AddConfArg(PUTExec, inter->FindFile(PUTExec));

		// set PUT input type
		std::string itype = inter->GetConfArg(PUTINPUTTYPE);
		if (itype == "cmd")
			inter->SetPUTInputType(Bridge::InputType::CmdArgs);
		else if (itype == "file")
			inter->SetPUTInputType(Bridge::InputType::InputFile);

		logger::info("Main", "CmdArgs", "Getting PUT timeout");

		if (inter->GetConfArg(ITERARIONS) != "")
		{
			try {
				iterations = std::stoi(inter->GetConfArg(ITERARIONS));
			}
			catch (std::exception& e)
			{
				logger::error("Main", "CmdArgs", "Cannot convert setting framework iterations to valid integer");
				exit(2);
			}
		}
		if (iterations < 0)
		{
			logger::error("Main", "CmdArgs", "Cannot convert setting framework iterations to valid integer");
			exit(2);
		}
		if (iterations == 0)
		{
			logger::warn("Main", "CmdArgs", "Setting framework iterations is set to 0. This will disable any actual work, so consider setting it to a positive non-zero value.");
		}

		// get timeout for PUT output generation
		std::string tmp = inter->GetConfArg(PUTTIMEOUT);
		if (tmp != "")
		{
			try {
				inter->SetPUTTimeout(std::stoi(tmp));
			}
			catch (std::out_of_range&) {
				logger::warn("Main", "Main", "Cannot get timeout for PUT");
			}
			catch (std::invalid_argument&) {
				logger::warn("Main", "Main", "Cannot get timeout for PUT");
			}
		} else {
			inter->SetPUTTimeout(0);
		}
	}

	// before we start 

	// register the stages
	logger::info("Main", "Main", "Initializing Targets");
	std::string target = "";
	int x = 1;
	while ((target = inter->GetConfArg(STAGE + std::to_string(x))) != "")
	{
		SelectTargets(target, x);
		x++;
	}

	logger::info("Main", "Main", "Starting Execution...");
	// we verified that we have all pathes to executables
	// now we can begin the actual execution
	for (int i = 1; i <= iterations; i++)
	{
		// reset preexisting stages, this is necessary, since we do not want to carry over data by older stages
		//inter->ResetStages();

		// reset calculated inputs, to gain statistically independent execution data
		inter->ResetInputs();
		logger::info("Main", "Main", "Starting framework iteration " + std::to_string(i));
		inter->Execute();
	}

	inter->SaveOverallRuntime();

	logger::info("Main", "Main", "Writing time measurements");
	// this "stage" does not use [data] and [stage] parameters so we can set them to null
	OutputTimes::OutputTimes(inter, nullptr, nullptr);

	logger::info("Main", "Main", "Finished Execution");
}
