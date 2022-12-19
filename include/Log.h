/* This file contains logging services
** Autor: Kai Münch*/

#pragma once

#ifndef H_LOGGER
#define H_LOGGER

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

#include "Interface.h"

#define logger Logging

/// <summary>
/// Provides Loggind services
/// </summary>
class Logging
{
private:
	/// <summary>
	/// output file stream
	/// </summary>
	static inline std::ofstream out;
	/// <summary>
	/// stores whether [out] has been initialized
	/// </summary>
	static inline bool init;
	/// <summary>
	/// stores start time of application
	/// </summary>
	static inline std::chrono::system_clock::time_point execstart;

	/// <summary>
	/// string that contains time and date of application start
	/// </summary>
	static inline std::string _timestring;

	/// <summary>
	/// returns the time passed since the start of the application
	/// </summary>
	/// <returns></returns>
	static std::string TimePassed()
	{
		std::stringstream ss;
		ss << std::setw(12) << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - execstart).count() << "ms";
		return ss.str();
	}

public:

	static std::string GetTimeString()
	{
		return _timestring;
	}

	/// <summary>
	/// writes informative logging information
	/// </summary>
	/// <param name="_module"></param>
	/// <param name="_function"></param>
	/// <param name="message"></param>
	static void info(std::string _module, std::string _function, std::string message)
	{
		if (init)
		{
			std::stringstream ss;
			ss << "[info]\t[" << Logging::TimePassed() << "]\t" << "[" << _module << "]\t" << "[" << _function << "]\t" << message << "\n";
			out << ss.str();
			out.flush();
			std::cout << ss.str();
		}
	}

	/// <summary>
	/// writes a warning
	/// </summary>
	/// <param name="_module"></param>
	/// <param name="_function"></param>
	/// <param name="message"></param>
	static void warn(std::string _module, std::string _function, std::string message)
	{
		if (init)
		{
			std::stringstream ss;
			ss << "[warn]\t[" << Logging::TimePassed() << "]\t" << "[" << _module << "]\t" << "[" << _function << "]\t" << message << "\n";
			out << ss.str();
			out.flush();
			std::cout << ss.str();
		}
	}

	/// <summary>
	/// writes an error
	/// </summary>
	/// <param name="_module"></param>
	/// <param name="_function"></param>
	/// <param name="message"></param>
	static void error(std::string _module, std::string _function, std::string message)
	{
		if (init)
		{
			std::stringstream ss;
			ss << "[error]\t[" << Logging::TimePassed() << "]\t" << "[" << _module << "]\t" << "[" << _function << "]\t" << message << "\n";
			out << ss.str();
			out.flush();
			std::cout << ss.str();
		}
	}

	/// <summary>
	/// initializes log file
	/// </summary>
	static void Initialize()
	{
		execstart = std::chrono::system_clock::now();
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
		// for more information about date/time format
		strftime(buf, sizeof(buf), "%Y-%m-%d.%H-%M-%S", &tstruct);
		std::stringstream ss;
		ss << "Log_" << buf;

		std::stringstream st;
		st << buf;
		_timestring = st.str();

		std::filesystem::path file = Bridge::Interface::GetSingleton()->pathconf / std::filesystem::path("log") / (ss.str() + ".log");
		try {
			std::filesystem::create_directories(Bridge::Interface::GetSingleton()->pathconf / std::filesystem::path("log"));
		}
		catch (std::filesystem::filesystem_error&)
		{

		}
		out = std::ofstream(file.string().c_str());
		init = true;
	}
};

#endif