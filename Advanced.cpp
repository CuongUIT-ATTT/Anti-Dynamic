#include <windows.h>
#include <intrin.h> // Bắt buộc để sử dụng __rdtsc, __cpuid, __readfsdword, và __readgsqword

// ============================================================================
// --- KỸ THUẬT BONUS NÂNG CAO: API HASHING (ẨN GIẤU IMPORT TABLE - IAT) ---
// ============================================================================

// Thuật toán băm chuỗi DJB2 dùng để chuyển đổi tên hàm thành mã băm số cố định
DWORD GetDJB2Hash(const char* str) {
    DWORD hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Định nghĩa mẫu con trỏ hàm (Function Prototype) cho hàm IsDebuggerPresent
typedef BOOL(WINAPI* pfnIsDebuggerPresent)();

// Trình phân giải địa chỉ API thủ công từ bảng Export Directory của DLL
FARPROC GetAPIAddressByHash(HMODULE hModule, DWORD targetHash) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return NULL;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY exportDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDir.VirtualAddress == 0) return NULL;

    PIMAGE_EXPORT_DIRECTORY pExportKey = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + exportDir.VirtualAddress);

    DWORD* names = (DWORD*)((BYTE*)hModule + pExportKey->AddressOfNames);
    DWORD* functions = (DWORD*)((BYTE*)hModule + pExportKey->AddressOfFunctions);
    WORD* ordinals = (WORD*)((BYTE*)hModule + pExportKey->AddressOfNameOrdinals);

    // Duyệt tìm hàm có mã băm trùng khớp trong thư viện DLL
    for (DWORD i = 0; i < pExportKey->NumberOfNames; i++) {
        char* functionName = (char*)((BYTE*)hModule + names[i]);
        if (GetDJB2Hash(functionName) == targetHash) {
            WORD ordinal = ordinals[i];
            return (FARPROC)((BYTE*)hModule + functions[ordinal]);
        }
    }
    return NULL;
}

// ============================================================================
// --- PAYLOAD CHƯƠNG TRÌNH (Yêu cầu 2.1) ---
// ============================================================================
void ExecutePayload() {
    LPCWSTR boxTitle = L"NT230 - Malware Payload Demo";
    LPCWSTR boxMessage = L"Thông điệp minh họa chương trình mã độc\n"
        L"Mã nhóm: G13 \n"
        L"Thành viên: Phạm Hùng Cường - Nguyễn Minh Quân - Phan Đức Anh - Trần Gia Bảo";

    MessageBoxW(NULL, boxMessage, boxTitle, MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// --- CÁC PHƯƠNG THỨC KIỂM TRA MÔI TRƯỜNG PHÂN TÍCH ĐỘNG (Yêu cầu 2.2 & 2.4) ---
// ============================================================================

// ANTI-DEBUGGING METHOD 1: Sử dụng Windows API thông qua API Hashing (Nâng cấp Advanced)
bool CheckDebuggerAPI() {
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) return false;

    // 0x41b3dfa4 là mã băm danh tính DJB2 của chuỗi ký tự "IsDebuggerPresent"
    DWORD hashIsDebuggerPresent = 0x41b3dfa4;

    // Gọi hàm động bằng con trỏ để xóa sạch dấu vết IsDebuggerPresent khỏi bảng IAT
    pfnIsDebuggerPresent DynamicIsDebuggerPresent = (pfnIsDebuggerPresent)GetAPIAddressByHash(hKernel32, hashIsDebuggerPresent);
    if (DynamicIsDebuggerPresent) {
        return DynamicIsDebuggerPresent() == TRUE;
    }
    return false;
}

// ANTI-DEBUGGING METHOD 2: Mức thấp thủ công sử dụng cấu trúc cấu trúc PEB
bool CheckDebuggerManualPEB() {
    unsigned char* pebAddress = nullptr;
#ifdef _WIN64
    // Cấu hình kiến trúc x64: Lấy PEB pointer từ đoạn mã GS tại offset 0x60
    pebAddress = (unsigned char*)__readgsqword(0x60);
#else
    // Cấu hình kiến trúc x86: Lấy PEB pointer từ đoạn mã FS tại offset 0x30
    pebAddress = (unsigned char*)__readfsdword(0x30);
#endif
    // Đọc flag BeingDebugged tại offset 0x02 của PEB
    return (*(pebAddress + 2) == 1);
}

// ANTI-VM METHOD 1: Artifact-based (Kiểm tra dữ liệu Registry hệ thống)
bool CheckVMRegistry() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR biosVersion[256] = { 0 };
        DWORD bufferSize = sizeof(biosVersion);

        if (RegQueryValueExW(hKey, L"SystemBiosVersion", NULL, NULL, (LPBYTE)biosVersion, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (wcsstr(biosVersion, L"VMware") ||
                wcsstr(biosVersion, L"VBOX") ||
                wcsstr(biosVersion, L"VIRTUAL") ||
                wcsstr(biosVersion, L"QEMU"))
            {
                return true; // Định vị được chuỗi máy ảo đặc trưng
            }
        }
        RegCloseKey(hKey);
    }
    return false;
}

// ANTI-VM METHOD 2: Hardware-based / Fingerprint (Kiểm tra số lượng nhân CPU)
bool CheckVMCPUCount() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    // Nếu số nhân CPU hệ thống nhận được < 4, khả năng cao là môi trường Sandbox/VM cấu hình thấp
    if (sysInfo.dwNumberOfProcessors < 4) {
        return true;
    }
    return false;
}

// SANDBOX DETECTION: Theo dõi hành vi tương tác dịch chuyển chuột của người dùng
bool CheckSandboxMouseMovement() {
    POINT pos1, pos2;

    // Đo tọa độ vị trí chuột lần 1
    if (!GetCursorPos(&pos1)) return false;

    // Tạm dừng thực thi hệ thống trong 2 giây để chờ đợi dịch chuyển
    Sleep(2000);

    // Đo tọa độ vị trí chuột lần 2
    if (!GetCursorPos(&pos2)) return false;

    // Nếu tọa độ không có bất kỳ sai lệch nào, xác định đây là môi trường mô phỏng tự động không có tương tác người dùng
    if (pos1.x == pos2.x && pos1.y == pos2.y) {
        return true; // Phát hiện Sandbox
    }

    return false; // Có dấu hiệu hoạt động của người dùng thật
}

// ============================================================================
// --- HÀM THỰC THI CHÍNH & LOGIC ĐIỀU KHIỂN (Yêu cầu 2.3) ---
// ============================================================================
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
) {
    // Logic tổng hợp: Chỉ cần dính bất kỳ cơ chế phát hiện nào, chương trình lập tức thoát ẩn
    if (CheckDebuggerAPI() ||
        CheckDebuggerManualPEB() ||
        CheckVMRegistry() ||
        CheckVMCPUCount() ||
        CheckSandboxMouseMovement())
    {
        // Trạng thái phát hiện môi trường giám sát -> Che giấu hoàn toàn hành vi, tự sát im lặng
        return 0;
    }

    // Vượt qua tất cả các chốt chặn an toàn (Môi trường sạch) -> Kích hoạt payload hiển thị thông tin nhóm
    ExecutePayload();

    return 0;
}