#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "beacon.h"
#include "common.h"
#include "common.c"

typedef struct {
    int* alive;
    int numAlive;
} SWEEP_RESULT;

SWEEP_RESULT PingSweep(SCAN_SETTINGS settings, HANDLE hStop) {
    SWEEP_RESULT result = {0};

    // Initialize Winsock
    WSADATA wsaData;
    if (WS2_32$WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        BeaconPrintf(CALLBACK_ERROR, "[-] WSAStartup failed: %d\n", WS2_32$WSAGetLastError());
        return result;
    }

    // Allocate per-host result
    result.alive = (int*)MemAlloc(settings.numTargets * sizeof(int));

    // Open ICMP handle
    HANDLE hIcmp = IPHLPAPI$IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        BeaconPrintf(CALLBACK_ERROR, "[-] Failed to open ICMP handle: %d\n", KERNEL32$GetLastError());
        WS2_32$WSACleanup();
        return result;
    }

    // Match default ping command options
    IP_OPTION_INFORMATION opts = {0};
    opts.Ttl = 128;

    char replyBuf[ICMP_REPLY_SIZE];

    for (int i = 0; i < settings.numTargets; i++) {
        if (KERNEL32$WaitForSingleObjectEx(hStop, 0, FALSE) == WAIT_OBJECT_0)
            break;

        // Resolve hostname/IP to a sockaddr
        ADDRINFOA hints = {0};
        PADDRINFOA res = NULL;
        hints.ai_family = AF_INET;
        if (WS2_32$getaddrinfo(settings.targets[i], NULL, &hints, &res) != 0)
            continue;

        IPAddr ip = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
        WS2_32$freeaddrinfo(res);

        DWORD ret = IPHLPAPI$IcmpSendEcho(
            hIcmp, ip,
            (LPVOID)ICMP_ECHO_DATA, sizeof(ICMP_ECHO_DATA) - 1,
            &opts, replyBuf, sizeof(replyBuf),
            (DWORD)settings.timeout
        );

        if (ret != 0) {
            result.alive[i] = 1;
            result.numAlive++;

            if (settings.verbose) {
                BeaconPrintf(CALLBACK_OUTPUT, "[+] %s\n is alive", settings.targets[i]);
                BeaconWakeup();
            }
        }
    }

    IPHLPAPI$IcmpCloseHandle(hIcmp);
    WS2_32$WSACleanup();

    return result;
}

VOID PrintSweepSummary(SCAN_SETTINGS settings, SWEEP_RESULT result) {
    BeaconPrintf(CALLBACK_OUTPUT, "[*] %d/%d hosts alive\n", result.numAlive, settings.numTargets);
    for (int h = 0; h < settings.numTargets; h++) {
        if (result.alive[h])
            BeaconPrintf(CALLBACK_OUTPUT, "  - %s\n", settings.targets[h]);
    }
    MemFree(result.alive);
}

VOID go(char* args, int argc) {
    datap parser;
    HANDLE hStop = BeaconGetStopJobEvent();
    
    BeaconDataParse(&parser, args, argc);
    int lenTargets = 0;
    char* strTargets = BeaconDataExtract(&parser, &lenTargets);

    // Parse settings
    SCAN_SETTINGS settings = ParseTargets(strTargets);
    settings.timeout = BeaconDataInt(&parser);
    settings.verbose = BeaconDataInt(&parser);

    BeaconPrintf(CALLBACK_OUTPUT, "[*] Ping sweep started:\n");
    BeaconPrintf(CALLBACK_OUTPUT, "  - Targets to scan: %d\n", settings.numTargets);
    BeaconPrintf(CALLBACK_OUTPUT, "  - Timeout:         %dms\n", settings.timeout);
    BeaconPrintf(CALLBACK_OUTPUT, "  - Verbose:         %s\n\n", settings.verbose == 1 ? "true" : "false");
    BeaconWakeup();

    // Start scan
    SWEEP_RESULT result = PingSweep(settings, hStop);

    // Print scan results
    BeaconPrintf(CALLBACK_OUTPUT, "[*] Ping sweep completed.\n\n");
    PrintSweepSummary(settings, result);
    BeaconPrintf(CALLBACK_OUTPUT, "\n[+] BOF execution completed.\n");
    BeaconWakeup();
    return;
}
