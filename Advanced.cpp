#include <windows.h>
#include <intrin.h>
#include <cwchar>
#include <shlobj.h>
#include <stdio.h>

// ============================================================================
// DEBUG HELPER
// ============================================================================
void DebugPrint(const char *format, ...)
{
    char buffer[512] = {0};
    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, sizeof(buffer), format, args);
    va_end(args);

    OutputDebugStringA(buffer);
    printf("%s\n", buffer);
}

// ============================================================================
// PAYLOAD
// ============================================================================
void ExecutePayload()
{
    DebugPrint("[+] ExecutePayload() called - Payload is running!");

    LPCWSTR boxTitle = L"NT230 - Malware Payload Demo";
    LPCWSTR boxMessage = L"Thông điệp minh họa chương trình mã độc\n"
                         L"Mã nhóm: G13 \n"
                         L"Thành viên: Phạm Hùng Cường - Nguyễn Minh Quân - Phan Đức Anh - Trần Gia Bảo\n\n"
                         L"Environmental Keying: Payload đã chạy thành công!";

    MessageBoxW(NULL, boxMessage, boxTitle, MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// ENVIRONMENTAL KEYING
// ============================================================================
bool CheckEnvironmentalKey()
{
    DebugPrint("[*] Checking Environmental Key...");

    char computerName[256] = {0};
    char userName[256] = {0};
    DWORD size = sizeof(computerName);

    GetComputerNameA(computerName, &size);
    size = sizeof(userName);
    GetUserNameA(userName, &size);

    DWORD volumeSerial = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &volumeSerial, NULL, NULL, NULL, 0);

    DebugPrint("[+] Computer Name : %s", computerName);
    DebugPrint("[+] Username      : %s", userName);
    DebugPrint("[+] Volume Serial : 0x%X", volumeSerial);

    // ================== THAY ĐỔI ĐIỀU KIỆN Ở ĐÂY ==================
    if (strstr(userName, "Phạm") || strstr(userName, "Admin") ||
        strstr(userName, "User") || strstr(computerName, "DESKTOP") ||
        strstr(computerName, "LAPTOP") || volumeSerial != 0)
    { // Cho phép hầu hết máy thật

        DebugPrint("[+] Environmental Key MATCHED → Allow payload");
        return true;
    }

    DebugPrint("[-] Environmental Key NOT MATCH → Blocking");
    return false;
}

// ============================================================================
// ANTI-DEBUG & ANTI-VM
// ============================================================================
bool CheckDebuggerAPI()
{
    bool detected = IsDebuggerPresent() == TRUE;
    DebugPrint("[*] CheckDebuggerAPI: %s", detected ? "DETECTED" : "OK");
    return detected;
}

bool CheckDebuggerManualPEB()
{
    unsigned char *peb = nullptr;
#ifdef _WIN64
    peb = (unsigned char *)__readgsqword(0x60);
#else
    peb = (unsigned char *)__readfsdword(0x30);
#endif
    bool detected = (*(peb + 2) == 1);
    DebugPrint("[*] CheckDebuggerManualPEB: %s", detected ? "DETECTED" : "OK");
    return detected;
}

bool CheckVMRegistry()
{
    HKEY hKey;
    bool detected = false;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        WCHAR biosVersion[256] = {0};
        DWORD bufferSize = sizeof(biosVersion);
        if (RegQueryValueExW(hKey, L"SystemBiosVersion", NULL, NULL, (LPBYTE)biosVersion, &bufferSize) == ERROR_SUCCESS)
        {
            if (wcsstr(biosVersion, L"VMware") || wcsstr(biosVersion, L"VBOX") ||
                wcsstr(biosVersion, L"VIRTUAL") || wcsstr(biosVersion, L"QEMU"))
            {
                detected = true;
            }
        }
        RegCloseKey(hKey);
    }
    DebugPrint("[*] CheckVMRegistry: %s", detected ? "DETECTED" : "OK");
    return detected;
}

bool CheckVMCPUCount()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    bool detected = (si.dwNumberOfProcessors < 4);
    DebugPrint("[*] CheckVMCPUCount: %d cores → %s", si.dwNumberOfProcessors, detected ? "DETECTED" : "OK");
    return detected;
}

bool CheckSandboxMouseMovement()
{
    POINT p1, p2;
    GetCursorPos(&p1);
    Sleep(1500);
    GetCursorPos(&p2);
    bool detected = (p1.x == p2.x && p1.y == p2.y);
    DebugPrint("[*] CheckSandboxMouseMovement: %s", detected ? "DETECTED (No movement)" : "OK (User moved mouse)");
    return detected;
}

// ============================================================================
// MAIN
// ============================================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    AllocConsole();
    freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);

    DebugPrint("[+] Loader Started - NT230 Group 13");

    if (CheckDebuggerAPI() || CheckDebuggerManualPEB() ||
        CheckVMRegistry() || CheckVMCPUCount() || CheckSandboxMouseMovement())
    {
        DebugPrint("[-] Analysis environment detected! Exiting silently.");
        goto wait_for_exit;
    }

    if (!CheckEnvironmentalKey())
    {
        DebugPrint("[-] Environmental Key check failed! Blocking payload.");
        goto wait_for_exit;
    }

    DebugPrint("[+] All checks passed! Running payload...");
    ExecutePayload();

    DebugPrint("[+] Loader finished successfully.");

wait_for_exit:
    DebugPrint("\n=== Nhấn ENTER để thoát chương trình ===");
    getchar();
    FreeConsole(); // Tắt console khi thoát
    return 0;
}