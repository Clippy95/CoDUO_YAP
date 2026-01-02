#include "framework.h"
#include "loader/component_loader.h"
#include <windows.h>
#include <wincrypt.h>

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#pragma comment(lib, "advapi32.lib")
namespace steam_check {
    auto cod_uo_sp_steam = "ce69464ffa38ad84e1d4b5e4e25202b741446018";
    auto cod_uo_mp_steam = "ee876688e132cfc8092500e6dca09eafe477f90b";
    std::string GetExecutableHash()
    {
        // Get the path of the main executable (not the DLL)
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH); // NULL = main .exe

        // Open the file
        HANDLE hFile = CreateFileA(exePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            return ""; // File not found
        }

        // Get file size
        DWORD fileSize = GetFileSize(hFile, NULL);
        if (fileSize == INVALID_FILE_SIZE)
        {
            CloseHandle(hFile);
            return "";
        }

        // Read file into memory
        std::vector<BYTE> fileData(fileSize);
        DWORD bytesRead;
        if (!ReadFile(hFile, fileData.data(), fileSize, &bytesRead, NULL))
        {
            CloseHandle(hFile);
            return "";
        }
        CloseHandle(hFile);

        // Create hash
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;

        if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            return "";

        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return "";
        }

        if (!CryptHashData(hHash, fileData.data(), fileSize, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        // Get hash value
        BYTE hash[20]; // SHA1 = 20 bytes
        DWORD hashLen = 20;
        if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        // Convert to hex string
        std::string hashStr;
        char hexBuffer[3];
        for (DWORD i = 0; i < hashLen; i++)
        {
            sprintf_s(hexBuffer, "%02x", hash[i]);
            hashStr += hexBuffer;
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        return hashStr;
    }

    class component final : public component_interface
    {
    public:
        void post_start() override
        {
            std::string exehash = GetExecutableHash();
            if (!exehash.empty())
            {
                if (exehash == cod_uo_mp_steam || exehash == cod_uo_sp_steam)
                {
                    int result = MessageBoxA(NULL,
                        "Steam executables detected!\n\n"
                        "CoDUO_QOL is incompatible with the Steam release and requires the unpacked latest executables.\n\n"
                        "Would you like to run the patcher now?",
                        "CoDUO_QOL - Incompatible Executable",
                        MB_YESNO | MB_ICONWARNING);

                    if (result == IDYES)
                    {

                        char exePath[MAX_PATH];
                        GetModuleFileNameA(NULL, exePath, MAX_PATH);


                        char* lastSlash = strrchr(exePath, '\\');
                        if (lastSlash) *lastSlash = '\0';


                        char batchPath[MAX_PATH];
                        sprintf_s(batchPath, "%s\\patch_coduo.bat", exePath);

                        // Check if batch file exists
                        if (GetFileAttributesA(batchPath) == INVALID_FILE_ATTRIBUTES)
                        {
                            MessageBoxA(NULL,
                                "patch_coduo.bat not found!\n\n"
                                "Please ensure the patcher is in the same directory as the game executable.",
                                "CoDUO_QOL - Error",
                                MB_OK | MB_ICONERROR);
                            ExitProcess(1);
                        }


                        char cmdLine[MAX_PATH * 2];
                        sprintf_s(cmdLine, "cmd.exe /c \"%s\"", batchPath);

                        STARTUPINFOA si = { sizeof(si) };
                        PROCESS_INFORMATION pi = { 0 };

                        if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                            CREATE_NEW_CONSOLE, NULL, exePath, &si, &pi))
                        {
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                        else
                        {
                            char errorMsg[512];
                            sprintf_s(errorMsg,
                                "Failed to launch patcher (Error: %d)\n\n"
                                "Batch file: %s\n\n"
                                "Please run patch_coduo.bat manually.",
                                GetLastError(), batchPath);
                            MessageBoxA(NULL, errorMsg, "CoDUO_QOL - Error", MB_OK | MB_ICONERROR);
                        }
                    }

                    ExitProcess(0);
                }
            }
        }
    };
}
REGISTER_COMPONENT(steam_check::component);