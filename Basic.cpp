#include <windows.h>
#include <intrin.h> // Bắt buộc để sử dụng các hàm nội tại như __rdtsc, __cpuid, __readgsqword, __readfsdword
#include <cwchar>   // Thư viện xử lý chuỗi ký tự rộng (wcsstr)

// ============================================================================
// 1. PAYLOAD PHỤC VỤ MINH HỌA (Yêu cầu 2.1)
// ============================================================================
/**
 * Hàm thực thi hành vi giả lập khi môi trường an toàn (không bị phân tích).
 * Trong bài thực hành này, hành vi là hiển thị một hộp thoại thông tin (MessageBox).
 */
void ExecutePayload() {
    LPCWSTR boxTitle = L"NT230 - Malware Payload Demo";
    LPCWSTR boxMessage = L"Thông điệp minh họa chương trình mã độc\n"
                         L"Mã nhóm: G13 \n"
                         L"Thành viên: Phạm Hùng Cường - Nguyễn Minh Quân - Phan Đức Anh - Trần Gia Bảo";

    // Hiển thị hộp thoại với biểu tượng thông tin (MB_ICONINFORMATION) và nút OK
    MessageBoxW(NULL, boxMessage, boxTitle, MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// 2. CÁC PHƯƠNG PHÁP CHỐNG GỠ LỖI (ANTI-DEBUGGING) (Yêu cầu 2.2)
// ============================================================================

/**
 * Cách 1: Sử dụng Win32 API tiêu chuẩn.
 * Hệ điều hành Windows cung cấp sẵn hàm IsDebuggerPresent để kiểm tra xem
 * tiến trình hiện tại có đang bị giám sát bởi một debugger (gỡ lỗi) hay không.
 */
bool CheckDebuggerAPI() {
    return IsDebuggerPresent() == TRUE;
}

/**
 * Cách 2: Truy cập thủ công vào cấu trúc PEB (Process Environment Block).
 * Thay vì gọi API (dễ bị thám mã phát hiện), ta đọc trực tiếp từ bộ nhớ hệ thống:
 * - Trên Windows 64-bit: PEB nằm ở offset 0x60 của thanh ghi GS.
 * - Trên Windows 32-bit: PEB nằm ở offset 0x30 của thanh ghi FS.
 * Cờ BeingDebugged nằm ở offset +2 tính từ gốc của PEB. Nếu cờ này bằng 1, nghĩa là đang bị debug.
 */
bool CheckDebuggerManualPEB() {
    unsigned char* pebAddress = nullptr;
#ifdef _WIN64
    // Đọc con trỏ PEB từ kiến trúc 64-bit
    pebAddress = (unsigned char*)__readgsqword(0x60);
#else
    // Đọc con trỏ PEB từ kiến trúc 32-bit
    pebAddress = (unsigned char*)__readfsdword(0x30);
#endif

    // Kiểm tra giá trị tại offset 2 (Cờ BeingDebugged)
    return (*(pebAddress + 2) == 1);
}

// ============================================================================
// 3. CÁC PHƯƠNG PHÁP PHÁT HIỆN MÁY ẢO (ANTI-VM)
// ============================================================================

/**
 * Cách 1: Kiểm tra Artifacts dựa trên Registry hệ thống.
 * Các phần mềm máy ảo như VMware, VirtualBox thường để lại thông tin nhận diện
 * trong thông tin cấu hình phần cứng BIOS lưu tại Registry.
 */
bool CheckVMRegistry() {
    HKEY hKey;
    // Mở khóa Registry chứa thông tin cấu hình BIOS
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR biosVersion[256] = { 0 };
        DWORD bufferSize = sizeof(biosVersion);

        // Đọc giá trị của khóa "SystemBiosVersion"
        if (RegQueryValueExW(hKey, L"SystemBiosVersion", NULL, NULL, (LPBYTE)biosVersion, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            // Tìm kiếm các chuỗi đặc trưng của các môi trường ảo hóa phổ biến
            if (wcsstr(biosVersion, L"VMware") ||
                wcsstr(biosVersion, L"VBOX") ||
                wcsstr(biosVersion, L"VIRTUAL") ||
                wcsstr(biosVersion, L"QEMU"))
            {
                return true; // Phát hiện máy ảo
            }
        }
        RegCloseKey(hKey);
    }
    return false; // Có thể là máy thật
}

/**
 * Cách 2: Kiểm tra dựa trên phần cứng (Số lượng CPU nhân xử lý).
 * Các hệ thống Sandbox tự động hoặc cấu hình máy ảo cơ bản thường chỉ cấp 1 đến 2 nhân CPU.
 * Máy tính cá nhân thông thường hiện nay hầu hết đều có từ 4 nhân trở lên.
 */
bool CheckVMCPUCount() {
    SYSTEM_INFO sysInfo;
    // Lấy thông tin phần cứng của hệ thống hiện tại
    GetSystemInfo(&sysInfo);

    // Nếu số nhân CPU < 4, nghi ngờ đây là môi trường máy ảo phân tích (Lab)
    if (sysInfo.dwNumberOfProcessors < 4) {
        return true;
    }
    return false;
}

// ============================================================================
// 4. PHÁT HIỆN MÔI TRƯỜNG SANDBOX (Yêu cầu 2.4)
// ============================================================================

/**
 * Kiểm tra sự tương tác của người dùng thông qua hành vi di chuyển chuột.
 * Các Sandbox tự động thường chạy mẫu kiểm tra trong thời gian ngắn và không có
 * hành vi di chuyển chuột tự nhiên giống con người.
 */
bool CheckSandboxMouseMovement() {
    POINT pos1, pos2;

    // Lấy tọa độ con trỏ chuột lần thứ nhất
    if (!GetCursorPos(&pos1)) return false;

    // Tạm dừng chương trình 2000 mili-giây (2 giây) để chờ đợi tương tác thực tế từ người dùng
    Sleep(2000);

    // Lấy tọa độ con trỏ chuột lần thứ hai sau khoảng thời gian chờ
    if (!GetCursorPos(&pos2)) return false;

    // So sánh tọa độ giữa 2 lần lấy dữ liệu
    // Nếu cả trục X và Y hoàn toàn không đổi, nghi ngờ đây là môi trường Sandbox chạy tự động
    if (pos1.x == pos2.x && pos1.y == pos2.y) {
        return true; // Phát hiện Sandbox không có người tương tác
    }

    return false; // Môi trường có dấu hiệu tương tác thật
}

// ============================================================================
// 5. HÀM THỰC THI CHÍNH & LOGIC ĐIỀU KHIỂN (Yêu cầu 2.3)
// ============================================================================
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
) {
    // Tích hợp tất cả các cơ chế kiểm tra môi trường:
    // 1. Kiểm tra debugger bằng API và PEB thủ công
    // 2. Kiểm tra máy ảo bằng Registry và số lượng nhân CPU
    // 3. Kiểm tra Sandbox bằng hành vi di chuyển chuột
    if (CheckDebuggerAPI()       || 
        CheckDebuggerManualPEB() || 
        CheckVMRegistry()        || 
        CheckVMCPUCount()        || 
        CheckSandboxMouseMovement()) 
    {
        // Nếu bất kỳ cơ chế kiểm tra nào trả về 'true' (phát hiện môi trường phân tích)
        // Chương trình lập tức kết thúc im lặng (Thoát mà không để lại dấu vết hành vi nguy hiểm)
        return 0;
    }

    // Nếu vượt qua tất cả các bài kiểm tra (môi trường an toàn/máy thật) -> Kích hoạt payload chính
    ExecutePayload();

    return 0;
}