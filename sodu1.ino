#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DFRobotDFPlayerMini.h>

// Thông tin WiFi và API
const char* ssid = "VJU Guest";
const char* password = "Vjuguest@2024";
const char* apiKey = "AK_CS.d40b848092e711efa865954870c6d3a1.LOcehByA9rAsj3hjEUoCH64mBubM7HBveBYdGABQlUstKcMMHTA6TJ1RZBMWx3DXptCLRCap";
const char* apiUrlLatest = "https://oauth.casso.vn/v2/transactions?fromDate=2024-01-01&pageSize=1&sort=DESC";
const char* apiUrlRecent = "https://oauth.casso.vn/v2/transactions?fromDate=2024-01-01&pageSize=10&sort=DESC";

// Cấu hình DFPlayer Mini
DFRobotDFPlayerMini myDFPlayer;

// Biến lưu giao dịch cũ
String lastTransactionID = "";
String lastTransactionTime = "";

// Web server
WebServer server(80);

// Nội dung trang web
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>Transaction information</title>
    <meta http-equiv="refresh" content="5">
    <style>
      body { font-family: Arial, sans-serif; margin: 20px; }
      h1 { color: #007BFF; }
      table { width: 100%; border-collapse: collapse; }
      th, td { padding: 8px 12px; border: 1px solid #ddd; text-align: left; }
      th { background-color: #f2f2f2; }
    </style>
  </head>
  <body>
    <h1>Latest transaction information</h1>
    %TRANSACTION_LATEST%
    <h1>10 most recent transaction</h1>
    %TRANSACTION_TABLE%
  </body>
</html>
)rawliteral";

// Bảng giao dịch (dạng HTML)
String latestTransactionHTML = "<p>No transaction information yet.</p>";
String transactionTable = "<p>There are no recent transactions.</p>";

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 26, 27); // DFPlayer Mini (TX = 26, RX = 27)

  // Khởi động DFPlayer
  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("Không thể khởi động DFPlayer!");
    while (true);
  }
  myDFPlayer.volume(30);

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối tới WiFi...");
  }
  Serial.println("Đã kết nối WiFi!");

  // Cấu hình Web Server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server đang chạy trên http://[ESP32_IP]");
  Serial.println(WiFi.localIP());

  // Chạy lần đầu
  fetchLatestTransaction();
  fetchRecentTransactions();
}

void loop() {
  server.handleClient();

  // Lần kiểm tra giao dịch mới
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime > 10000) {
    fetchLatestTransaction();
    fetchRecentTransactions();
    lastCheckTime = millis();
  }
}

void handleRoot() {
  String pageContent = htmlPage;
  pageContent.replace("%TRANSACTION_LATEST%", latestTransactionHTML);
  pageContent.replace("%TRANSACTION_TABLE%", transactionTable);
  server.send(200, "text/html", pageContent);
}

void fetchLatestTransaction() {
  HTTPClient http;
  http.begin(apiUrlLatest);
  http.addHeader("Authorization", String("Apikey ") + apiKey);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      JsonObject latestTransaction = doc["data"]["records"][0];
      String newTransactionID = latestTransaction["tid"].as<String>();
      String newTransactionTime = latestTransaction["when"].as<String>();
      String description = latestTransaction["description"].as<String>();
      String amount = formatCurrency(latestTransaction["amount"].as<int>());

      latestTransactionHTML = "<table>";
      latestTransactionHTML += "<tr><th>Transaction ID</th><td>" + newTransactionID + "</td></tr>";
      latestTransactionHTML += "<tr><th>Time</th><td>" + newTransactionTime + "</td></tr>";
      latestTransactionHTML += "<tr><th>Describe</th><td>" + description + "</td></tr>";
      latestTransactionHTML += "<tr><th>MONEY</th><td>" + amount + "</td></tr>";
      latestTransactionHTML += "</table>";

      // Phát âm thanh nếu giao dịch mới
      if (newTransactionID != lastTransactionID || newTransactionTime != lastTransactionTime) {
        lastTransactionID = newTransactionID;
        lastTransactionTime = newTransactionTime;
        playTransactionAudio(amount);
      }
    }
  }
  http.end();
}

void fetchRecentTransactions() {
  HTTPClient http;
  http.begin(apiUrlRecent);
  http.addHeader("Authorization", String("Apikey ") + apiKey);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      JsonArray records = doc["data"]["records"].as<JsonArray>();
      transactionTable = "<table>";
      transactionTable += "<tr><th>ID =</th><th>time</th><th>description</th><th>money</th></tr>";

      for (JsonObject transaction : records) {
        String id = transaction["tid"].as<String>();
        String time = transaction["when"].as<String>();
        String desc = transaction["description"].as<String>();
        String amount = formatCurrency(transaction["amount"].as<int>());
        transactionTable += "<tr><td>" + id + "</td><td>" + time + "</td><td>" + desc + "</td><td>" + amount + "</td></tr>";
      }
      transactionTable += "</table>";
    }
  }
  http.end();
}

void playTransactionAudio(String amount) {
  Serial.println("Phát âm thanh cho giao dịch: " + amount);
  myDFPlayer.play(1); // Phát file âm thanh mặc định (001.mp3)
}

String formatCurrency(int amount) {
  String str = String(abs(amount));
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


