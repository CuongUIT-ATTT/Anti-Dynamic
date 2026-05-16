# Tài liệu hướng dẫn kiểm thử Anti-Dynamic Analysis

## Phần mở đầu

Tài liệu này mô tả quy trình kiểm thử hai phiên bản chương trình mô phỏng kỹ thuật chống phân tích động trong mã độc, phục vụ bài tập lớn môn **Cơ chế hoạt động của mã độc (NT230)** tại Trường Đại học Công nghệ Thông tin - ĐHQG-HCM, dưới sự hướng dẫn của **TS. Phan Thế Duy**.

Hai phiên bản cần kiểm thử:

- `Anti_Dynamic-Basic.exe`: phiên bản Basic, triển khai các kỹ thuật Anti-Debug, Anti-VM và Anti-Sandbox.
- `Anti_Dynamic.exe`: phiên bản Advanced, kế thừa logic của bản Basic và bổ sung kỹ thuật **API Hashing** bằng thuật toán **DJB2** để ẩn hàm `IsDebuggerPresent`.

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

Mục tiêu của phần kiểm thử tĩnh là chứng minh hàm `IsDebuggerPresent` đã được ẩn khỏi bảng Import Address Table (IAT) và không còn xuất hiện dưới dạng chuỗi ký tự trần trong file thực thi. Điều này xác nhận kỹ thuật API Hashing bằng DJB2 đã che giấu dấu vết API chống gỡ lỗi khỏi các phương pháp phân tích tĩnh cơ bản.

### TC-01: Kiểm tra bảng Import Address Table (IAT) bằng CFF Explorer

#### Mục tiêu

Xác minh rằng `IsDebuggerPresent` không xuất hiện trực tiếp trong bảng Import của file PE.

#### Công cụ sử dụng

- CFF Explorer
- File kiểm thử: `Anti_Dynamic.exe`

#### Steps

1. Khởi động CFF Explorer.
2. Nạp file `Anti_Dynamic.exe` vào công cụ.
3. Ở cây thư mục bên trái, chọn **Import Directory**.
4. Mở danh sách hàm import của thư viện `kernel32.dll`.
5. Quan sát toàn bộ danh sách hàm được import.

#### Expected Results

Danh sách Import của `kernel32.dll` không có sự xuất hiện của hàm `IsDebuggerPresent`.

#### Cơ chế hoạt động

Ở phiên bản Advanced, chương trình không gọi trực tiếp `IsDebuggerPresent` thông qua IAT. Thay vào đó, tên hàm được xử lý bằng cơ chế API Hashing với thuật toán DJB2. Tại thời điểm chạy, chương trình duyệt bảng export của thư viện cần thiết, tính hash từng tên hàm và so khớp với mã băm đã định nghĩa để lấy địa chỉ hàm thật.

#### Minh chứng báo cáo

Chụp ảnh màn hình danh sách Import trong CFF Explorer, trong đó không có hàm `IsDebuggerPresent`.

### TC-02: Quét chuỗi ký tự (Strings Scan) bằng x64dbg

#### Mục tiêu

Xác minh rằng chuỗi `IsDebuggerPresent` không xuất hiện dưới dạng chuỗi ký tự trần trong file thực thi.

#### Công cụ sử dụng

- x64dbg
- File kiểm thử: `Anti_Dynamic.exe`

#### Steps

1. Mở x64dbg.
2. Nạp file `Anti_Dynamic.exe` vào x64dbg.
3. Tại cửa sổ CPU, click chuột phải.
4. Chọn **Search for** > **All modules** > **String references**.
5. Nhập từ khóa `IsDebuggerPresent` vào ô tìm kiếm.

#### Expected Results

x64dbg trả về `0 results`, không tìm thấy chuỗi `IsDebuggerPresent`.

#### Cơ chế hoạt động

Do phiên bản Advanced sử dụng mã băm thay cho tên API gốc, chuỗi `IsDebuggerPresent` không cần được lưu trực tiếp trong vùng dữ liệu của chương trình. Vì vậy, thao tác quét chuỗi không thể phát hiện tên API này bằng phương pháp thông thường.

#### Minh chứng báo cáo

Chụp ảnh màn hình kết quả tìm kiếm rỗng trong x64dbg.

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

Ở phiên bản Advanced, trước khi gọi API chống gỡ lỗi, chương trình phân giải địa chỉ hàm thông qua mã băm DJB2 thay vì import trực tiếp. Nếu phân giải thành công và không phát hiện debugger, luồng thực thi tiếp tục giống bản Basic.

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

Ở bản Advanced, logic phát hiện debugger vẫn được giữ nguyên về mặt hành vi, nhưng lời gọi `IsDebuggerPresent` được ẩn bằng API Hashing. Chương trình phân giải API từ mã băm tại runtime, sau đó gọi hàm tương ứng để xác định trạng thái debugger. Nếu phát hiện debugger, chương trình thoát im lặng.

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
| 1 | TC-01: Kiểm tra IAT | Advanced (`Anti_Dynamic.exe`) | CFF Explorer | Không thấy `IsDebuggerPresent` trong Import Directory | [ ] |
| 2 | TC-02: Quét chuỗi `IsDebuggerPresent` | Advanced (`Anti_Dynamic.exe`) | x64dbg | Trả về `0 results` | [ ] |
| 3 | Kịch bản 1: Môi trường sạch | Basic và Advanced | Click đúp hoặc `Ctrl + F5`, lắc chuột 3 giây | Popup nhóm `G13` hiển thị | [ ] |
| 4 | Kịch bản 2: Phát hiện Sandbox | Basic và Advanced | Click đúp hoặc `Ctrl + F5`, giữ chuột đứng im | Chương trình thoát im lặng, không popup | [ ] |
| 5 | Kịch bản 3: Phát hiện Debugger | Basic và Advanced | `F5` trong Visual Studio hoặc x64dbg | Chương trình thoát im lặng, không popup | [ ] |
| 6 | Giả lập Anti-VM bằng điều kiện CPU | Basic và Advanced | Sửa `< 4` thành `> 1`, build và chạy lại | Chương trình thoát im lặng, không popup | [ ] |
| 7 | Khôi phục điều kiện CPU ban đầu | Basic và Advanced | Sửa `> 1` về `< 4` | Mã nguồn trở về logic thiết kế ban đầu | [ ] |

