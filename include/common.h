#ifndef COMMON_H
#define COMMON_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define MemAlloc(size) KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define MemFree(ptr)   KERNEL32$HeapFree(KERNEL32$GetProcessHeap(), 0, (ptr))

// KERNEL32
DECLSPEC_IMPORT HANDLE  WINAPI KERNEL32$GetProcessHeap(VOID);
DECLSPEC_IMPORT LPVOID  WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);
DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$GetLastError(VOID);
DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$WaitForSingleObjectEx(HANDLE, DWORD, BOOL);

// MSVCRT
DECLSPEC_IMPORT size_t  WINAPI MSVCRT$strlen(const char*);
DECLSPEC_IMPORT char*   WINAPI MSVCRT$strcpy(char*, const char*);
DECLSPEC_IMPORT char*   WINAPI MSVCRT$strtok(char*, const char*);
DECLSPEC_IMPORT int     WINAPI MSVCRT$atoi(const char*);
DECLSPEC_IMPORT int     WINAPI MSVCRT$_snprintf(char*, size_t, const char*, ...);

// WS2_32
DECLSPEC_IMPORT int     WINAPI WS2_32$WSAStartup(WORD, LPWSADATA);
DECLSPEC_IMPORT int     WINAPI WS2_32$WSACleanup(void);
DECLSPEC_IMPORT SOCKET  WINAPI WS2_32$socket(int, int, int);
DECLSPEC_IMPORT int     WINAPI WS2_32$closesocket(SOCKET);
DECLSPEC_IMPORT int     WINAPI WS2_32$connect(SOCKET, const struct sockaddr*, int);
DECLSPEC_IMPORT int     WINAPI WS2_32$ioctlsocket(SOCKET, long, u_long*);
DECLSPEC_IMPORT int     WINAPI WS2_32$WSAPoll(LPWSAPOLLFD, ULONG, INT);
DECLSPEC_IMPORT int     WINAPI WS2_32$WSAGetLastError(void);
DECLSPEC_IMPORT u_short WINAPI WS2_32$htons(u_short);
DECLSPEC_IMPORT int     WINAPI WS2_32$getaddrinfo(PCSTR, PCSTR, const ADDRINFOA*, PADDRINFOA*);
DECLSPEC_IMPORT void    WINAPI WS2_32$freeaddrinfo(PADDRINFOA);
DECLSPEC_IMPORT int     WINAPI WS2_32$getsockopt(SOCKET, int, int, char*, int*);

// IPHLPAPI
typedef ULONG IPAddr;

typedef struct {
    UCHAR  Ttl;
    UCHAR  Tos;
    UCHAR  Flags;
    UCHAR  OptionsSize;
    PUCHAR OptionsData;
} IP_OPTION_INFORMATION;

typedef struct {
    IPAddr Address;
    ULONG  Status;
    ULONG  RoundTripTime;
    USHORT DataSize;
    USHORT Reserved;
    PVOID  Data;
    IP_OPTION_INFORMATION Options;
} ICMP_ECHO_REPLY;

#define ICMP_ECHO_DATA  "abcdefghijklmnopqrstuvwabcdefghi"
#define ICMP_REPLY_SIZE (sizeof(ICMP_ECHO_REPLY) + sizeof(ICMP_ECHO_DATA))

DECLSPEC_IMPORT HANDLE WINAPI IPHLPAPI$IcmpCreateFile(VOID);
DECLSPEC_IMPORT DWORD  WINAPI IPHLPAPI$IcmpSendEcho(HANDLE, IPAddr, LPVOID, WORD, IP_OPTION_INFORMATION*, LPVOID, DWORD, DWORD);
DECLSPEC_IMPORT BOOL   WINAPI IPHLPAPI$IcmpCloseHandle(HANDLE);

typedef struct {
    char **targets;
    int numTargets;
    int timeout;
    int verbose;
} SCAN_SETTINGS;

#endif