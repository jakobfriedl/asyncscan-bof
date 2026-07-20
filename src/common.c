#include "common.h"

SCAN_SETTINGS ParseTargets(char* strTargets) {
    SCAN_SETTINGS settings = {0};
    char *p, *token;
    int i, count;

    // Parse targets
    count = 1;
    for (p = strTargets; *p; p++) if (*p == ',') count++;

    settings.targets = (char**)MemAlloc(count * sizeof(char*));
    char* targetsCopy = (char*)MemAlloc(MSVCRT$strlen(strTargets) + 1);
    MSVCRT$strcpy(targetsCopy, strTargets);

    i = 0;
    token = MSVCRT$strtok(targetsCopy, ",");
    while (token && i < count) {
        int len = MSVCRT$strlen(token) + 1;
        settings.targets[i] = (char*)MemAlloc(len);
        MSVCRT$strcpy(settings.targets[i++], token);
        token = MSVCRT$strtok(NULL, ",");
    }
    settings.numTargets = i;
    MemFree(targetsCopy);

    return settings;
}