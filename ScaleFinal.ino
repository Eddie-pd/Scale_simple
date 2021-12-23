/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   Settling time (number of samples) and data filtering can be adjusted in the config.h file
   For calibration and storing the calibration value in eeprom, see example file "Calibration.ino"

   The update() function checks for new data and starts the next conversion. In order to acheive maximum effective
   sample rate, update() should be called at least as often as the HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS.
   If you have other time consuming code running (i.e. a graphical LCD), consider calling update() from an interrupt routine,
   see example file "Read_1x_load_cell_interrupt_driven.ino".

   This is an example sketch on how to use this library
*/

//Import library for lcd, and tell it what pins it will be using(initializing)
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); //Här skapas en datavariabel kallad liquidcrystal, som innehåller vilka pins som ska få vad senare.

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR) //Här kollar den om Arduinon har en "AVR" vilket är där jag har mitt EEPROM minne tror jag.
#include <EEPROM.h> //När den märker att en APR finns så hämtar den EEPROM bibblan.
#endif

//pins:
const int HX711_dout = 13; //mcu > HX711 dout pin 
const int HX711_sck = 9; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck); //Creating data variabel again like in liquidcrystal but for hx711.

const int calVal_eepromAdress = 0; //A variable of the adress where the calibrationvalue is store... don't really understand this one...
unsigned long t = 0; //Creating variable for time that will be used in determening when to print new datapoint.

void setup() {
  Serial.begin(57600); delay(10); //This
  Serial.println(); //This
  Serial.println("Starting...");  //This
//Start the lcd library and tell it the screen size. Also tiltswitch gives input to ardu. 
  lcd.begin(16, 2);

  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch, this is the variable for the calibraion value... 
#if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom, here we read a new calibrationvalue that is saved in the arduinos EEPROM. The values adress is 0 and so it start reading from there whatever that means and then finds the value in CalibrationValue which we needlessly defined beforehand in the code i think since it is saved in the eeprom already.

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step, Tare verkar innebära att man tar bort vikten som finns för att kalibrera.
  LoadCell.start(stabilizingtime, _tare); //Vet inte riktigt varför tare är inblandat här men kanske har att göra med att man vill ha 0 gram som värde i början. _tare är ju = True...
  if (LoadCell.getTareTimeoutFlag()) {  
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    //Serial.println("Startup is complete");
  }
}

void loop() {
  static boolean newDataReady = 0;  //A boolean I assume can only be 0 or 1, meaning not true or true. 
  const int serialPrintInterval = 500; //increase value to slow down serial print activity

  // check for new data/start next conversion: //When done set newDataReady to true so that the printing of values can happen.
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {  //If enough time has gone since the last new data was ready, bring in the new one.
      float i = LoadCell.getData();
      //Serial.print("Load_cell output val: ");
      //Serial.println(i);
      lcd.begin(16, 2);
      lcd.clear();
      lcd.print(i);
      lcd.print(" gram");
      newDataReady = 0;  //No longer any new data to deliver.
      t = millis(); //t is now millis until serialPrintInterval amount of time has gone.
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {  //I beilev this checks wether or not you have written anything in the serial command window.
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); //If what you have written is 't' then tareNoDelay happens. Tare means that thing when you put the cup cake form on and take away the value. I assume that means that pressing t will take away the weight of the cupcake.
  } //I CAN COFIRM THAT WHAT I WROTE UP HERE IS CORRECT CUZ WHEN I WROTE COMMAND 't' IT TOOK OF THE WEIGHT THAT WAS ALREADY ON FUCK YEAH.

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}
