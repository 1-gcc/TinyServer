#define LOGGER_H



class Logger
{
protected:
	int flag ;
	char * libraryName ;
	void * lib ;
	static void notrace(char * format,...)
	{
	}
	char * (*setTraceFile)(char*file);
public:
	Logger();
	virtual ~Logger();
	bool load();
	void (*trace)(char*format,...) ;
};
