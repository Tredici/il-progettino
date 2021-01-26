#include "time_utils.h"
#include <errno.h>
#include <string.h>
#include <math.h>

struct tm time_date_init(int year, int month, int day)
{
    struct tm ans;

    memset(&ans, 0, sizeof(ans));
    /* inizializza i campi */
    ans.tm_year = year - 1900; /* man mktime */
    /* la convenzione seguita dalla libreria pone
     * gennaio come mese 0 */
    ans.tm_mon = month-1;
    ans.tm_mday = day; /* i giorni sono normali */
    /* normalizza il tutto */
    mktime(&ans);
    return ans;
}

int time_date_read(const struct tm* date, int* year, int* month, int* day)
{
    if (date == NULL)
        return -1;
    if (year != NULL)
        *year = 1900 + date->tm_year;
    if (month != NULL)
        *month = 1 + date->tm_mon;
    if (day != NULL)
        *day = date->tm_mday;

    return 0;
}

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

int time_date_diff(const struct tm* time1, const struct tm* time2)
{
    int ans;
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

    /* fa questo giochetto per evitare problemi con
     * leap second e simili */
    return (int)round((double)(t_a-t_b)/(24*60*60));
}
