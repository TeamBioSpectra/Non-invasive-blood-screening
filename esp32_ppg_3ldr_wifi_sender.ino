/*
  BioSpectra - ESP32 PPG Sensor (3 LDRs) -> WiFi Sender  [FINAL]
  =================================================================
  Each LDR is paired with one LED: while that LED is on, only its paired
  LDR is read. Sends Red/Green/IR values + Hb Index to the Raspberry Pi
  over WiFi. No changes needed on the Pi side (pi_oled_receiver.py).

  ---------------------------------------------------------------
  FULL WIRING REFERENCE
  ---------------------------------------------------------------
  LEDs (output-capable pins):
    Red LED   (anode via resistor) -> GPIO27
    Green LED (anode via resistor) -> GPIO25
    IR LED    (anode via resistor) -> GPIO26
    All LED cathodes                -> GND

  LDRs (input-only / ADC-capable pins), each paired with one LED:
    LDR #1 (paired with Red LED)   -> GPIO34
    LDR #2 (paired with Green LED) -> GPIO35
    LDR #3 (paired with IR LED)    -> GPIO32
    Each LDR wired as a voltage divider: one leg -> 3.3V, other leg ->
    series resistor -> GND, with the ESP32 pin reading the midpoint.

  Power:
    All sensors/LEDs run off the ESP32's 3.3V and GND pins (not VIN/5V).

  ---------------------------------------------------------------
  BEFORE UPLOADING, EDIT:
    - ssid, password : your WiFi credentials (2.4GHz network only -
                        ESP32 cannot connect to 5GHz)
    - piIP            : your Raspberry Pi's IP address (run `hostname -I`
                         on the Pi to find it)
*/

#include <WiFi.h>

// --- LEDs (output-capable pins) ---
#define RED_LED   27
#define GREEN_LED 25
#define IR_LED    26

// --- LDRs (input-only / ADC-capable pins), each paired with one LED ---
#define LDR_RED   34   // paired with RED_LED
#define LDR_GREEN 35   // paired with GREEN_LED
#define LDR_IR    32   // paired with IR_LED

const char* ssid     = "NARZO 70 Pro 5G";
const char* password = "24197298";
const char* piIP      = "10.240.87.48";   // <-- Pi's IP address
const uint16_t piPort = 5000;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(IR_LED, OUTPUT);
  // LDR pins don't need pinMode() set for analogRead() on ESP32

  connectToWiFi();
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("WiFi FAILED. Status code: ");
    Serial.println(WiFi.status());
    Serial.println("(1=no matching network, 4=wrong password, 6=lost connection)");
  }
}

void loop() {
  // If WiFi dropped since setup(), try to reconnect before this cycle
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected - retrying...");
    connectToWiFi();
  }

  // --- Red ---
  digitalWrite(RED_LED, HIGH);
  delay(500);
  int redVal = analogRead(LDR_RED);
  delay(1500);
  digitalWrite(RED_LED, LOW);
  delay(500);

  // --- Green ---
  digitalWrite(GREEN_LED, HIGH);
  delay(500);
  int greenVal = analogRead(LDR_GREEN);
  delay(1500);
  digitalWrite(GREEN_LED, LOW);
  delay(500);

  // --- IR ---
  digitalWrite(IR_LED, HIGH);
  delay(500);
  int irVal = analogRead(LDR_IR);
  delay(1500);
  digitalWrite(IR_LED, LOW);

  float hbIndex = (redVal + irVal == 0) ? 0 : (float)greenVal / (redVal + irVal);

  Serial.print("Red: ");   Serial.println(redVal);
  Serial.print("Green: "); Serial.println(greenVal);
  Serial.print("IR: ");    Serial.println(irVal);
  Serial.print("Hb Index: "); Serial.println(hbIndex, 4);

  // --- Send to Raspberry Pi over WiFi ---
  if (client.connect(piIP, piPort)) {
    String payload = String(redVal) + "," + String(greenVal) + "," +
                      String(irVal) + "," + String(hbIndex, 4) + "\n";
    client.print(payload);
    client.stop();
    Serial.print("Sent to Pi: ");
    Serial.println(payload);
  } else {
    Serial.println("Could not connect to Pi - check IP/network");
  }

  delay(2000);
}
