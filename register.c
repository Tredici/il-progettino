#include "register.h"
#include <time.h>

enum entry_type
{
    SWAB,       /* tampone */
    NEW_CASE    /* nuovo caso */
};

/** Una entry necessiterà di:
 *  tempo:
 *      +data
 *      ? +ora
 *  tipo:
 *      >tampone
 *
 */

struct entry
{
    struct tm e_time;
    enum entry_type type;
    int counter;
};
