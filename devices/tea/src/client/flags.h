#ifndef FLAGS_H
#define FLAGS_H

#include "tea.h"
#include <getopt.h>

enum {
  __FLAG_SHIFT_STRICT,
  __FLAG_SHIFT_RELAXED,
  __FLAG_SHIFT_START,
  __FLAG_SHIFT_END,
  __FLAG_SHIFT_BASENAME,
}; /* max 32, since set_flags is an int */

#define FLAG_STRICT (1 << __FLAG_SHIFT_STRICT)
#define FLAG_HAS_ARG_STRICT no_argument
#define FLAG_LONG_STRICT "strict"
#define FLAG_SHORT_STRICT 'S'
#define FLAG_OPTSTR_STRICT "S"

#define FLAG_RELAXED (1 << __FLAG_SHIFT_RELAXED)
#define FLAG_HAS_ARG_RELAXED no_argument
#define FLAG_LONG_RELAXED "relaxed"
#define FLAG_SHORT_RELAXED 'R'
#define FLAG_OPTSTR_RELAXED "R"

#define FLAG_START (1 << __FLAG_SHIFT_START)
#define FLAG_HAS_ARG_START required_argument
#define FLAG_LONG_START "start"
#define FLAG_SHORT_START 'B'
#define FLAG_OPTSTR_START "B:"

#define FLAG_END (1 << __FLAG_SHIFT_END)
#define FLAG_HAS_ARG_END required_argument
#define FLAG_LONG_END "end"
#define FLAG_SHORT_END 'E'
#define FLAG_OPTSTR_END "E:"

#define FLAG_BASENAME (1 << __FLAG_SHIFT_BASENAME)
#define FLAG_HAS_ARG_BASENAME required_argument
#define FLAG_LONG_BASENAME "basename"
#define FLAG_SHORT_BASENAME 'N'
#define FLAG_OPTSTR_BASENAME "N:"

/* parse flag values into a tea_request struct.
 * returns the number of flags set or a negative errno */
int parse_flags(int argc, char *argv[], struct tea_request *req,
                int *set_flags);

#endif /* flags.h */
