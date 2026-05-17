# Tài liệu hướng dẫn kiểm thử Anti-Dynamic Analysis

## Phần mở đầu

Tài liệu này mô tả quy trình kiểm thử hai phiên bản chương trình mô phỏng kỹ thuật chống phân tích động trong mã độc, phục vụ bài tập lớn môn **Cơ chế hoạt động của mã độc (NT230)** tại Trường Đại học Công nghệ Thông tin - ĐHQG-HCM, dưới sự hướng dẫn của **TS. Phan Thế Duy**.

Hai phiên bản cần kiểm thử:

- `Anti_Dynamic-Basic.exe`: phiên bản Basic, triển khai các kỹ thuật Anti-Debug, Anti-VM và Anti-Sandbox.
- `Anti_Dynamic.exe`: phiên bản Advanced, kế thừa logic của bản Basic và bổ sung kỹ thuật **Environmental Keying** để xác thực môi trường chạy dựa trên Computer Name, Username và Volume Serial.

Thông tin nhóm thực hiện:

- Mã nhóm: `G13`
- Thành viên:
  - Phạm Hùng Cường
  - Nguyễn Minh Quân
  - Phan Đức Anh
  - Trần Gia Bảo

Mục tiêu của tài liệu là chuẩn hóa các bước kiểm thử, kết quả kỳ vọng và cơ chế hoạt động để phục vụ quá trình nghiệm thu, chụp minh chứng và viết báo cáo kỹ thuật.

## Phần 1 - Kiểm thử phân tích tĩnh (Static Verification)

> Phần này **chỉ áp dụng cho phiên bản Advanced**: `Anti_Dynamic.exe`.

Mục tiêu của phần kiểm thử tĩnh là chứng minh hàm `CheckEnvironmentalKey()` đã được triển khai để xác thực các tham số môi trường như Computer Name, Username và Volume Serial. Điều này xác nhận rằng kỹ thuật Environmental Keying đã được áp dụng để kiểm soát thực thi payload dựa trên các đặc tính hệ thống.

### TC-01: Kiểm tra hàm CheckEnvironmentalKey() bằng Disassembler (IDA Pro / Ghidra)

#### Mục tiêu

Xác minh rằng chương trình triển khai hàm kiểm tra môi trường để lấy thông tin Computer Name, Username và Volume Serial.

#### Công cụ sử dụng

- IDA Pro, Ghidra hoặc x64dbg
- File kiểm thử: `Anti_Dynamic.exe`

#### Steps

1. Mở IDA Pro hoặc Ghidra.
2. Nạp file `Anti_Dynamic.exe` vào công cụ.
3. Tìm hàm `CheckEnvironmentalKey()` hoặc các hàm liên quan.
4. Quan sát các lời gọi API:
   - `GetComputerNameA()`
   - `GetUserNameA()`
   - `GetVolumeInformationA()`

#### Expected Results

Danh sách API calls chứa những hàm lấy thông tin môi trường (Computer Name, Username, Volume Serial).

#### Cơ chế hoạt động

Ở phiên bản Advanced, chương trình triển khai hàm `CheckEnvironmentalKey()` để trích xuất các tham số môi trường từ hệ thống Windows. Những thông tin này được so sánh với các điều kiện được lập trình sẵn (ví dụ: tên máy chứa "DESKTOP" hoặc "LAPTOP", username chứa "Phạm" hoặc "Admin"). Nếu các điều kiện khớp, payload được phép thực thi.

#### Minh chứng báo cáo

Chụp ảnh màn hình Disassembler cho thấy các lời gọi API `GetComputerNameA`, `GetUserNameA`, `GetVolumeInformationA` trong hàm kiểm tra môi trường.

### TC-02: Quét chuỗi tham số Environmental Keying bằng x64dbg

#### Mục tiêu

Xác minh rằng chuỗi kiểm tra môi trường như "DESKTOP", "LAPTOP", "Phạm", "Admin" xuất hiện trong file thực thi để phục vụ logic Environmental Keying.

#### Công cụ sử dụng

- x64dbg
- File kiểm thử: `Anti_Dynamic.exe`

#### Steps

1. Mở x64dbg.
2. Nạp file `Anti_Dynamic.exe` vào x64dbg.
3. Tại cửa sổ CPU, click chuột phải.
4. Chọn **Search for** > **All modules** > **String references**.
5. Nhập từ khóa `DESKTOP` hoặc `LAPTOP` vào ô tìm kiếm.
6. Xác nhận có kết quả tìm thấy trong bảng dữ liệu của chương trình.

#### Expected Results

x64dbg tìm thấy chuỗi `DESKTOP`, `LAPTOP`, `Phạm`, `Admin` hoặc các tham số khác được sử dụng trong hàm kiểm tra môi trường.

#### Cơ chế hoạt động

Phiên bản Advanced sử dụng Environmental Keying để xác định các mục tiêu payload dựa trên các tham số hệ thống như Computer Name và Username. Những chuỗi so sánh này được lưu trữ trong bảng dữ liệu của file PE và được quét lúc thực thi. Nếu các điều kiện environmental khớp, payload được kích hoạt. Kỹ thuật này giúp payload chỉ chạy trên các máy có cấu hình hoặc môi trường cụ thể.

#### Minh chứng báo cáo

Chụp ảnh màn hình x64dbg cho thấy kết quả tìm kiếm chuỗi environmental keying như "DESKTOP", "LAPTOP", "Phạm", "Admin" trong bảng dữ liệu chương trình.

## Phần 2 - Kiểm thử phân tích động và hành vi (Dynamic Verification)

Phần này áp dụng cho cả hai phiên bản:

- `Anti_Dynamic-Basic.exe`
- `Anti_Dynamic.exe`

Lý do gộp chung là hai phiên bản có cùng logic thay đổi hành vi khi chạy trong các môi trường khác nhau. Điểm khác biệt chính nằm ở cách phiên bản Advanced che giấu và phân giải API chống gỡ lỗi, nhưng kết quả hành vi cuối cùng vẫn giống bản Basic.

### Kịch bản 1: Môi trường sạch

#### Mục tiêu

Xác minh rằng chương trình vượt qua các lớp kiểm tra Anti-Debug, Anti-VM và Anti-Sandbox khi chạy trong môi trường người dùng thật, từ đó kích hoạt payload hiển thị thông tin nhóm `G13`.

#### File áp dụng

Thực hiện lần lượt với:

- `Anti_Dynamic-Basic.exe`
- `Anti_Dynamic.exe`

#### Steps

1. Đảm bảo không có trình gỡ lỗi nào đang gắn vào tiến trình.
2. Chạy chương trình bằng một trong các cách sau:
   - Click đúp trực tiếp vào file `.exe`.
   - Hoặc nhấn `Ctrl + F5` trong Visual Studio để chạy ở chế độ **Start Without Debugging**.
3. Ngay sau khi chương trình bắt đầu chạy, rê hoặc lắc chuột liên tục trên màn hình trong khoảng 3 giây.

#### Expected Results

Popup hiển thị thông tin nhóm xuất hiện, bao gồm:

- Mã nhóm: `G13`
- Phạm Hùng Cường
- Nguyễn Minh Quân
- Phan Đức Anh
- Trần Gia Bảo

#### Cơ chế hoạt động

Khi chương trình không bị debugger theo dõi, các hàm kiểm tra Anti-Debug không phát hiện dấu hiệu bất thường. Máy thật thường có số lượng CPU đủ lớn và không tồn tại các khóa Registry đặc trưng của VMware hoặc VirtualBox, do đó vượt qua kiểm tra Anti-VM. Hành động lắc chuột làm tọa độ chuột trước và sau khoảng chờ thay đổi, giúp chương trình xác nhận có tương tác người dùng thật và không xem môi trường hiện tại là Sandbox tự động.

Ở phiên bản Advanced, trước khi thực thi payload, chương trình kiểm tra Environmental Key bằng cách so sánh Computer Name, Username và Volume Serial với các điều kiện được thiết lập. Chỉ khi các điều kiện khớp, hàm `ExecutePayload()` mới được gọi. Nếu Environmental Key không khớp, chương trình thoát im lặng.

#### Minh chứng báo cáo

Chụp ảnh màn hình popup thông tin nhóm `G13` hiển thị thành công.

### Kịch bản 2: Phát hiện Sandbox

#### Mục tiêu

Xác minh rằng chương trình có thể phát hiện môi trường thiếu tương tác người dùng, từ đó tự kết thúc im lặng.

#### File áp dụng

Thực hiện lần lượt với:

- `Anti_Dynamic-Basic.exe`
- `Anti_Dynamic.exe`

#### Steps

1. Chạy chương trình bằng cách click đúp trực tiếp hoặc nhấn `Ctrl + F5` trong Visual Studio.
2. Sau khi chương trình bắt đầu chạy, buông hoàn toàn tay khỏi chuột.
3. Giữ chuột đứng im tuyệt đối và không nhấn bất kỳ phím nào trong khoảng 3 giây.

#### Expected Results

Chương trình tự kết thúc im lặng, không hiển thị popup payload.

#### Cơ chế hoạt động

Hàm `CheckSandboxMouseMovement()` ghi nhận tọa độ chuột tại thời điểm ban đầu, sau đó chờ một khoảng thời gian bằng `Sleep(2000)` và lấy lại tọa độ chuột lần thứ hai. Nếu tọa độ trước và sau không thay đổi, tức `Delta = 0`, chương trình kết luận môi trường đang chạy có khả năng là Sandbox tự động không có tương tác người dùng. Khi đó, chương trình rẽ nhánh thoát im lặng.

#### Minh chứng báo cáo

Chụp ảnh màn hình desktop sau khi chạy chương trình nhưng không có popup hoặc hộp thoại nào xuất hiện.

### Kịch bản 3: Phát hiện Debugger

#### Mục tiêu

Xác minh rằng chương trình có thể phát hiện quá trình gỡ lỗi và tự kết thúc im lặng để chống phân tích động.

#### File áp dụng

Thực hiện lần lượt với:

- `Anti_Dynamic-Basic.exe`
- `Anti_Dynamic.exe`

#### Steps

1. Chạy chương trình trong môi trường có debugger bằng một trong các cách sau:
   - Nhấn `F5` trong Visual Studio để chạy ở chế độ **Start Debugging**.
   - Hoặc mở x64dbg rồi nạp file `.exe` cần kiểm thử.
2. Nếu dùng x64dbg, nhấn `F9` để cho chương trình thực thi tự do.
3. Quan sát trạng thái tiến trình và kiểm tra xem popup payload có xuất hiện hay không.

#### Expected Results

Chương trình tự kết thúc im lặng, không hiển thị popup payload.

#### Cơ chế hoạt động

Ở bản Basic, chương trình kiểm tra debugger thông qua API và kiểm tra thủ công flag `BeingDebugged` trong cấu trúc PEB. Khi tiến trình bị debugger gắn vào, flag này có giá trị `1` hoặc API chống gỡ lỗi trả về trạng thái phát hiện debugger, khiến chương trình thoát ngay.

Ở bản Advanced, trước khi kiểm tra Anti-Debug, chương trình trước tiên kiểm tra Environmental Key. Nếu Environmental Key không khớp, chương trình không cần thực hiện các kiểm tra Anti-Debug khác mà sẽ thoát ngay. Nếu Environmental Key khớp, chương trình tiếp tục kiểm tra debugger và các môi trường phân tích khác giống bản Basic.

#### Minh chứng báo cáo

Chụp ảnh màn hình Visual Studio hoặc x64dbg cho thấy tiến trình đã kết thúc, không có popup payload xuất hiện.

## Phần 3 - Giả lập tình huống phát hiện máy ảo

### Mục tiêu

Do máy thật thường có cấu hình phần cứng hợp lệ và không chứa dấu vết Registry của phần mềm ảo hóa, các hàm Anti-VM có thể trả về trạng thái sạch. Để kiểm thử trực tiếp logic phát hiện máy ảo trên máy thật, có thể sửa tạm điều kiện kiểm tra số nhân CPU.

### File áp dụng

Có thể áp dụng cho mã nguồn của cả hai phiên bản:

- `basic.cpp`
- `Advanced.cpp`

### Steps

1. Mở file mã nguồn cần kiểm thử.
2. Tìm điều kiện kiểm tra số lượng CPU trong hàm Anti-VM:

```cpp
if (sysInfo.dwNumberOfProcessors < 4)
```

3. Sửa tạm thời điều kiện thành:

```cpp
if (sysInfo.dwNumberOfProcessors > 1)
```

4. Build lại chương trình ở chế độ Release.
5. Chạy file `.exe` bằng `Ctrl + F5` hoặc click đúp trực tiếp.
6. Có thể lắc chuột trong quá trình chạy để đảm bảo không bị chặn bởi kiểm tra Sandbox.

### Expected Results

Chương trình vẫn tự kết thúc im lặng và không hiển thị popup payload, dù có tương tác chuột. Kết quả này chứng minh nhánh xử lý Anti-VM đã được kích hoạt.

### Cơ chế hoạt động

Điều kiện gốc xem môi trường có ít hơn 4 nhân CPU là dấu hiệu nghi ngờ máy ảo hoặc môi trường phân tích. Khi sửa thành `> 1`, hầu hết máy thật hiện nay sẽ thỏa điều kiện này, khiến hàm kiểm tra CPU trả về trạng thái phát hiện máy ảo. Đây là cách ép chương trình đi vào nhánh Anti-VM để phục vụ kiểm thử và chụp minh chứng.

### Lưu ý sau kiểm thử

Sau khi hoàn tất kiểm thử, phải khôi phục điều kiện ban đầu:

```cpp
if (sysInfo.dwNumberOfProcessors < 4)
```

Việc khôi phục là bắt buộc để đảm bảo logic chương trình trở lại đúng thiết kế ban đầu.

## Checklist kiểm thử nhanh

| STT | Hạng mục kiểm thử | Phiên bản áp dụng | Công cụ / Cách chạy | Kết quả kỳ vọng | Trạng thái |
| --- | --- | --- | --- | --- | --- |
| 1 | TC-01: Kiểm tra Environmental Keying | Advanced (`Anti_Dynamic.exe`) | IDA Pro / Ghidra / x64dbg | Tìm thấy hàm kiểm tra môi trường và các API lấy thông tin hệ thống | [ ] |
| 2 | TC-02: Quét chuỗi Environmental Keying | Advanced (`Anti_Dynamic.exe`) | x64dbg | Tìm thấy chuỗi `DESKTOP`, `LAPTOP`, `Phạm`, `Admin` trong dữ liệu | [ ] |
| 3 | Kịch bản 1: Môi trường sạch | Basic và Advanced | Click đúp hoặc `Ctrl + F5`, lắc chuột 3 giây | Popup nhóm `G13` hiển thị | [ ] |
| 4 | Kịch bản 2: Phát hiện Sandbox | Basic và Advanced | Click đúp hoặc `Ctrl + F5`, giữ chuột đứng im | Chương trình thoát im lặng, không popup | [ ] |
| 5 | Kịch bản 3: Phát hiện Debugger | Basic và Advanced | `F5` trong Visual Studio hoặc x64dbg | Chương trình thoát im lặng, không popup | [ ] |
| 6 | Giả lập Anti-VM bằng điều kiện CPU | Basic và Advanced | Sửa `< 4` thành `> 1`, build và chạy lại | Chương trình thoát im lặng, không popup | [ ] |
| 7 | Khôi phục điều kiện CPU ban đầu | Basic và Advanced | Sửa `> 1` về `< 4` | Mã nguồn trở về logic thiết kế ban đầu | [ ] |

