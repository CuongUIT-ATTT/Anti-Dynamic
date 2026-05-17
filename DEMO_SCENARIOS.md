# Demo Scenarios - Advanced.cpp

## 5 Kịch Bản Demo Ngắn Gọn

---

## Scenario 1: Clean Environment (Môi Trường Sạch)

### Requirements

- Máy thật Windows (≥ 4 CPU cores)
- Không có debugger attached
- Advanced.exe compiled
- Mouse ready (để lắc)

### Steps

1. Mở cmd → `systeminfo | findstr /C:"Processor"`
2. Mở cmd → `whoami` (xem username)
3. Mở cmd → `hostname` (xem computer name)
4. Mở Visual Studio
5. Nhấn **Ctrl+F5** (Start Without Debugging)
6. Khi chương trình chạy → **Lắc chuột liên tục 2 giây**
7. Quan sát console output → Nhấn Enter khi xong

### Expected Result

```
[+] Loader Started - NT230 Group 13
[*] Checking Environmental Key...
[+] Computer Name : DESKTOP-xxxxx
[+] Username      : admin
[+] Volume Serial : 0xxxxxxxxx
[+] Environmental Key MATCHED → Allow payload
[*] CheckDebuggerAPI: OK
[*] CheckDebuggerManualPEB: OK
[*] CheckVMRegistry: OK
[*] CheckVMCPUCount: 8 cores → OK
[*] CheckSandboxMouseMovement: OK (User moved mouse)
[+] All checks passed! Running payload...
✅ MessageBox hiển thị (NT230 - Payload Demo)
```

---

## Scenario 2: Debugger Detection (Phát Hiện Debugger)

### Requirements

- Advanced.cpp opened trong Visual Studio
- Không cần debugger trước tiên

### Steps

1. Nhấn **F5** (Start Debugging)
2. Để chương trình chạy trong debug mode
3. Quan sát console output

### Expected Result

```
[+] Loader Started - NT230 Group 13
[*] Checking Environmental Key...
[+] Computer Name : DESKTOP-xxxxx
[+] Username      : admin
[+] Volume Serial : 0xxxxxxxxx
[+] Environmental Key MATCHED → Allow payload
[*] CheckDebuggerAPI: DETECTED ← KEY!
[-] Analysis environment detected! Exiting silently.
❌ NO MessageBox (payload blocked)
```

---

## Scenario 3: VM Detection (Phát Hiện VM)

### Requirements

- VMware hoặc VirtualBox VM (< 4 cores)
- Windows 10/11 inside VM
- Advanced.exe copied to VM

### Steps

1. SSH/RDP vào VM hoặc chạy desktop VM
2. Inside VM: Mở cmd → `systeminfo | findstr /C:"Processor"` (check cores)
3. Inside VM: Double-click Advanced.exe hoặc Ctrl+F5
4. Lắc chuột nếu cần
5. Quan sát console output

### Expected Result

```
[+] Loader Started - NT230 Group 13
[*] Checking Environmental Key...
[+] Computer Name : VM-name
[+] Username      : user
[+] Volume Serial : 0xxxxxxxxx
[+] Environmental Key MATCHED → Allow payload
[*] CheckDebuggerAPI: OK
[*] CheckDebuggerManualPEB: OK
[*] CheckVMRegistry: DETECTED ← KEY! (hoặc CPU count)
❌ NO MessageBox (payload blocked)
```

---

## Scenario 4: Sandbox Detection (Phát Hiện Sandbox)

### Requirements

- Máy thật Windows (≥ 4 cores)
- Timer/stopwatch (để track 1.5 giây)

### Steps (Version A - WITHOUT mouse movement)

1. Nhấn **Ctrl+F5**
2. **Ngay lập tức: KHÔNG DI CHUYỂN CHUỘT** - giữ tuyệt đối im trong 1.5 giây
3. Quan sát console output

### Expected Result

```
[+] Loader Started - NT230 Group 13
[*] Checking Environmental Key...
[+] Computer Name : DESKTOP-xxxxx
[+] Username      : admin
[+] Volume Serial : 0xxxxxxxxx
[+] Environmental Key MATCHED → Allow payload
[*] CheckDebuggerAPI: OK
[*] CheckDebuggerManualPEB: OK
[*] CheckVMRegistry: OK
[*] CheckVMCPUCount: 8 cores → OK
[*] CheckSandboxMouseMovement: DETECTED (No movement) ← KEY!
❌ NO MessageBox (payload blocked)
```

### Steps (Version B - WITH mouse movement, optional comparison)

1. Nhấn **Ctrl+F5** lần 2
2. **Ngay lập tức: LẮC CHUỘT LIÊN TỤC** - di chuyển nhanh trong 1.5+ giây
3. Quan sát console output

### Expected Result (Version B)

```
...same as Scenario 1...
[*] CheckSandboxMouseMovement: OK (User moved mouse)
[+] All checks passed! Running payload...
✅ MessageBox hiển thị
```

---

## Scenario 5: Environmental Keying (Kiểm tra Env Keying)

### Requirements

- Máy thật Windows
- Registry Editor hoặc cmd
- Khả năng kiểm tra environment

### Steps (Part A - MATCHED - Payload runs)

1. Mở cmd → `whoami` (kiểm tra username)
2. Mở cmd → `hostname` (kiểm tra computer name)
3. Kiểm tra: Username có chứa "Phạm", "Admin", "User" HOẶC Computer có chứa "DESKTOP", "LAPTOP"?
4. Nếu YES → Nhấn Ctrl+F5
5. Lắc chuột 2 giây
6. Quan sát console

### Expected Result (Part A)

```
[+] Loader Started - NT230 Group 13
[*] Checking Environmental Key...
[+] Computer Name : DESKTOP-ABC123
[+] Username      : admin
[+] Volume Serial : 0xxxxxxxxx
[+] Environmental Key MATCHED → Allow payload ← KEY!
[+] All checks passed! Running payload...
✅ MessageBox hiển thị
```

### Steps (Part B - NOT MATCHED - Payload blocked, optional)

1. Chạy trên environment mà:
   - Username: không chứa "Phạm", "Admin", "User"
   - Computer Name: không chứa "DESKTOP", "LAPTOP"
   - (Ví dụ: VM với username "testuser" + computer "SOMEPC")
2. Nhấn Ctrl+F5
3. Lắc chuột
4. Quan sát console

### Expected Result (Part B)

```
[+] Loader Started - NT230 Group 13
[*] Checking Environmental Key...
[+] Computer Name : SOMEPC
[+] Username      : testuser
[+] Volume Serial : 0xxxxxxxxx
[-] Environmental Key NOT MATCH → Blocking ← KEY!
❌ NO MessageBox (payload blocked)
```

---

## Summary Table

| Scenario    | Requirement       | Action               | Result          |
| ----------- | ----------------- | -------------------- | --------------- |
| 1. Clean    | Real PC, 4+ cores | Ctrl+F5 + move mouse | ✅ Payload runs |
| 2. Debugger | VS opened         | F5                   | ❌ Blocked      |
| 3. VM       | VM with <4 cores  | Ctrl+F5 + move mouse | ❌ Blocked      |
| 4. Sandbox  | Real PC           | Ctrl+F5 + NO mouse   | ❌ Blocked      |
| 5. Env Key  | Check env vars    | Ctrl+F5              | ✅/❌ Depends   |
