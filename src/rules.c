#include "rules.h"
#include <string.h>

const char *RULE_NAMES[AUTO_COUNT] = {
    "Conway Life",  "HighLife",     "Seeds",
    "Day & Night",  "Maze",         "Anneal",
    "Brian Brain",  "Wireworld",    "Langton Ant",
    "Cyclic CA",    "Elementary",   "Custom",
};

const char *RULE_DESC[AUTO_COUNT] = {
    "B3/S23 Classic",    "B36/S23 Replicator", "B2/S0 Explosive",
    "B3678/S34678",      "B3/S12345",           "B4678/S35678",
    "3-state firing",    "4-state electron",    "Highway builder",
    "N-state cycling",   "1D Wolfram rules",    "Edit B/S rules",
};

void rule_get_bs_string(char *buf, int bufsize) {
    char tmp[32];
    int pos = 0;
    tmp[pos++] = 'B';
    for (int i = 0; i <= 8; i++)
        if ((g_sim.birth_mask >> i) & 1)
            tmp[pos++] = (char)('0' + i);
    tmp[pos++] = '/';
    tmp[pos++] = 'S';
    for (int i = 0; i <= 8; i++)
        if ((g_sim.surv_mask >> i) & 1)
            tmp[pos++] = (char)('0' + i);
    tmp[pos] = '\0';
    /* safe copy */
    int n = (int)strlen(tmp);
    if (n >= bufsize) n = bufsize - 1;
    memcpy(buf, tmp, (size_t)n);
    buf[n] = '\0';
}