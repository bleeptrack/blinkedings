#include <ESP8266WiFi.h>
#include <SPI.h>
#include <TFT_ILI9163C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <IRremoteESP8266.h>
extern "C" {
#include "user_interface.h"
}
#include <FS.h>
#include "rboot.h"
#include "rboot-api.h"

#include <Fonts/FreeSans12pt7b.h>

#define BNO055_SAMPLERATE_DELAY_MS (10)

#define VERSION 2


#define GPIO_LCD_DC 0
#define GPIO_TX     1
#define GPIO_WS2813 4
#define GPIO_RX     3
#define GPIO_DN     2
#define GPIO_DP     5

#define GPIO_BOOT   16
#define GPIO_MOSI   13
#define GPIO_CLK    14
#define GPIO_LCD_CS 15
#define GPIO_BNO    12

#define MUX_JOY 0
#define MUX_BAT 1
#define MUX_LDR 2
#define MUX_ALK 4
#define MUX_IN1 5

#define VIBRATOR 3
#define MQ3_EN   4
#define LCD_LED  5
#define IR_EN    6
#define OUT1     7

#define UP      711
#define DOWN    567
#define RIGHT   469
#define LEFT    950
#define OFFSET  20

#define I2C_PCA 0x25

#define NUM_LEDS    4

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF


TFT_ILI9163C tft = TFT_ILI9163C(GPIO_LCD_CS, GPIO_LCD_DC);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, GPIO_WS2813, NEO_GRB + NEO_KHZ800);


byte portExpanderConfig = 0; //stores the 74HC595 config



#define COLORMAPSIZE 32
int joystick = 0; //Jostick register

int count = 3;
int del = 1000;
int pixelmap[5] = {0,1,4,3,2};
boolean gameover = false;
int score = 0;

void setup() {
  initBadge();
  rboot_config rboot_config = rboot_get_config();
  SPIFFS.begin();
  File f = SPIFFS.open("/rom"+String(rboot_config.current_rom),"w");
  f.println("Blinkedings\n");
  pixels.setBrightness(20);

 
}

boolean simon = true;
int playerCount = 1;
int zahlen[20];
int level = 1;

void loop() {
  if(gameover){
    tft.fillScreen(BLACK);
    tft.setCursor(0,0);
    tft.println("Blinkedings!");
    tft.setCursor(0,20);
    tft.print("GAMEOVER :(");
    tft.setCursor(0,40);
    tft.print("Punkte: ");
    tft.print(score);
    tft.writeFramebuffer();
  }else{
    tft.fillScreen(BLACK);
    tft.setCursor(0,0);
    tft.println("Blinkedings!");
    tft.setCursor(0,20);
    tft.print("Level: ");
    tft.print(level);
    tft.setCursor(0,40);
    tft.print("Punkte: ");
    tft.print(score);
    tft.writeFramebuffer();
    if(simon){
      for(int i = 0; i<count;i++){
        zahlen[i] = random(1,5);
        blink(zahlen[i]);
        delay(500);
      }
      vibr();
      simon = false;
    }else{
      if(playerCount<=count){
        joystick = getJoystick();
        if(joystick>0 && joystick<5){
          blink(pixelmap[joystick]);
          if(pixelmap[joystick] != zahlen[playerCount-1]){
            
            tft.println("FALSCH!");
            tft.writeFramebuffer();
            gameover = true;
          }else{
            playerCount++;
          }
        }
      }else{
        simon = true;
        playerCount = 1;
        levelUp();
        vibr();
      }
    }
  }
  
}

void levelUp(){
  score = score + count*level;
  if(level%3==0){
    count++;
  }
  if(level%2==0 && del>100){
    del = del-100;
  }
  level++;
}

void pxlOver(){
  pixels.setPixelColor(0, 0, 0, 0);
  pixels.setPixelColor(2, 0, 0, 0);
  pixels.setPixelColor(1, 0, 0, 0);
  pixels.setPixelColor(3, 0, 0, 0);
  pixels.show();
}

void blink(int i){
  pxlClear();
  switch(i){
    case 1:
      pixels.setPixelColor(i-1, 0, 255, 0);
      break;
    case 2:
      pixels.setPixelColor(i-1, 0, 0, 255);
      break;
    case 3:
      pixels.setPixelColor(i-1, 255, 0, 0);
      break;
    case 4:
      pixels.setPixelColor(i-1, 100, 100, 0);
      break;      
  }
  pixels.show();
  delay(del);
  pxlClear();

}

void vibr(){
  setGPIO(VIBRATOR, 1);
  delay(60);
  setGPIO(VIBRATOR, 0);
}

void pxlClear(){
  pixels.setPixelColor(0, 0, 0, 0);
  pixels.setPixelColor(2, 0, 0, 0);
  pixels.setPixelColor(1, 0, 0, 0);
  pixels.setPixelColor(3, 0, 0, 0);
  pixels.show();
}


int getJoystick() {
  uint16_t adc = analogRead(A0);
  /*tft.fillScreen(BLACK);
  tft.setCursor(0,80);
    tft.println(adc);
    tft.writeFramebuffer();*/

  if (adc < UP + OFFSET && adc > UP - OFFSET)             return 1;
  else if (adc < DOWN + OFFSET && adc > DOWN - OFFSET)    return 2;
  else if (adc < RIGHT + OFFSET && adc > RIGHT - OFFSET)  return 3;
  else if (adc < LEFT + OFFSET && adc > LEFT - OFFSET)    return 4;
  if (digitalRead(GPIO_BOOT) == 1) return 5;
  return 0;
}

void setGPIO(byte channel, boolean level) {
  bitWrite(portExpanderConfig, channel, level);
  Wire.beginTransmission(I2C_PCA);
  Wire.write(portExpanderConfig);
  Wire.endTransmission();
}

void setAnalogMUX(byte channel) {
  portExpanderConfig = portExpanderConfig & 0b11111000;
  portExpanderConfig = portExpanderConfig | channel;
  Wire.beginTransmission(I2C_PCA);
  Wire.write(portExpanderConfig);
  Wire.endTransmission();
}







void initBadge() { //initialize the badge

  pinMode(GPIO_BOOT, INPUT_PULLDOWN_16);  // settings for the leds
  pinMode(GPIO_WS2813, OUTPUT);

  pixels.begin(); //initialize the WS2813
  pixels.clear();
  pixels.show();

  Wire.begin(9, 10); // Initalize i2c bus
  Wire.beginTransmission(I2C_PCA);
  Wire.write(0b00000000); //...clear the I2C extender to switch off vibrator and backlight
  Wire.endTransmission();

  delay(100);

  tft.begin(); //initialize the tft. This also sets up SPI to 80MHz Mode 0
  tft.setRotation(2); //turn screen
  tft.scroll(32); //move down by 32 pixels (needed)
  tft.fillScreen(BLACK);  //make screen black
  tft.writeFramebuffer();
  setGPIO(LCD_LED, HIGH);

  pixels.clear(); //clear the WS2813 another time, in case they catched up some noise
  pixels.show();
}
