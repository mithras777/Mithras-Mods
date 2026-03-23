#pragma once

#define MAKE_STR_HELPER(a_str) #a_str
#define MAKE_STR(a_str) MAKE_STR_HELPER(a_str)

#define VERSION_MAJOR                       1
#define VERSION_MINOR                       0
#define VERSION_REVISION                    0
#define VERSION_BUILD                       0
#define VERSION_STR                         MAKE_STR(VERSION_MAJOR) "." MAKE_STR(VERSION_MINOR) "." MAKE_STR(VERSION_REVISION) "." MAKE_STR(VERSION_BUILD)
#define VERSION_PRODUCTNAME_DESCRIPTION_STR "AssassinationSKSE" " v" VERSION_STR

#define VERSION_PRODUCTNAME_STR             "AssassinationSKSE"
#define VERSION_YEAR_STR                    "(C) 2025"
#define VERSION_AUTHOR_STR                  "Mithras666"
#define VERSION_LICENSE_STR                 "MIT"
#define VERSION_COPYRIGHT_STR               VERSION_YEAR_STR " " VERSION_AUTHOR_STR " " VERSION_LICENSE_STR
