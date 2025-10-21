#ifndef CAFE_ENUM_H
#define CAFE_ENUM_H

#include "cafe_magic.h"

/* enum definition */
#define __CAFE_DEF_ENUM(enum_name, ...) enum enum_name { __VA_ARGS__ };

/* enum value to string func definition */
#define __CAFE_DEF_ENUM_STR_CASE(val)                                          \
  case val:                                                                    \
    return #val;

#define __CAFE_DEF_ENUM_STR(enum_name, ...)                                    \
  static inline const char *enum_name##_str(enum enum_name value) {            \
    switch (value) {                                                           \
      EVAL(MAP(__CAFE_DEF_ENUM_STR_CASE, __VA_ARGS__))                         \
    default:                                                                   \
      return "CAFE_" #enum_name "_UNKNOWN";                                    \
    }                                                                          \
  }

/* define an enum and a stringification function
 * use like:
 * __CAFE_ENUM(my_enum,
 *   MY_ENUM_VAL_1,
 *   MY_ENUM_VAL_2,
 *   ...
 * )
 */
#define __CAFE_ENUM(enum_name, ...)                                            \
  __CAFE_DEF_ENUM(enum_name, __VA_ARGS__)                                      \
  __CAFE_DEF_ENUM_STR(enum_name, __VA_ARGS__)

#endif /* CAFE_ENUM_H */
