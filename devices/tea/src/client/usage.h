#ifndef USAGE_H
#define USAGE_H

/* char *name is the name of the executable (argv[0]) */

/* display general usage information */
void ask_for_help(const char *name);
void usage(const char *name);

/* usage for specific commands */
void usage_dump(const char *name);
void usage_get(const char *name);
void usage_help(const char *name);

#endif /* usage.h */
