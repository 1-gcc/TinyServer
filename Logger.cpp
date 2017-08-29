#include <stdio.h>
#include <dlfcn.h>
#include "Logger.h"

	Logger::~Logger()
	{
		if(lib)
			dlclose(lib);
		lib = NULL;

	}
	Logger::Logger()
	{
		flag = RTLD_NOW | RTLD_GLOBAL;
		libraryName = "./logger";
		lib = NULL;
		setTraceFile = NULL;
		trace = notrace;
	}
	bool Logger::load()
	{
		void * lib = dlopen(libraryName,flag);
		if(lib == NULL)
		{
			char* err = dlerror();
			fprintf(stderr,"dlopen error %s\n",err);
			return false;
		}
		setTraceFile = (char* (*)(char*)) dlsym(lib,"setTraceFile");
		if(setTraceFile == NULL)
		{
			char * err = dlerror();
			fprintf(stderr,"dlsym error %s\n",err);
			return false;
		}
		trace = (void (*)(char*,...))dlsym(lib,"trace");
		if(trace == NULL)
		{
			char* err = dlerror();
			fprintf(stderr,"dlsym error %s\n",err);
			return false;
		}
		setTraceFile("TraceFile_201708.trc");
		return true;

	}

