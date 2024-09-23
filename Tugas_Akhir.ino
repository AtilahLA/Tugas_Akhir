// Blynk credentials
#define BLYNK_TEMPLATE_ID "TMPL6AHCkBGzx"
#define BLYNK_TEMPLATE_NAME "kelembapan"
#define BLYNK_AUTH_TOKEN "CP4F79RX2ZZo-zh2yiWCmE4hcVEV3K8U"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BlynkSimpleEsp32.h>

// OLED display settings
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi credentials
#define WIFI_SSID "Redmi Note 11"
#define WIFI_PASSWORD "11111111"

// Telegram BOT Token (Get from BotFather)
#define BOT_TOKEN "6387319255:AAFXDw9HwqU4gcV0DkJR9Ka8RQ64pQJvwLs"
#define CHAT_ID "5535845901"

// Pin untuk sensor kelembapan tanah dan buzzer
const int sensor_pin = 34;
const int buzzer_pin = 25;  // Pin untuk buzzer

// SSL Certificate Root Telegram
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

int _moisture, sensor_analog;
int positionX = 10;  // Initial X position of the circle (starting with radius offset)
int direction = 1;  // 1 means moving right, -1 means moving left
const int radius = 10;  // Radius of the circle

// Flag untuk notifikasi
bool isDryNotified = false;

void setup(void) {
  Serial.begin(115200);     

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);  // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.display();           // Display initial buffer

  // Setup pin untuk buzzer
  pinMode(buzzer_pin, OUTPUT);
  digitalWrite(buzzer_pin, LOW);  // Matikan buzzer di awal

  // Connect to Wi-Fi
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startAttemptTime = millis(); // Simpan waktu awal
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttemptTime > 10000) { // Timeout after 10 seconds
      Serial.println("Gagal terhubung ke WiFi.");
      return;
    }
  }
  
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Inisialisasi Telegram Bot
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  bot.sendMessage(CHAT_ID, "Bot started up", "");
  
  // Inisialisasi Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
}

void loop(void) {
  Blynk.run();  // Run Blynk

  // Baca data sensor
  sensor_analog = analogRead(sensor_pin);
  _moisture = (100 - ((sensor_analog / 4095.00) * 100));

  // Cetak ke Serial Monitor
  Serial.print("Moisture = ");
  Serial.print(_moisture);
  Serial.println("%");

  // Tampilkan kelembapan di OLED
  display.clearDisplay();  // Clear the buffer
  display.setCursor(0, 0); // Set cursor to top-left corner
  display.print("Moisture = ");
  display.print(_moisture);
  display.println("%");

  // Kirim data kelembapan ke Blynk (Virtual Pin V2)
  Blynk.virtualWrite(V2, _moisture);

  // Cek pesan baru dari Telegram
  if (bot.getUpdates(bot.last_message_received + 1)) {
    String command = bot.messages[0].text;
    if (command == "/status") {
      String statusMessage = "Status Kelembapan: " + String(_moisture) + "%";
      bot.sendMessage(CHAT_ID, statusMessage, "");
    }
  }

  // Logika buzzer dan notifikasi
  if (_moisture <= 30 && !isDryNotified) {
    bot.sendMessage(CHAT_ID, "Peringatan: Tanah kering! Kelembapan = " + String(_moisture) + "%", "");
    isDryNotified = true;
    Serial.println("Notifikasi: Tanah kering terkirim.");

    // Nyalakan buzzer
    digitalWrite(buzzer_pin, LOW);
    Serial.println("Buzzer ON (kelembapan <= 30)");

    // Kendalikan LED di Blynk (Merah untuk Kering)
    Blynk.virtualWrite(V0, 255); // Nyalakan LED merah
    Blynk.virtualWrite(V4, 0);   // Matikan LED kuning
    Blynk.virtualWrite(V3, 0);   // Matikan LED hijau
  }
  // Reset notifikasi dan matikan buzzer jika kelembapan di atas 31% (memberikan buffer kecil)
  else if (_moisture > 31 && isDryNotified) {
    isDryNotified = false;
    Serial.println("Tanah sudah basah kembali.");

    // Matikan buzzer
    digitalWrite(buzzer_pin, HIGH);
    Serial.println("Buzzer OFF (kelembapan > 31)");
  }

  // Tambahan logika untuk menampilkan status tanah dan kendalikan LED di Blynk
  if (_moisture >= 31 && _moisture <= 60) {
    display.setCursor(0, 16); // Set cursor sedikit ke bawah
    display.println("Status: Agak Basah");
    Serial.println("Status: Agak Basah");

    // LED Kuning untuk "Agak Basah"
    Blynk.virtualWrite(V0, 0);   // Matikan LED merah
    Blynk.virtualWrite(V4, 255); // Nyalakan LED kuning
    Blynk.virtualWrite(V3, 0);   // Matikan LED hijau
  } else if (_moisture > 60) {
    display.setCursor(0, 16); // Set cursor sedikit ke bawah
    display.println("Status: Basah");
    Serial.println("Status: Basah");

    // LED Hijau untuk "Basah"
    Blynk.virtualWrite(V0, 0);   // Matikan LED merah
    Blynk.virtualWrite(V4, 0);   // Matikan LED kuning
    Blynk.virtualWrite(V3, 255); // Nyalakan LED hijau
  } else {
    display.setCursor(0, 16); // Set cursor sedikit ke bawah
    display.println("Status: Kering");
    Serial.println("Status: Kering");

    // LED Merah untuk "Kering"
    Blynk.virtualWrite(V0, 255); // Nyalakan LED merah
    Blynk.virtualWrite(V4, 0);   // Matikan LED kuning
    Blynk.virtualWrite(V3, 0);   // Matikan LED hijau
  }

  display.display();  // Tampilkan status di OLED

  delay(2000);  // Penundaan 2 detik sebelum pembacaan berikutnya
}
