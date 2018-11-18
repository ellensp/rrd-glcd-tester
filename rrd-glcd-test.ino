#include <SPI.h>
#include <U8glib.h>
#include <SD.h>

//Lets you test the standard features of a reprap discount full graphics 128x64 LCD
//Test STOP and Encoder button
//Test buzzer (Hold both STOP and Encoder buttons to activate)
//Test SD detect, disply SD card information
//Test Encode left/right
//Verify the LCD is working.

//Standard pins when on a RAMPS 1.4

#define DOGLCD_CS       16
#define DOGLCD_MOSI     17
#define DOGLCD_SCK      23
#define BTN_EN1         31
#define BTN_EN2         33
#define BTN_ENC         35
#define SD_DETECT_PIN   49
#define SDSS            53
#define BEEPER_PIN      37
#define KILL_PIN        41

Sd2Card card;
SdVolume volume;


int x=0;                                //Offset postion of title  
int kill_pin_status = 1;                //Last read status of the stop pin, start at 1 to ensure buzzer is off
int encoderPos = 1;                     //Current encoder position
int encoder0PinALast;                   //Used to decode rotory encoder, last value
int encoder0PinNow;                     //Used to decode rotory encoder, current value
char posStr[4];                         //Char array to store encoderPos as a string  
char tmp_string[16];
int enc_pin_status;                     //Last read status of the encoder button
int sd_detect_pin_status = true;               //Last read status of the SD detect pin
int scroll_direction=1;                 //Direction of title scroll, 1 right, -1 left
unsigned long previousMillis = 0;       //Previous Millis value
unsigned long currentMillis;            //Current Millis value
const long interval = 1000/3;           //How often to run the display loop, every 1/3 of a second aproximatly 
boolean gotsddata = false;

int sdcardinit;
int sdcardtype;
int sdvolumeinit;
int sdvolumefattype;
unsigned long sdvolumebpc;
unsigned long sdvolumecc;


// SPI Com: SCK = en = 23, MOSI = rw = 17, CS = di = 16
U8GLIB_ST7920_128X64_1X u8g(DOGLCD_SCK, DOGLCD_MOSI, DOGLCD_CS);

void setup() {  
  pinMode(SD_DETECT_PIN, INPUT);        // Set SD_DETECT_PIN as an unput
  digitalWrite(SD_DETECT_PIN, HIGH);    // turn on pullup resistors
  pinMode(KILL_PIN, INPUT);             // Set KILL_PIN as an unput
  digitalWrite(KILL_PIN, HIGH);         // turn on pullup resistors
  pinMode(BTN_EN1, INPUT);              // Set BTN_EN1 as an unput, half of the encoder
  digitalWrite(BTN_EN1, HIGH);          // turn on pullup resistors
  pinMode(BTN_EN2, INPUT);              // Set BTN_EN2 as an unput, second half of the encoder
  digitalWrite(BTN_EN2, HIGH);          // turn on pullup resistors
  pinMode(BTN_ENC, INPUT);              // Set BTN_ENC as an unput, encoder button
  digitalWrite(BTN_ENC, HIGH);          // turn on pullup resistors
  u8g.setFont(u8g_font_helvR08);        //Set the font for the display
  u8g.setColorIndex(1);                 // Instructs the display to draw with a pixel on. 
}

//Main arduino loop
void loop() {
  // Read the encoder and update encoderPos    
  encoder0PinNow = digitalRead(BTN_EN1);
  if ((encoder0PinALast == LOW) && (encoder0PinNow == HIGH)) {
    if (digitalRead(BTN_EN2) == LOW) {
      encoderPos++;
    } else {
      encoderPos--;
    }
  }
  encoder0PinALast = encoder0PinNow;

  
  //read in SD data when SD is incerted
  if (!sd_detect_pin_status && !gotsddata) {
      sdcardinit = card.init(SPI_HALF_SPEED, SDSS);
      sdcardtype = card.type();
      sdvolumeinit = volume.init(card);
      sdvolumefattype = volume.fatType();
      sdvolumebpc = volume.blocksPerCluster();
      sdvolumecc = volume.clusterCount();
      gotsddata = true;
  }

  //check if it is time to update the display 
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    //read the kill pin status
    kill_pin_status = digitalRead(KILL_PIN); 
    //read the encoder button status
    enc_pin_status = digitalRead(BTN_ENC);
    //read the SD detect pin status  
    sd_detect_pin_status = digitalRead(SD_DETECT_PIN);
    if (sd_detect_pin_status) {
      gotsddata = false;
    }
  
    //Check if both Kill switch and encoder are pressed, if so switch on buzzer
    if(kill_pin_status || enc_pin_status) digitalWrite(BEEPER_PIN, LOW); 
    else digitalWrite(BEEPER_PIN, HIGH);

    //Draw new screen
    u8g.firstPage();
    do {  
      if(gotsddata) draw2();
      else draw();
    } while( u8g.nextPage() );

    //Update Title position
    x=x+scroll_direction;
    if (x > 40) scroll_direction = -1;
    if (x < 1) scroll_direction = 1;
  }
}

//Assemble the display  
void draw(){
  u8g.setColorIndex(0);
  u8g.drawBox(0,0,128,64);
  u8g.setColorIndex(1);
  
  u8g.drawStr( 2+x, 10, "RRD GLCD TEST");
  u8g.drawStr( 2, 3*9, "Stop pin status:");
  if (kill_pin_status) u8g.drawStr( 84, 3*9, "Open");
  else u8g.drawStr( 84, 3*9, "Closed");

  u8g.drawStr( 2, 4*9, "Enc pin status:");
  if (enc_pin_status) u8g.drawStr( 84, 4*9, "Open");
  else u8g.drawStr( 84, 4*9, "Closed");

  u8g.drawStr( 2, 6*9, "Encoder value:");
  sprintf (posStr, "%d", encoderPos);
  u8g.drawStr( 84, 6*9, posStr );
  
  u8g.drawStr( 2, 5*9, "SD detect status:");
  if (sd_detect_pin_status) u8g.drawStr( 84, 5*9, "Open");
  else u8g.drawStr( 84, 5*9, "Closed");
  
  
  u8g.drawStr( 2, 7*9, "Buzzer:");
  if (kill_pin_status || enc_pin_status) u8g.drawStr( 84, 7*9, "Off");
  else u8g.drawStr( 84, 7*9, "On");

  u8g.drawFrame(0,0,128,64);
}

//Display SD card info
void draw2(){
  u8g.setColorIndex(0);
  u8g.drawBox(0,0,128,64);
  u8g.setColorIndex(1);
  
  u8g.drawStr( 0, 1*10, "Initializing SD card:");
  if (sdcardinit) {
      u8g.drawStr( 90, 1*10, "OK");
      u8g.drawStr( 0, 2*10, "Card type:");
      switch (sdcardtype) {
        case SD_CARD_TYPE_SD1:
          u8g.drawStr( 90, 2*10, "SD1");
          break;
        case SD_CARD_TYPE_SD2:
          u8g.drawStr( 90, 2*10, "SD2");
          break;
        case SD_CARD_TYPE_SDHC:
          u8g.drawStr( 90, 2*10, "SDHC");
          break;
        default:
          u8g.drawStr( 90, 2*10, "Unknown");
      }
      if (!sdvolumeinit) {
        u8g.drawStr( 0, 3*10, "No FAT16/FAT32 partition.");
      } else {

        // print the type and size of the first FAT-type volume
        u8g.drawStr( 0, 3*10,"Volume type is:       FAT");
        itoa(sdvolumefattype, tmp_string, 10);
        u8g.drawStr( 110, 3*10,tmp_string);

        uint32_t volumesize;
        volumesize = sdvolumebpc;    // clusters are collections of blocks
        volumesize *= sdvolumecc;       // we'll have a lot of clusters
        volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
        volumesize /= 1024;
        
        u8g.drawStr( 0, 4*10,"Volume size (Mb):");
        itoa(volumesize, tmp_string, 10);
        u8g.drawStr( 90, 4*10,tmp_string);
      }
  } else {
      u8g.drawStr( 90, 1*10, "FAILED");
  }
}
