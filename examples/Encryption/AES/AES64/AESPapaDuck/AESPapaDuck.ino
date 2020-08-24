#include <ClusterDuck.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "timer.h"
#include "AESLib.h"

#define SSID        ""
#define PASSWORD    ""

#define ORG         ""
#define DEVICE_ID   ""
#define DEVICE_TYPE ""
#define TOKEN       ""

char server[]           = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[]       = "use-token-auth";
char token[]            = TOKEN;
char clientId[]         = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

ClusterDuck duck;

AESLib aesLib;
char ciphertext[512];

// AES Encryption Key
byte aes_key[] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };

// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

byte dec_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

auto timer = timer_create_default(); // create a timer with default settings

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);

byte ping = 0xF4;
bool retry = true;

void setup() {
  // put your setup code here, to run once:

  duck.begin();
  duck.setDeviceId("Papa");

  duck.setupLoRa();
  duck.setupDisplay("Papa");

  const char * ap = "PapaDuck Setup";
  duck.setupWifiAp(ap);
  duck.setupDns();

  duck.setupInternet(SSID, PASSWORD);
//  duck.setupWebServer();

  Serial.println("PAPA Online");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(WiFi.status() != WL_CONNECTED && retry)
  {
    Serial.print("WiFi disconnected, reconnecting to local network: ");
    Serial.print(duck.getSSID());
    if(duck.ssidAvailable()) {
      duck.setupInternet(duck.getSSID(), duck.getPassword());
      duck.setupDns();
    } else {
      retry = false;
      timer.in(5000, enableRetry);
    }
    
  }
  if(WiFi.status() == WL_CONNECTED) setupMQTT();

  if(duck.getFlag()) {  //If LoRa packet received
    duck.flipFlag();
    duck.flipInterrupt();
    int pSize = duck.handlePacket();
    if(pSize > 3) {
      duck.getPacketData(pSize);
      quackJson();

    }
    duck.flipInterrupt();
    duck.startReceive();
  }


  timer.tick();
}

void setupMQTT()
{
  if (!!!client.connected()) {
    Serial.print("Reconnecting client to "); Serial.println(server);
    while ( ! (ORG == "quickstart" ? client.connect(clientId) : client.connect(clientId, authMethod, token)))
    {
      timer.tick(); //Advance timer to reboot after awhile
      Serial.print("i");
      delay(500);
    }
  }
}

void quackJson() {
  const int bufferSize = 4*  JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  JsonObject root = doc.as<JsonObject>();

  Packet lastPacket = duck.getLastPacket();

  uint16_t dlen = lastPacket.payload.length();
  sprintf(ciphertext, "%s", lastPacket.payload.c_str());
  String decrypted = decrypt(ciphertext, dlen, dec_iv);

  doc["DeviceID"]        = lastPacket.senderId;
  doc["MessageID"]       = lastPacket.messageId;
  doc["Payload"]     .set(decrypted);
  doc["path"]         .set(lastPacket.path + "," + duck.getDeviceId());

  String loc = "iot-2/evt/"+ lastPacket.topic +"/fmt/json";
  Serial.print(loc);
  int len = loc.length();

  char topic[len];
  loc.toCharArray(topic, len);

  String jsonstat;
  serializeJson(doc, jsonstat);

  if (client.publish(topic, jsonstat.c_str())) {

    serializeJsonPretty(doc, Serial);
     Serial.println("");
    Serial.println("Publish ok");

  }
  else {
    Serial.println("Publish failed");
  }

}

bool enableRetry(void *) {
  retry = true;
}

String decrypt(char * msg, uint16_t msgLen, byte iv[]) {
  char decrypted[msgLen];
  aesLib.decrypt64(msg, msgLen, decrypted, aes_key, sizeof(aes_key), iv);
  return String(decrypted);
}
