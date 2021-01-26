#include "time_utils.h"
#include <errno.h>
#include <string.h>

int time_copy_date(struct tm* dst, const struct tm* src)
{
    if (dst == NULL || src == NULL)
        return -1;

    memset(dst, 0, sizeof(struct tm));
    dst->tm_yday = src->tm_yday;
    dst->tm_mon = src->tm_mon;
    dst->tm_mday = src->tm_mday;

    return 0;
}

int time_date_cmp(const struct tm* time1, const struct tm* time2)
{
    struct tm a, b;
    time_t t_a, t_b;

    if (time1 == NULL || time2 == NULL)
    {
        errno = EINVAL;
        return 0;
    }

    time_copy_date(&a, time1);
    time_copy_date(&b, time2);

    t_a = mktime(&a); t_b = mktime(&b);

    if (t_a < t_b)
        return -1;
    else if (t_a == t_b)
        return 0;
    else
        return 1;
}
