/* This file contains definitions of universal configuration arguments
** Autor: Kai Münch*/

#pragma once

#ifndef H_CONFARGS
#define H_CONFARGS

#define EXECPATH std::string("ExecPath")
#define WORKINGDIR std::string("WorkDir")
#define OUTPUTDIR std::string("OutputDir")
#define ARCHIVE std::string("ArchiveDir")
#define DISABLEWORKINGFILEBACKUP std::string("DisableWorkingFileBackup")
#define ITERARIONS std::string("FrameworkIterations")
#define PUT std::string("ProgramUnderTest")
#define PUTExec std::string("ProgramUnderTest_Exec")
#define PUTINPUTTYPE std::string("ProgramUnderTest_InputType")
#define PUTCompileSH std::string("ProgramUnderTestCompileSH")
#define PUTFIXEDARGS_BEFORE std::string("ProgramUnderTest_FixedArguments_BeforeFile")
#define PUTFIXEDARGS_AFTER std::string("ProgramUnderTest_FixedArguments_AfterFile")
#define PUTFILEARG std::string("ProgramUnderTest_FileAgument")
#define PUTTIMEOUT std::string("ProgramUnderTest_TimeOut")
#define PUTVALIDEXITCODE std::string("ProgramUnderTest_ExitCodeOnSuccess")
#define PUTFAILINGEXITCODE std::string("ProgramUnderTest_ExitCodeOnFail")

#define STAGE std::string("Stage")

#endif