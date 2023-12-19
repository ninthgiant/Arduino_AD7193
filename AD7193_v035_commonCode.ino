/*
 *   AD7193_SD_RTC_v03
 *   RAM, Sept. 1, 2023  
 * 
 *   Originally built from AD7193_VoltageMeasure_Example
 *   See my AD7193_Test (AD_1793_Test - actual name) for details of where this came from
 *
 *   NOTE: default pin assignment for PMOD AD7193 board MISO =10
 *          -- I changed it to 2 because SD shield uses PIN 10
 *            - that change is in the header file for AD7193 library; not in this file
 *   
 *   Sept. 1, 2023  
 *   - Add SD card to the app
 *   - setting file name using Date each time
 *   - Saving a time stamp each time slows it way down, so only save a counter
 *   - Saves data at about 68-70 samples per second;
 *   
 *   Dec 15, 2023
 *  - update so that it shares as much as possible with HX711 code - all but amplifier-specific code is common
 *  - use unsigned longs for data - simplifies life on back end
 *  - can't make AD9173 library work yet with Rev4 so this is ONLY for Rev3 UNO
 *
 *  On R3 Compile:
 *    Sketch uses 19478 bytes (60%) of program storage space. Maximum is 32256 bytes.
*     Global variables use 1673 bytes (81%) of dynamic memory, leaving 375 bytes for local variables. Maximum is 2048 bytes.
*/

#include <Wire.h> 

// headers for LCD
#include <LiquidCrystal_I2C.h>
// declare the lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#include <SPI.h>
#include <SD.h>  // interface with the SD card
String myFilename;  // for saving data

// #include <Time.h> // dealing with time stamps
#include "RTClib.h"  // this is the Jeelabs version through Adafruit
#include <TimeLib.h>;
RTC_DS1307 RTC;  // instantiate a real time clock

// Setup time variables that will be used in multiple places
DateTime currenttime;
long int myUnixTime;

// AD7193 library - doesn't yet work with R4
#include <AD7193.h>
AD7193 scale;
long int strain;

unsigned long tCounter = 0;  // Count # loops we to thru - must be global because don't want to initialize each time

// set constants
// initialize variables for SD -- use chipSelect = 4 without RTC board
const int chipSelect = 10;  // for the Wigoneer board and Adafruit board
// set the communications speed
const int comLevel = 115200;
// set the delay amount
const int dataDelay = 0;
// set flag for amount of feedback - false means to give us too much info
const bool verbose = true;
// set flag for printing to screen - could automate this when check for serial
const bool printScreen = true;
// set flag for saving to disk
const bool saveData = true;
// set flag for printing to LCD
const bool printLCD = true;
// set flag to use the RTC
const bool useRTC = true;
// set flag to use the RTC
const bool debug = false;
// set flag to use the RTC
const bool addStamp = true;


////////////////////
//  Get_Data - encapsulate data access to test different ideas - for now, very simple
////
unsigned long Get_Data() {
  return (scale.ReadADCChannel(1));
}

////////////////////
//  Get_TimeStamp - encapsulate data in case we change libraries
////
long int Get_TimeStamp() {
  /* GET CURRENT TIME FROM RTC */
  currenttime = RTC.now();

  // convert to raw unix value for return - could give options
  return (currenttime.unixtime());
}


///////////////////
// File name function - based on RTC time - so that we have a new filename for every day "DL_MM_DD.txt"
/////////
String rtnFilename() {

  DateTime currenttime;

  currenttime = RTC.now();

  int MM = currenttime.month();
  int DD = currenttime.day();

  String formattedDateTime = "DL_";
  formattedDateTime += (MM < 10 ? "0" : "") + String(MM) + "_";
  formattedDateTime += (DD < 10 ? "0" : "") + String(DD);
  formattedDateTime += ".txt";

  return formattedDateTime;
}



void setup() {

  ///////////////////////////
  // setup Serial and SPI
  ///////////////////////////

  Serial.begin(115200);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  ///////////////////////////
  // setup LCD
  ///////////////////////////
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Checking...");  // remove this when actually checking
  delay(1000);
  lcd.setCursor(0,0);
  lcd.print("                ");  // remove this when actually checking

  //
  ///////////////////////////
  // RTC - turn on and off with flag
  ///////////////////////////

  ///////////////////////////
  // RTC - Setup - turn on and off with flag
  ///////////////////////////
  if (useRTC) {  // must do this first so that we can use the GetFileName when we want it

    Serial.println("RTC setup");
    if (!RTC.begin()) {  // initialize RTC
      Serial.println("RTC failed");
    } else {
      if (!RTC.isrunning()) {
        Serial.println("RTC is NOT running!");
        // following line sets the RTC to the date & time this sketch was compiled
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
      } else {
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("RTC is already initialized.");
      }
    }
    // need to check to see if the clock is correct, or always update to internal clock
  }

  ///////////////////////////
  // setup SD Card - turn on and off with flag
  ///////////////////////////
  if (saveData) {

    if (debug) {
      myFilename = "Test_013.txt";  // use this if you want a custom name
      // myFilename = rtnFilename();  // use this if debugging and want to have the autonamed file - NEED RTC running to make it work
    } else {
      myFilename = rtnFilename();
    }

    // setup lcd for user feedback
    lcd.setCursor(0, 0);
    lcd.print("Initialize card ");

    // initialize the SD card process
    Serial.print("Initializing SD card...");

    delay(1000);

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");  // don't do anything more:
      lcd.print("                ");
      lcd.setCursor(1, 0);
      lcd.print("Card failed!");
      lcd.setCursor(1, 1);
      lcd.print("Disconnect!");
      delay(10000);
      // turn off the lcd
      lcd.noBacklight();
      lcd.noDisplay();


      while (1)
        ;
      Serial.println("card initialized.");  // confirm that it is good to go
    }                                       // end of checking for card and initializing

  }  // end of if SaveDATA



  ///////////////////////////////////
  // AD7193 Device setup
  ///////////////////////////////////

  scale.begin();

  //This will append status bits onto end of data - is required for library to work properly
  scale.AppendStatusValuetoData();

  // Set the gain of the PGA
  scale.SetPGAGain(128);

  // Set the Averaging
  scale.SetAveraging(3);  // was 3 originally - higher number slower sps


  //////////////////////////////////////
  // Inform the user of current status after checking if AD7193 is working - similar, but different from HX711 code
  /////////////////////////////////////

  bool lcGood = true;  // set flag
  // Check that Load Cell is working properly
  if (Get_Data() == Get_Data() == Get_Data()) {  //  all the same so is wrong
    Serial.println("Bad data");
    lcd.setCursor(0, 0);
    lcd.print("Load cell BAD! ");
    lcGood = false;
    delay(10000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Load cell works");
    delay(1000);

    Serial.println("Load cell working properly.");

    int N = 10;
    for (int i = 1; i < N; i++) {

      lcd.setCursor(0, 1);
      lcd.print("Start in: ");
      lcd.print(N - i);
      lcd.print(" s");

      delay(1000);
    }  // end of for loop

    lcd.setCursor(0, 1);
    lcd.print("Data:           ");

    
  }    // end of if lcGood, else

  // turn off the lcd?
  if (!printLCD) {
    lcd.noBacklight();
    lcd.noDisplay();
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Data:           ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }


}


void loop() {
  tCounter = tCounter + 1;
  unsigned long strain;

    // get a single time stamp to put on all 300 data points - not sure worth the speed to lose the resolution
  myUnixTime = Get_TimeStamp();
    // open the file to write 300 data points
  File dataFile = SD.open(myFilename, FILE_WRITE);

  if (dataFile) {

    for (int i = 0; i < 300; i++) {

      strain = Get_Data();
      

      if (addStamp) {  // only adds it to the saved data
        dataFile.print(strain);
        dataFile.print(", ");  // do we want to add the time stamp
        dataFile.println(myUnixTime);

        // datafile.println(currenttime.getUnixTime());  // for now, do it every time. let's see how it goes
      } else {
        dataFile.print(strain);
        dataFile.println(", ");  // do we want to add the time stamp
      }


      if (debug) {
        Serial.print(strain);
        if (i < 1) {
          Serial.print(",");
          Serial.println(tCounter);
        } else {
          Serial.print(",");
          Serial.println(Get_TimeStamp());
        }

      } else {
        Serial.println(strain);   //activate this only for testing wiht debug false
        // Serial.println(",");
      }  // end of if debug



    }  // end of for loop; data file being open

    // show user we are collecting data
    lcd.setCursor(6, 0);
    lcd.print(strain);  // do this while we are messing with closing the datafile

    dataFile.close();  // at the end, close before we start another loop


  } else {
    Serial.println("Error opening file ");  //  + myFile);  // if the file isn't open, pop up an error:
    lcd.setCursor(0, 0);
    lcd.print("Can't open file!!");  // do this while we are messing with closing the datafile
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("Figure it out!");  // do this while we are messing with closing the datafile
    delay(10000);
  }
}


// -- END OF FILE --
