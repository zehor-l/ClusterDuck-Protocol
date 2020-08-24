#include <ClusterDuck.h>
#include "AESLib.h"
AESLib aesLib;

ClusterDuck duck;

String message = "quackx3";

char cleartext[256];

// AES Encryption Key
byte aes_key[] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };

// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void aes_init() {
  Serial.println("gen_iv()");
  aesLib.gen_iv(aes_iv);
  Serial.println("encrypt()");
  Serial.println(encrypt(strdup(message.c_str()), message.length(), aes_iv));
}

String encrypt(char * msg, uint16_t msgLen, byte iv[]) {
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted[cipherlength]; // AHA! needs to be large, 2x is not enough
  aesLib.encrypt64(msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  Serial.print("encrypted = "); Serial.println(encrypted);
  return String(encrypted);
}

byte enc_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // iv_block gets written to, provide own fresh copy...

void setup() {
   
  duck.begin();
  duck.setDeviceId("E01");
  duck.setupMamaDuck();

  aes_init();  
  aesLib.set_paddingmode(paddingMode::Array);

}

void loop() {
  
  duck.runMamaDuck();
  Serial.println(message);
  Serial.println(message.length());
  sprintf(cleartext, "%s", message);

  // Encrypting
  Serial.println("Encrypting...");
  uint16_t clen = String(cleartext).length();
  String encrypted = encrypt(cleartext, clen, enc_iv);
  Serial.print("Ciphertext: ");
  Serial.println(encrypted);

  // Sending encrypted message through a LoRa packet
  duck.sendPayloadStandard(encrypted);
  
  for (int i = 0; i < 16; i++) {
    enc_iv[i] = 0;
  }
  delay(3000);
}
