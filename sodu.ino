#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFRobotDFPlayerMini.h>

const char* ssid = "FPT Telecom";
const char* password = "diep1976";
const char* apiKey = "your api";  // Thay bằng API key của bạn
const char* apiUrl = "https://oauth.casso.vn/v2/transactions?fromDate=2024-01-01&pageSize=10&sort=ASC";

DFRobotDFPlayerMini myDFPlayer;

String lastTid = ""; // Biến lưu ID giao dịch gần nhất

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 26, 27); // Kết nối DFPlayer

  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("Không thể khởi động DFPlayer!");
    while (true);
  }

  myDFPlayer.volume(20); // Đặt âm lượng

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối tới WiFi...");
  }
  Serial.println("Đã kết nối WiFi!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Authorization", String("Apikey ") + apiKey);
    
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Phản hồi từ API:");
      Serial.println(response);

      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        JsonArray records = doc["data"]["records"].as<JsonArray>();
        for (JsonObject record : records) {
          const char* tid = record["tid"];
          int amount = record["amount"];
          const char* description = record["description"];
          const char* when = record["when"];

          // So sánh ID giao dịch
          if (String(tid) != lastTid) {
            lastTid = tid; // Cập nhật ID giao dịch gần nhất
            
            // Chuyển đổi số tiền thành VND và định dạng
            String amountStr = formatCurrency(amount);
            Serial.printf("Giao dịch ID: %s\n", tid);
            Serial.printf("Số tiền: %s\n", amountStr.c_str());
            Serial.printf("Mô tả: %s\n", description);
            Serial.printf("Thời gian: %s\n", when);
            Serial.println("---------------------------");

            // Phát âm thanh thông báo
            myDFPlayer.play(String(amount > 0 ? "001" : "002").toInt()); // 001 cho thành công, 002 cho lỗi

            // Gửi dữ liệu giao dịch tới trang web (thay URL bằng địa chỉ của bạn)
            sendTransactionData(tid, amountStr, description, when);
          }
        }
      } else {
        Serial.print("Lỗi phân tích cú pháp JSON: ");
        Serial.println(error.f_str());
      }
    } else {
      Serial.printf("Lỗi HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Không thể kết nối tới WiFi");
  }

  delay(5000); // Thời gian chờ trước khi lấy giao dịch tiếp theo
}

// Hàm định dạng số tiền
String formatCurrency(int amount) {
  String str = String(abs(amount)); // Lấy giá trị tuyệt đối
  String formatted = "";
  int len = str.length();
  int count = 0;

  for (int i = len - 1; i >= 0; i--) {
    if (count > 0 && count % 3 == 0) {
      formatted = '.' + formatted;
    }
    formatted = str[i] + formatted;
    count++;
  }

  return formatted + " VND";
}

// Hàm gửi dữ liệu giao dịch tới trang web
void sendTransactionData(const char* tid, String amountStr, const char* description, const char* when) {
  // Gửi dữ liệu đến trang web (thay URL bằng địa chỉ của bạn)
  HTTPClient http;
  http.begin("http://your-web-url.com/api/save_transaction"); // Địa chỉ API để lưu giao dịch
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = String("{\"tid\":\"") + tid + "\",\"amount\":\"" + amountStr + "\",\"description\":\"" + description + "\",\"when\":\"" + when + "\"}";
  
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Dữ liệu đã được gửi thành công:");
    Serial.println(response);
  } else {
    Serial.printf("Lỗi khi gửi dữ liệu: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}

