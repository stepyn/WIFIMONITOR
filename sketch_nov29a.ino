#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>  // Core graphics library
#include <Adafruit_ILI9341.h>
#include "button.h"

#define ILI9341
#define TFT_DC 2   // D2
#define TFT_CS 15  // D15
#define TFT_RST 4
#define TFT_MOSI 23  // SDI
#define TFT_CLK 18
#define TFT_RST 4
#define TFT_MISO 19

#define butpin1 25  //  up
#define butpin2 26  // down
#define butpin3 27  //  - middle

#define WIDTH 320
#define HEIGHT 240  // посмотреть на оригинальный код проверить юэйслайн и разность
#define GRAPH_BASELINE (HEIGHT - 18)
#define GRAPH_HEIGHT (HEIGHT - 52)
#define CHANNEL_WIDTH (WIDTH / 16)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

#define TFT_WHITE ILI9341_WHITE   /* 255, 255, 255 */
#define TFT_BLACK ILI9341_BLACK   /*   0,   0,   0 */
#define TFT_RED ILI9341_RED       /* 255,   0,   0 */
#define TFT_ORANGE ILI9341_ORANGE /* 255, 165,   0 */
#define TFT_YELLOW ILI9341_YELLOW /* 255, 255,   0 */
#define TFT_GREEN ILI9341_GREEN   /*   0, 255,   0 */
#define TFT_CYAN ILI9341_CYAN     /*   0, 255, 255 */
#define TFT_BLUE ILI9341_BLUE     /*   0,   0, 255 */
#define TFT_MAGENTA ILI9341_MAGENTA

// RSSI RANGE
#define RSSI_CEILING -40
#define RSSI_FLOOR -100
#define NEAR_CHANNEL_RSSI_ALLOW -70

uint16_t channel_color[] = {
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE
};

button butt1(butpin1);
button butt2(butpin2);
button butt3(butpin3);


void plot() {
  tft.setTextSize(1);
  uint8_t ap_count[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int32_t max_rssi[] = { -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 };

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  // clear old graph
  tft.fillScreen(ILI9341_BLACK);
  // plot found WiFi info
  for (int i = 0; i < n; i++) {
    int32_t channel = WiFi.channel(i);
    int32_t rssi = WiFi.RSSI(i);
    uint16_t color = channel_color[channel - 1];
    int height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, GRAPH_HEIGHT), 1, GRAPH_HEIGHT);
    // channel stat
    ap_count[channel - 1]++;
    if (rssi > max_rssi[channel - 1]) {
      max_rssi[channel - 1] = rssi;
    }

    tft.drawLine(channel * CHANNEL_WIDTH, GRAPH_BASELINE - height, (channel - 1) * CHANNEL_WIDTH, GRAPH_BASELINE + 1, color);
    tft.drawLine(channel * CHANNEL_WIDTH, GRAPH_BASELINE - height, (channel + 1) * CHANNEL_WIDTH, GRAPH_BASELINE + 1, color);

    // Print SSID, signal strengh and  encrypted
    tft.setTextColor(color);
    tft.setCursor((channel - 1) * CHANNEL_WIDTH, GRAPH_BASELINE - 10 - height);
    tft.print(WiFi.SSID(i));
    tft.print("(");
    tft.print(rssi);
    tft.print(" ");
    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_WPA2_PSK:
        tft.print("WPA2");
        break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        tft.print("WPA2-EAP");
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        tft.print("WPA/WPA2");
        break;
      case WIFI_AUTH_WPA3_PSK:
        tft.print("WPA3");
        break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        tft.print("WPA2/WPA3");
        break;
      case WIFI_AUTH_WAPI_PSK:
        tft.print("WAPI");
        break;
      case WIFI_AUTH_WPA_PSK:
        tft.print("WPA");
        break;
      case WIFI_AUTH_WEP:
        tft.print("WEP");
        break;
      case WIFI_AUTH_OPEN:
        tft.print("open");
        break;
      default:
        tft.print("unknown");
    }
    tft.print(")");
  }
  // print WiFi stat
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(0, 0);
  tft.print(n);
  tft.print(" networks found, suggested channels: ");
  bool listed_first_channel = false;
  for (int i = 1; i <= 14; i++) {                                                     // 
    if ((i == 1) || (max_rssi[i - 2] < NEAR_CHANNEL_RSSI_ALLOW)) {                    // check previous channel signal strengh
      if ((i == sizeof(channel_color)) || (max_rssi[i] < NEAR_CHANNEL_RSSI_ALLOW)) {  // check next channel signal strengh
        if (ap_count[i - 1] == 0) {                                                   // check no AP exists in same channel
          if (!listed_first_channel) {
            listed_first_channel = true;
          } else {
            tft.print(", ");
          }
          tft.print(i);
        }
      }
    }
  }
  // draw graph base axle
  tft.drawFastHLine(0, GRAPH_BASELINE, 320, TFT_WHITE);
  for (int i = 1; i <= 14; i++) {
    tft.setTextColor(channel_color[i - 1]);
    tft.setCursor((i * CHANNEL_WIDTH) - ((i < 10) ? 3 : 6), GRAPH_BASELINE + 2);
    tft.print(i);
    if (ap_count[i - 1] > 0) {
      tft.setCursor((i * CHANNEL_WIDTH) - ((ap_count[i - 1] < 10) ? 9 : 12), GRAPH_BASELINE + 11);
      tft.print('(');
      tft.print(ap_count[i - 1]);
      tft.print(')');
    }
  }
  delay(5000);
}


void scan() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  tft.setTextSize(2);
  int n = WiFi.scanNetworks();
  tft.fillScreen(ILI9341_BLACK);
  for (int i = 0; i < n; i++) {
    tft.setCursor(5, 20 * i);
    tft.print(i + 1);
    tft.print("|");
    tft.print(WiFi.SSID(i));
    tft.print("|");
    tft.print(WiFi.RSSI(i));
    tft.print("|");
    tft.print(WiFi.channel(i));
  }
  tft.setCursor(5, 225);
  tft.print(n);
  tft.print("  networks found");
  delay(1500);
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
}


byte pointer = 1;
byte str1 = 1;
byte str2 = 2;
void loop() {

  tft.setTextColor(TFT_GREEN);
  tft.setCursor(120, 2);
  tft.setTextSize(2);
  tft.print(" MENU ");
  tft.setCursor(5, 20);
  tft.print("1 Plot the map of AP's");
  tft.setCursor(5, 40);
  tft.print("2 Scan for networks ");




 //for (pointer; pointer )
  tft.setCursor(5, 110);
  tft.print("  ");
  tft.print(pointer);
  tft.print("  ");

  if ((pointer == str1) && butt2.click()) while (pointer > 0) plot();
  if ((pointer == str2) && butt2.click()) while (pointer > 0) scan();
  // up 25
  // down 27
  // middle 26pin

  if (butt1.click()) {
    Serial.println("up");
    pointer--;
    Serial.println(pointer);
    //tft.println("up");
  }
  if (butt2.click())  //
  {
    Serial.println("middle");
  }
  if (butt3.click()) {
    pointer++;
    Serial.println("down");
    Serial.println(pointer);
  }
}
