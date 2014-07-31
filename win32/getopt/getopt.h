#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int getopt(int nargc, char ** nargv, char * ostr);

#ifdef __cplusplus
}
#endif


extern int opterr = 1;
extern int optind = 1;
extern int optopt;
extern char * optarg;
