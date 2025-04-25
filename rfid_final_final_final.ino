#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

// Pin Definitions
#define RST_PIN  D3  // Reset pin for RFID
#define SS_PIN   D4  // Slave Select pin for RFID
#define BUZZER   D8  // Buzzer pin
#define SCL_PIN  D1  // LCD SCL (I2C)
#define SDA_PIN  D2  // LCD SDA (I2C)

// LCD Object (Use 0x27 or 0x3F based on your module)
LiquidCrystal_PCF8574 lcd(0x27);

// RFID Object
MFRC522 mfrc522(SS_PIN, RST_PIN);
// WiFi Credentials
#define WIFI_SSID "Xyz"
#define WIFI_PASSWORD "12345678"

// Google Sheets URL
const String sheet_url = "https://script.google.com/macros/s/AKfycbw3cdLsWZbWbCEArSxRSKxJvisFUrTtqglXDOfOWCAKgj1FRa84kUON6bkHqSA28P4TUQ/exec?name=";

// UID to Name Mapping
struct User {
    String uid;
    String name;
};
User users[] = {
    {"732EF927", "Adam"},
    {"34B3BDA", "John"},
    {"E3D7814", "Qareena"},
    {"241229A7", "Aiman"}
};
const int userCount = sizeof(users) / sizeof(users[0]);

String getUserName(String uid) {
    for (int i = 0; i < userCount; i++) {
        if (users[i].uid == uid) {
            return users[i].name;
        }
    }
    return "Unknown";
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.begin(16, 2);
    lcd.setBacklight(1);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
    delay(2000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    SPI.begin();
    mfrc522.PCD_Init();
    pinMode(BUZZER, OUTPUT);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan Your Card");
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

    String cardUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        cardUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    cardUID.toUpperCase();

    String userName = getUserName(cardUID);
    Serial.println("Card UID: " + cardUID + " (" + userName + ")");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("User: " + userName);

    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);
    delay(200);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);

    if (WiFi.status() == WL_CONNECTED) {
        std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
        client->setInsecure();
        HTTPClient https;
        String requestURL = sheet_url + userName;
        Serial.println("Sending to Google Sheets: " + requestURL);
        
        if (https.begin(*client, requestURL)) {
            int httpCode = https.GET();
            if (httpCode > 0) {
                Serial.printf("Data Sent, Response Code: %d\n", httpCode);
                lcd.setCursor(0, 1);
                lcd.print("Data Recorded!");
            } else {
                Serial.printf("Failed to Send Data: %s\n", https.errorToString(httpCode).c_str());
            }
            https.end();
        } else {
            Serial.println("HTTPS Connection Failed.");
        }
    }

    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan Your Card");
}
