#include <LCDI2C_Multilingual.h>

LCDI2C_Generic lcd(0x27, 20, 4);  // I2C address: 0x27; Display size: 20x4

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
}

void loop() {
  // when characters arrive over the serial port...
  if (Serial.available()) {
    // wait a bit for the entire message to arrive
    delay(500);
    // clear the screen
    lcd.clear();
    // read all the available characters
    while (Serial.available() > 0) {
      // display each character to the LCD
      lcd.write(Serial.read());
      Serial.println(Serial.read());
    }
  }
}