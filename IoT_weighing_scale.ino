#include "IoT_weighing_scale.h"

uint8_t dataPin = 32;
uint8_t clockPin = 33;

HX711_ADC LoadCell(dataPin, clockPin);

float price;
const int calVal_eepromAdress = 0;
unsigned long t = 0;

// sim module
int simRXPin = 16;
int simTXPin = 17;
int simResetPin = 18;
// create an instance for serial communication to sim800l module
HardwareSerial sim800l(2);
// create an of adafruit_FONA library class
Adafruit_FONA gsm = Adafruit_FONA(simResetPin);

// buzzer
int buzzerPin = 23;

// processing indicator LED
int processPin = 19;

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// reset button
int resetPin = En;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

// set button manager and initialize buttons
int btnPin = 34;
int numberOfbuttons = 4;
BfButtonManager btnManager(btnPin, numberOfbuttons);
BfButton btn1(BfButton::ANALOG_BUTTON_ARRAY, 0);
BfButton btn2(BfButton::ANALOG_BUTTON_ARRAY, 1);
BfButton btn3(BfButton::ANALOG_BUTTON_ARRAY, 2);
BfButton btn4(BfButton::ANALOG_BUTTON_ARRAY, 3);

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
  Serial.print(btn->getID());
  switch (pattern)
  {
    case BfButton::SINGLE_PRESS:
    Serial.println("Button pressed.");
    //LoadCell.tareNoDelay();
    LoadCell.tare();
    // check if last tare operation is complete:
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
    }
    break;
    case BfButton::DOUBLE_PRESS:
    Serial.println("Button double pressed.");
    //LoadCell.tareNoDelay();
    LoadCell.tare();
    // check if last tare operation is complete:
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
    }
    break;
    case BfButton::LONG_PRESS:
    Serial.println("Button Long pressed.");
    //LoadCell.tareNoDelay();
    LoadCell.tare();
    // check if last tare operation is complete:
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
    }
    break;
  }
}

void smsHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
  Serial.print(btn->getID());
  if (pattern == BfButton::SINGLE_PRESS) {
    char sendto[21] = "09069025946";
    char message[141] = "Hello from digital weighing scale";
    gsm.sendSMS(sendto, message);
    delay(1000);
    Serial.println(" button pressed.");
    Serial.println("message sent.");
  }
}

void setup()
{
  Serial.begin(9600);
  sim800l.begin(9600);
  //rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  
  if(! gsm.begin(sim800l)) {
    Serial.println("Could not find sim800l");
  }

  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 832.10; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
  
  //integrate event handlers to buttons and add buttons to event manager
  btn1.onPress(smsHandler)
      .onDoublePress(pressHandler)     // default timeout
      .onPressFor(pressHandler, 1000); // custom timeout for 1 second
  btnManager.addButton(&btn1, 180, 230);
  btn2.onPress(pressHandler)
      .onDoublePress(pressHandler)     // default timeout
      .onPressFor(pressHandler, 1000); // custom timeout for 1 second
  btnManager.addButton(&btn2, 490, 550);
  btn3.onPress(pressHandler)
      .onDoublePress(pressHandler)     // default timeout
      .onPressFor(pressHandler, 1000); // custom timeout for 1 second
  btnManager.addButton(&btn3, 750, 850);
  btn4.onPress(pressHandler)
      .onDoublePress(pressHandler)     // default timeout
      .onPressFor(pressHandler, 1000); // custom timeout for 1 second
  btnManager.addButton(&btn4, 900, 1100);
  btnManager.begin();
  
  lcd.begin();// initialize LCD
  lcd.backlight();// turn on LCD backlight
  lcd.clear();
  Serial.println("Startup is complete");

  /*
  Serial.print("UNITS: ");
  Serial.println(scale.get_units(10));

  Serial.println("\nEmpty the scale, press a key to continue");
  while(!Serial.available());
  while(Serial.available()) Serial.read();

  scale.tare();
  Serial.print("UNITS: ");
  Serial.println(scale.get_units(10));


  Serial.println("\nPut 1000 gr in the scale, press a key to continue");
  while(!Serial.available());
  while(Serial.available()) Serial.read();

  scale.calibrate_scale(1000, 5);
  Serial.print("UNITS: ");
  Serial.println(scale.get_units(10));

  Serial.println("\nScale is calibrated, press a key to continue");
  while(!Serial.available());
  while(Serial.available()) Serial.read();

  
  
  scale.set_unit_price(20);  // we only have one price
  lcd.setCursor(0, 0);
  lcd.print("UNITS: ");
  lcd.setCursor(0, 1);
  lcd.print(": ");
  */
  
}


void loop()
{
  //int z;
  //z = analogRead(btnPin);
  //if (z > 100) lcd.print(z);
  //btnManager.printReading(btnPin);
  btnManager.loop();

  // setup serial transmission between serial 0 (serial monitor) and serial 2 (sim module)
  while(Serial.available()) {
    delay(1);
    sim800l.write(Serial.read());
  }
  while(sim800l.available()) {
    Serial.write(sim800l.read());
  }

  static boolean newDataReady = 0;
  const int serialPrintInterval = 100; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      //Serial.print("Load_cell output val: ");
      //Serial.println(i);
      lcd.clear();
      price = i * 20;
      
      //set display
      lcd.setCursor(0, 0);
      lcd.print("price: N");
      if(price < 0)
        lcd.print("0");
      else
        lcd.print(price, 1);
      lcd.setCursor(0, 1);
      if(i > 0)
        lcd.setCursor(1, 1);
      lcd.print(i, 1);
      lcd.print("g");

      newDataReady = 0;
      t = millis();
    }
  }
}

