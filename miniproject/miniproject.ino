/*
 * Sahachai Teeranantagul
 * 6210110359
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "Timer3CTC.h"
#include "Timer4CTC.h"

#define DEBUG 0

#if DEBUG == 1
#define debug(x)   Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

#define STATE_START          0
#define STATE_ENTER_PASSWORD 1
#define STATE_THINGS_DETECT  2
#define STATE_PASSWORD_CHECK 3
#define STATE_CAL_PRICE      4
#define STATE_RESTART        5
#define SEV_SEG_DASH         0x40
#define LED_RED_ON           0xFE
#define LED_YELLOW_ON        0xFD
#define LED_GREEN_ON         0xFB
#define LED_ALL_ON           0xF0
#define LED_ALL_OFF          0xFF

#define BUZZER               14

LiquidCrystal_I2C lcd(0x27,16,2);
Timer3CTC timer7seg;
Timer4CTC timerLight;

typedef struct Products {
  uint8_t product_id;
  unsigned int price;
} Product;

Product products[15] = {  // example products
                          {25, 200},  {11, 700},  {12, 650},
                          {13, 1500}, {14, 150},  {15, 800},
                          {16, 1200}, {17, 900},  {18, 500},
                          {26, 400},  {27, 1000}, {21, 1200},
                          {22, 750},  {23, 1350}, {24, 750}
                       };

uint8_t digit[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

uint8_t _1stDigit;
uint8_t _2ndDigit;
uint8_t _3rdDigit;
volatile uint8_t timerCount = 0;
volatile uint8_t state;
uint16_t num;
uint16_t light;
uint16_t lightINT;
bool idle, detect;
volatile bool resetFlag;
uint8_t passwordDigit[3];
uint8_t roundProducts, totalProducts;
const uint16_t myPassword = 123;
char dataBuffer[16];
int totalPrice = 0;

void _7segmentSetup() {
  DDRA = 0xFF;
  DDRL = 0x0F;
  PORTA = 0;
  PORTL = 0b11111110;
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Program begins..");
  lcd.setCursor(0,1);
  lcd.print("Your Welcome...");
  _7segmentSetup();
  DDRB = 0x0F;
  PORTB |= 0x0F;
  noTone(BUZZER);
  noInterrupts();
  timer7seg.initialize(5);  // every 5 ms
  timerLight.initialize(200); // every 200 ms
  timer7seg.attachInterrupt(disp7seg);  // ISR Timer 3
  timerLight.attachInterrupt(waitingTimer); // ISR Timer 4
  attachInterrupt(digitalPinToInterrupt(3), IR_detect, FALLING);  // ISR External interrupt INT5
  attachInterrupt(digitalPinToInterrupt(2), _reset, FALLING); // ISR External interrupt INT4
  interrupts();
//  Serial.println(products[0].product_id);
  num = 123;
  light = 0;
  roundProducts = 0;
  lightINT = 0;
  totalProducts = 0;
  idle = true;
  detect = false;
  resetFlag = false;
  state = STATE_START;
  timer7seg.start();  
}

void loop() {
  switch (state) {
    case STATE_START:
      PORTB = LED_RED_ON;
      light = analogRead(A15);
      delay(10);
      if (light >= 700) {
        state = STATE_ENTER_PASSWORD;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Enter password");
        lcd.setCursor(0,1);
        lcd.blink();
        PORTB = LED_YELLOW_ON;
        timerLight.start();
      }
    break;
    case STATE_RESTART:
      debugln("restart");
      detect = false;
      idle = true;
      resetFlag = false;
      roundProducts = 0;
      timerLight.stop();
      lcd.clear();
      lcd.noBlink();
      lcd.setCursor(0,0);
      lcd.print("Program begins..");
      lcd.setCursor(0,1);
      lcd.print("Your Welcome...");
      state = STATE_START;
    break;
    case STATE_ENTER_PASSWORD:
      debugln("from enter pass");
      lcd.setCursor(0,1);
      for (int index = 0; index < 3; index++) {
        passwordDigit[index] = readKeypadSingleDigit();
        delay(10);
        if (resetFlag) break;
        lcd.print("*");
      }
      if (resetFlag) break;
      else state = STATE_PASSWORD_CHECK;
    break;
    case STATE_PASSWORD_CHECK:
      debugln("from check pass");
      lcd.clear();
      lcd.setCursor(0,0);
      if (passwordCombine(passwordDigit) == myPassword) {
        state = STATE_THINGS_DETECT;
        PORTB = LED_GREEN_ON;
        detect = true;
        lcd.noBlink();
        lcd.clear();
        lcd.setCursor(0,0);
        sprintf(dataBuffer, "Total: %3d", totalProducts);
        lcd.print(dataBuffer);
        lcd.setCursor(0,1);
        sprintf(dataBuffer, "TotalPrice:%4d", totalPrice);
        lcd.print(dataBuffer);
        idle = false;
        splitDigit(roundProducts);
      } else {
        state = STATE_ENTER_PASSWORD;
        idle = true;
        lcd.print("Wrong!!! again");
        PORTB = LED_RED_ON;
        delay(1000);
        PORTB = LED_YELLOW_ON;
      }
    break;
    case STATE_THINGS_DETECT:
      debugln("this is detect");
//      Serial.println(readDistance());
      if ((PINK & 0x1) == 0 && roundProducts != 0) {
        state = STATE_CAL_PRICE;
        detect = false;
        splitDigit(totalProducts);
        PORTB = LED_YELLOW_ON;
      }
    break;
    case STATE_CAL_PRICE:
      debugln("from cal price");
      int productID[roundProducts];
      uint8_t productIdDigit[2];
      uint8_t constRountProduct = roundProducts;
      bool noProduct;
      for (int product = 0; product < constRountProduct; product++) { 
        noProduct = true;
        PORTB = LED_YELLOW_ON;
        lcd.clear();
        lcd.setCursor(0,0);
        sprintf(dataBuffer, "Product: %3d", roundProducts);
        lcd.print(dataBuffer);
        lcd.setCursor(0,1);
        lcd.print("Enter ID: ");
        lcd.blink();
        for (int idDigit = 0; idDigit < 2; idDigit++) {
          productIdDigit[idDigit] = readKeypadSingleDigit();
          if (resetFlag) break;
          lcd.setCursor(idDigit+10,1);
          lcd.print(productIdDigit[idDigit]);
        }
        lcd.noBlink();
        while (PINK & 0x2) { if (resetFlag) break; }
        if (resetFlag) break;
        
        productID[product] = idProductCombine(productIdDigit);
        for (int index = 0; index < 15; index++) {
          if (productID[product] == products[index].product_id) {
            roundProducts = roundProducts - 1;
            totalPrice = totalPrice + products[index].price;
            totalProducts = totalProducts + 1;
            splitDigit(totalProducts);
            noProduct = false;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Success");
            PORTB = LED_GREEN_ON;
            delay(1000);
            break;
          }
        }
        if (noProduct) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("No Product");
          product--;
          PORTB = LED_RED_ON;
          delay(1000);
          PORTB = LED_YELLOW_ON;
        }
//        Serial.println(productID[product]);
      }
      if (resetFlag) {        
        break;
      }
      else {
        state = STATE_THINGS_DETECT;
        lcd.clear();
        lcd.noBlink();
        lcd.setCursor(0,0);
        sprintf(dataBuffer, "Total: %3d", totalProducts);
        lcd.print(dataBuffer);
        lcd.setCursor(0,1);
        sprintf(dataBuffer, "TotalPrice:%4d", totalPrice);
        lcd.print(dataBuffer);
        detect = true;
        PORTB = LED_GREEN_ON;
        roundProducts = 0;
        splitDigit(roundProducts);
      }
    break;
    default:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("error");
      debugln("from default");
    break;
  }
}

void _reset() { // ISR External interrupt INT4
  state = STATE_RESTART;
  resetFlag = true;
}

void IR_detect() {  // ISR External interrupt INT5
  if (detect) {
    roundProducts++;
    splitDigit(roundProducts);
  }
}

void waitingTimer() { // ISR Timer 4 (every 200 ms)
  timerCount++;
  lightINT = lightINT + analogRead(A15);
  if (timerCount == 15) {
    if ((lightINT / 15) < 700) {
      state = STATE_RESTART;
      resetFlag = true;
    }
    timerCount = 0;
    lightINT = 0;
  }
}

void splitDigit(int _num) {
  _1stDigit = _num / 100;
  _2ndDigit = (_num % 100) / 10;
  _3rdDigit = (_num % 100) % 10;
}

void disp7seg() { // ISR Timer 3 (every 5 ms)
  if (~PORTL & 0b0100) {
    PORTL = 0b11111110;
    if (idle) PORTA = SEV_SEG_DASH;
    else PORTA = digit[_1stDigit];
  } else if (~PORTL & 0b0001) {
    PORTL = 0b11111101;
    if (idle) PORTA = SEV_SEG_DASH;
    else PORTA = digit[_2ndDigit];
  } else if (~PORTL & 0b0010) {
    PORTL = 0b11111011;
    if (idle) PORTA = SEV_SEG_DASH;
    else PORTA = digit[_3rdDigit];
  }
}

uint16_t passwordCombine(uint8_t num[3]) {
  return (num[0] * 100) + (num[1] * 10) + num[2];
}

uint8_t idProductCombine(uint8_t _id[2]) {
  return (_id[0] * 10) + _id[1];
}

uint16_t readKeypadSingleDigit(void) {
  uint8_t keypad;
  uint8_t password;
  uint8_t count;
  bool doneFlag = false;
  uint8_t index, index2;
  count = 0;
  while (doneFlag == false) {
    if (PIND & 0x80) { // wait for DV signal to go high
      tone(BUZZER, 200);
      keypad = PINC; // read 8-bit input
      doneFlag = true; // finish getting user input
      delay(100);
      while (PIND & 0x80) {} // wait for DV signal to go low
      noTone(BUZZER);
    } else 
    if (resetFlag) doneFlag = true;
  }

  count = 0;
  // convert each keypad input to number
  for (index = 0; index <= 7; index++) {
    if (keypad & (1 << index)) {
      count++;
      password = index + 1;
    }
  }

  if (count > 1) {
    password = 0;
  }
  return (password);
}
