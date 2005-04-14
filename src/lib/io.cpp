#include "lib/config.h"

#ifdef HAVE_MATLAB
#include "mex.h"
#endif

#include "lib/io.h"
#include "lib/common.h"
#include "lib/Time.h"
#include "lib/Mathmatics.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

FILE* CIO::target=stdout;
LONG CIO::last_progress_time=0 ;
LONG CIO::progress_start_time=0 ;
REAL CIO::last_progress=1 ;
EMessageType CIO::loglevel = M_ERROR;

CIO::CIO()
{
}

void CIO::message(EMessageType prio, const CHAR *fmt, ... )
{
#ifdef HAVE_MATLAB
	char str[4096];
    va_list list;
    va_start(list,fmt);
	vsnprintf(str, sizeof(str), fmt, list);
    va_end(list);

	check_target();
	switch (prio)
	{
		case M_DEBUG:
			fprintf(target, "[DEBUG] %s", str);
			break;
		case M_INFO:
			fprintf(target, "[INFO] %s", str);
			break;
		case M_NOTICE:
			fprintf(target, "[NOTICE] %s", str);
			break;
		case M_WARN:
			mexWarnMsgTxt(str);
			break;
		case M_ERROR:
		case M_CRITICAL:
		case M_ALERT:
		case M_EMERGENCY:
			mexErrMsgTxt(str);
			break;
		case M_MESSAGEONLY:
			fprintf(target, "%s", str);
			break;
		default:
			break;
	}
    fflush(target);
#else
	check_target();
	if (print_message_prio(prio, target))
	{
		va_list list;
		va_start(list,fmt);
		vfprintf(target,fmt,list);
		va_end(list);
		fflush(target);
	}
#endif
}

void CIO::buffered_message(EMessageType prio, const CHAR *fmt, ... )
{
	check_target();
	if (print_message_prio(prio, target))
	{
		va_list list;
		va_start(list,fmt);
		vfprintf(target,fmt,list);
		va_end(list);
	}
}

void CIO::progress(REAL current_val, REAL min_val, REAL max_val, INT decimals, const char* prefix)
{
	LONG runtime = CTime::get_runtime() ;
	
	char str[1000];
	REAL v=-1, estimate=0, total_estimate=0 ;
	check_target();

	if (max_val-min_val)
		v=100*(current_val-min_val+1)/(max_val-min_val+1);

	if (decimals < 1)
		decimals = 1;

	if (last_progress>v)
	{
		last_progress_time = runtime ;
		progress_start_time = runtime;
		last_progress = v ;
	}
	else
	{
		if (v>100) v=100.0 ;
		if (v<=0) v=1e-6 ;
		last_progress = v-1e-5 ; ;
		
		if ((v!=100.0) && (runtime - last_progress_time<100))
			return ;
		
		last_progress_time = runtime ;
		estimate = (1-v/100)*(last_progress_time-progress_start_time)/(v/100) ;
		total_estimate = (last_progress_time-progress_start_time)/(v/100) ;
	}
	
	if (estimate/100>120)
	{
		snprintf(str, sizeof(str), "%%s %%%d.%df%%%%    %%1.1f minutes remaining    %%1.1f minutes total    \r",decimals+3, decimals);
		message(M_MESSAGEONLY, str, prefix, v, (float)estimate/100/60, (float)total_estimate/100/60);
	}
	else
	{
		snprintf(str, sizeof(str), "%%s %%%d.%df%%%%    %%1.1f seconds remaining    %%1.1f seconds total    \r",decimals+3, decimals);
		message(M_MESSAGEONLY, str, prefix, v, (float)estimate/100, (float)total_estimate/100);
	}
	
    fflush(target);
}

CHAR* CIO::skip_spaces(CHAR* str)
{
	INT i=0;

	if (str)
	{
		for (i=0; isspace(str[i]); i++);

		return &str[i];
	}
	else 
		return str;
}

void CIO::set_loglevel(EMessageType level)
{
	loglevel=level;
}

void CIO::set_target(FILE* t)
{
	target=t;
}

void CIO::check_target()
{
	if (!target)
		target=stdout;
}

bool CIO::print_message_prio(EMessageType prio, FILE* target)
{
	const int num_levels=9;
	const EMessageType levels[num_levels]={M_MESSAGEONLY, M_DEBUG, M_INFO, M_NOTICE, M_WARN, M_ERROR, M_CRITICAL, M_ALERT, M_EMERGENCY};
	const char* strings[num_levels]={"[MESSAGEONLY]", "[DEBUG] ", "[INFO]", "[NOTICE]", "[WARN]", "[ERROR]", "[CRITICAL]", "[ALERT]", "[EMERGENCY]"};

	int i=0;
	int idx=0;
	bool output=false;

	while (i<num_levels && levels[i]!=loglevel)
	{
		if (levels[i]==loglevel)
			output=true;
		if (levels[i]==prio)
			idx=i;

		i++;
	}

	if (output)
		fprintf(target, strings[idx]);

	return output;
}
