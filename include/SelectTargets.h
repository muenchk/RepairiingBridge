#pragma once

#ifndef H_SELECTTARGETS
#define H_SELECTTARGETS

#include <Log.h>
#include <Interface.h>
#include <Executables/Alhazen.h>
#include <Executables/GenProg.h>
#include <Executables/Compile.h>
#include <Executables/InputTests.h>
#include <Executables/OutputTests.h>
#include <Executables/OutputTimes.h>
#include <Executables/DynamicExec.h>

/// <summary>
/// Sets functions and values in the Interface, depending on [target]
/// </summary>
/// <param name="target"></param>
void SelectTargets(std::string target, int stageid)
{
	// target is the identifier of the stage to run
	// they may appear in a nonsenical order while executing this function
	// but that is the responsibility of the user
	// for instance: First genprog and then alhazen does not make sense

	if (target == "compile")
	{
		Compile::SetTargets(stageid);
	}
	else if (target == "alhazen")
	{
		Alhazen::SetTargets(stageid);
	}
	else if (target == "inputtests")
	{
		InputTests::SetTargets(stageid);
	}
	else if (target == "genprog")
	{
		GenProg::SetTargets(stageid);
	}
	else if (target == "outputtests")
	{
		OutputTests::SetTargets(stageid);
	}
	else if (target == "outputtimes")
	{
		OutputTimes::SetTargets(stageid);
	}
	else if (target == "dynamicexec")
	{
		Dynamic_Exec::SetTargets(stageid);
	}
	else
	{
		logger::error("Main", "SelectTargetsExec", "No valid target found. target selected: \"" + target + "\"");
		exit(3);
	}
}

#endif