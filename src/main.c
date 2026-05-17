#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "beacon.h"
#include "common.h"

SCAN_SETTINGS ParseSettings(char* strTargets, char* strPorts) {
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

    // Parse ports
    count = 1;
    for (p = strPorts; *p; p++) if (*p == ',') count++;

    settings.ports = (int*)MemAlloc(count * sizeof(int));
    char* portsCopy = (char*)MemAlloc(MSVCRT$strlen(strPorts) + 1);
    MSVCRT$strcpy(portsCopy, strPorts);

    i = 0;
    token = MSVCRT$strtok(portsCopy, ",");
    while (token && i < count) {
        settings.ports[i++] = MSVCRT$atoi(token);
        token = MSVCRT$strtok(NULL, ",");
    }
    settings.numPorts = i;
    MemFree(portsCopy);

    return settings;
}

SCAN_RESULT PortScan(SCAN_SETTINGS settings, HANDLE hStop) {
    SCAN_RESULT result = {0};

    // Initialize Winsock
    WSADATA wsaData;
    if (WS2_32$WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        BeaconPrintf(CALLBACK_ERROR, "[-] WSAStartup failed: %d\n", WS2_32$WSAGetLastError());
        return result;
    }

    // Total connections to attempt = targets * ports
    int total = settings.numTargets * settings.numPorts;
    int queued = 0;
    int targetIndex = 0, portIndex = 0;
    int active = 0;

    // Allocate per-host result 
    // Each host gets an open port array with the size of settings.numPorts, as the maximum number of open ports is the number of ports scanned
    result.hosts = (HOST_RESULT*)MemAlloc(settings.numTargets * sizeof(HOST_RESULT));
    for (int h = 0; h < settings.numTargets; h++)
        result.hosts[h].openPorts = (int*)MemAlloc(settings.numPorts * sizeof(int));

    SCAN_ENTRY* entries = (SCAN_ENTRY*)MemAlloc(settings.maxConn * sizeof(SCAN_ENTRY));
    WSAPOLLFD* fds = (WSAPOLLFD*)MemAlloc(settings.maxConn * sizeof(WSAPOLLFD));
    int* entryOf = (int*)MemAlloc(settings.maxConn * sizeof(int));

    // Initialize entries as free
    for (int i = 0; i < settings.maxConn; i++)
        entries[i].sock = INVALID_SOCKET;

    // Notify about the first target before entering the scan loop
    BeaconPrintf(CALLBACK_OUTPUT, "[*] Scanning %s...\n", settings.targets[0]);

    while (queued < total || active > 0) {
        // Handle stop event
        if (KERNEL32$WaitForSingleObjectEx(hStop, 0, FALSE) == WAIT_OBJECT_0)
            break;

        // Populate free entries
        for (int s = 0; s < settings.maxConn && queued < total && active < settings.maxConn; s++) {
            if (entries[s].sock != INVALID_SOCKET) continue;

            // Capture current target index before it advances
            int curTargetIdx = targetIndex;
            char* target = settings.targets[curTargetIdx];
            int port = settings.ports[portIndex];

            portIndex++;
            if (portIndex >= settings.numPorts) {
                // Move to next target when all ports have been scanned on the current one
                portIndex = 0;
                targetIndex++;
                if (targetIndex < settings.numTargets) {
                    BeaconPrintf(CALLBACK_OUTPUT, "[*] Scanning %s...\n", settings.targets[targetIndex]);
                }
            }
            queued++;

            // Resolve hostname/IP to a sockaddr
            ADDRINFOA hints = {0};
            PADDRINFOA res = NULL;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            if (WS2_32$getaddrinfo(target, NULL, &hints, &res) != 0)
                continue;

            // Patch in port
            ((struct sockaddr_in*)res->ai_addr)->sin_port = WS2_32$htons((u_short)port);

            // Create socket
            SOCKET sock = WS2_32$socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                WS2_32$freeaddrinfo(res);
                continue;
            }

            // Connect
            u_long nb = 1;
            WS2_32$ioctlsocket(sock, FIONBIO, &nb);
            WS2_32$connect(sock, res->ai_addr, (int)res->ai_addrlen);
            WS2_32$freeaddrinfo(res);

            // Store socket in the free slot
            entries[s].sock = sock;
            entries[s].target = target;
            entries[s].port = port;
            entries[s].targetIndex = curTargetIdx;
            active++;
        }

        if (active == 0) break;

        int nfds = 0;
        for (int s = 0; s < settings.maxConn; s++) {
            if (entries[s].sock == INVALID_SOCKET) continue;
            fds[nfds].fd = entries[s].sock;
            fds[nfds].events = POLLWRNORM;
            fds[nfds].revents = 0;
            entryOf[nfds++] = s;
        }

        // Wait up to timeout ms for any socket to become ready
        if (WS2_32$WSAPoll(fds, nfds, settings.timeout) <= 0)
            continue;

        // Process results
        for (int i = 0; i < nfds; i++) {
            if (!fds[i].revents)
                continue;

            int s = entryOf[i];
            if (fds[i].revents & POLLWRNORM) {
                int err = 0, errLen = sizeof(err);
                WS2_32$getsockopt(entries[s].sock, SOL_SOCKET, SO_ERROR, (char*)&err, &errLen);

                if (err == 0) {
                    // Port open 
                    HOST_RESULT* host = &result.hosts[entries[s].targetIndex];
                    host->openPorts[host->open++] = entries[s].port;

                    // Print open ports as they are found when verbose mode is enabled
                    if (settings.verbose) {
                        BeaconPrintf(CALLBACK_OUTPUT, "[+] %s:%d open\n", entries[s].target, entries[s].port);
                        BeaconWakeup();
                    }
                } else {
                    // Port closed
                    result.hosts[entries[s].targetIndex].closed++;
                }
            }

            // Free the entry for the next connection
            WS2_32$closesocket(entries[s].sock);
            entries[s].sock = INVALID_SOCKET;
            active--;
        }
    }

    // Cleanup open sockets
    for (int s = 0; s < settings.maxConn; s++)
        if (entries[s].sock != INVALID_SOCKET)
            WS2_32$closesocket(entries[s].sock);

    MemFree(entryOf);
    MemFree(fds);
    MemFree(entries);
    WS2_32$WSACleanup();

    return result;
}

VOID PrintScanSummary(SCAN_SETTINGS settings, SCAN_RESULT result){    
    for (int h = 0; h < settings.numTargets; h++) {
        HOST_RESULT* host = &result.hosts[h];
        BeaconPrintf(CALLBACK_OUTPUT, "[*] Scan result for %s (%d open | %d closed):\n", settings.targets[h], host->open, host->closed);
        if (host->open == 0) 
            continue;
        
        for (int p = 0; p < host->open; p++){
            BeaconPrintf(CALLBACK_OUTPUT, "  - %d\n", host->openPorts[p]);
        }   
        BeaconPrintf(CALLBACK_OUTPUT, "\n"); 
    }
        
    for (int h = 0; h < settings.numTargets; h++)
        MemFree(result.hosts[h].openPorts);
    MemFree(result.hosts);
}

VOID go(char* args, int argc) {

    datap parser;
    HANDLE hStop = BeaconGetStopJobEvent();

    int lenTargets = 0;
    int lenPorts = 0;

    BeaconDataParse(&parser, args, argc);
    char* strTargets = BeaconDataExtract(&parser, &lenTargets);
    char* strPorts = BeaconDataExtract(&parser, &lenPorts);

    // Parse settings
    SCAN_SETTINGS settings = ParseSettings(strTargets, strPorts);
    settings.timeout = BeaconDataInt(&parser);
    settings.maxConn = BeaconDataInt(&parser);
    settings.verbose = BeaconDataInt(&parser);

    BeaconPrintf(CALLBACK_OUTPUT, "[*] Port scan started:\n");
    BeaconPrintf(CALLBACK_OUTPUT, "  - Targets to scan: %d\n", settings.numTargets);
    BeaconPrintf(CALLBACK_OUTPUT, "  - Ports to scan:   %d\n", settings.numPorts);
    BeaconPrintf(CALLBACK_OUTPUT, "  - Max connections: %d\n", settings.maxConn);
    BeaconPrintf(CALLBACK_OUTPUT, "  - Timeout:         %dms\n", settings.timeout);
    BeaconPrintf(CALLBACK_OUTPUT, "  - Verbose:         %s\n\n", settings.verbose == 1 ? "true": "false");
    BeaconWakeup(); 

    // Start scan
    SCAN_RESULT result = PortScan(settings, hStop);

    // Print scan results
    BeaconPrintf(CALLBACK_OUTPUT, "[*] Port scan completed.\n\n");
    PrintScanSummary(settings, result);
    
    BeaconPrintf(CALLBACK_OUTPUT, "[+] BOF execution completed.\n");
    return;
}
