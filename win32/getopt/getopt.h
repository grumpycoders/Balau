#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int getopt(int nargc, char ** nargv, char * ostr);

extern int opterr;
extern int optind;
extern int optopt;
extern char * optarg;

#ifdef __cplusplus
}
#endif
