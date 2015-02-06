#ifndef log_h
#define log_h

#define log_error(...) fprintf(stderr,##__VA_ARGS__)
#define log_info(...) fprintf(stdout,##__VA_ARGS__)
#define log_debug(...) fprintf(stdout,##__VA_ARGS__)

#endif

