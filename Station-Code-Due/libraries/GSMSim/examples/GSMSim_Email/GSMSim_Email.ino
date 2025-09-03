/*
 * GSMSimEmail Example
 * 
 * GSMSim_Email.ino
 *
 * By Erdem ARSLAN
 * Version: v.2.0.1
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/


/*
  ### Example Serial Output ###

 	Set Phone Function... 1
  is Module Registered to Network?... 1
  Signal Quality... 12
  Operator Name... Turk Telekom
  Connect GPRS... 1
  Get IP Address... xxx.xxx.xxx.xxx
  Set GMail... 1
  Write Email... OK
  Send Email... SUCCESS:EMAIL_SEND
  Close GPRS... 1



*/

#include <GSMSimEmail.h>

// You can use any Serial interface. I recommended HardwareSerial. Please use the library with highiest baudrate.
// In examples, i used HardwareSerial. You can change it anymore.

#define RESET_PIN 10 // you can use any pin.

static volatile int num = 0;

GSMSimEmail email(Serial1, RESET_PIN); // GSMSimEmail inherit from GSMSimGPRS. You can use GSMSim and GSMSimGPRS methods with it.

void setup() {
  Serial1.begin(115200); // If you dont change module baudrate, it comes with auto baudrate.

  while(!Serial1) {
    ; // wait for module for connect.
  }

  Serial.begin(115200); // Serial for debug...

  // Init module...
  email.init(); // use for init module. Use it if you dont have any valid reason.

  Serial.print("Set Phone Function... ");
  Serial.println(email.setPhoneFunc(1));
  //delay(1000);

  Serial.print("is Module Registered to Network?... ");
  Serial.println(email.isRegistered());
  //delay(1000);

  Serial.print("Signal Quality... ");
  Serial.println(email.signalQuality());
  //delay(1000);

  Serial.print("Operator Name... ");
  Serial.println(email.operatorNameFromSim());
  //delay(1000);


  //Serial.print("GPRS Init... ");
  //Serial.println(email.gprsInit("internet")); // Its optional. You can set apn, user and password with this method. Default APN: "internet" Default USER: "" Default PWD: ""
  //delay(1000);


  Serial.print("Connect GPRS... ");
  Serial.println(email.connect());
  //delay(1000);

  Serial.print("Get IP Address... ");
  Serial.println(email.getIP());
  //delay(1000);
  
  // you cannot use @gmail.com for username
  // If you used 2-step verification, create an app password and use it in here. No other settings required.
  // Help for create an app password please check: https://support.google.com/accounts/answer/185833?hl=en&ctx=ch_b%2F0%2FDisplayUnlockCaptcha
  Serial.print("Set GMail... ");
  Serial.println(email.gmail("your_gmail_username", "password"));
  //delay(1000);

  Serial.print("Write Email... ");
  Serial.println(email.write("your_gmail_address@gmail.com", "anybody_email_address_to_send@xxxxxxx.tdl", "Subject", "Mail Body"));
  //delay(1000);

  Serial.print("Send Email... ");
  Serial.println(email.send());
  //delay(1000);

  Serial.print("Close GPRS... ");
  Serial.println(email.closeConn());
  //delay(1000);

  // For other methods please look at readme.txt file.

}

void loop() {
  
  // Use your Serial interface...
  if(Serial1.available()) {
      String buffer = "";
      buffer = Serial1.readString();
      num = num + 1;
      Serial.print(num);
      Serial.print(". ");
      Serial.println(buffer);
  }
  
  // put your main code here, to run repeatedly:
}