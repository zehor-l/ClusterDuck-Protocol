#include <Arduino.h>
#include <RadioLib.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
unsigned long DELAY_TIME = 10000; // 1.5 sec
unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false; // true if still waiting for delay to finish
SX1276 lora = new Module(18, 26, 14, 25);
void setupDisplay()
{
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
}
// flag to indicate that a packet was received
volatile bool receivedFlag = false;
// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

void setup(){
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print(F("[SX1278] Initializing ... "));
  int state = lora.begin(915.0,125.0,7,7,20,1);
  lora.setDio0Action(setFlag);
  state = lora.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  setupDisplay();
  u8x8.drawString(0, 1, "FakePapa Online");
  delayStart = millis();   // start delay
  delayRunning = true; // not finished yet
}

void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }
  // we got a packet, set the flag
  receivedFlag = true;
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;
    // reset flag
    receivedFlag = false;
    // you can read received data as an Arduino String
    
    String str;
    int state = lora.readData(str);
    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = lora.receive(byteArr, 8);
    */
    if (state == ERR_NONE) {
    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[SX1278] CRC error!"));
    } else {
      // some other error occurred
      Serial.print(F("[SX1278] Failed, code "));
      Serial.println(state);
    }
    if (str.length() > 3) {
      String payMe = str + ";";
      Serial.println(payMe);
    }
    // put module back to listen mode
    lora.startReceive();
    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}
