// 6 Channel Receiver 
// Swapnil Nimbalkar
// The DC Minds
// + iBUS output added on a separate pin (D10), PWM outputs untouched

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <SoftwareSerial.h>

int ch_width_1 = 0;
int ch_width_2 = 0;
int ch_width_3 = 0;
int ch_width_4 = 0;
int ch_width_5 = 0;
int ch_width_6 = 0;

Servo ch1;
Servo ch2;
Servo ch3;
Servo ch4;
Servo ch5;
Servo ch6;

struct Signal {
byte throttle;
byte yaw;        
byte pitch;
byte roll;
byte aux1;
byte aux2;
};

Signal data;
const uint64_t pipeIn = 0xE9E8F0F0E1LL;
RF24 radio(7, 8); 

// ---------------- iBUS output ----------------
// Servo library already owns Timer1 for the 6 PWM channels above, so iBUS is
// generated with SoftwareSerial (pure bit-banged, no hardware timer needed)
// instead of PPM, which would need Timer2 chained in 8-bit steps to reach
// 1000-2000us pulses - workable, but more fragile to get right on a 328P.
// This way PWM and iBUS stay fully independent data paths.
#define IBUS_TX_PIN    10   // -> FC UART RX pin (set that UART's protocol to IBUS)
#define IBUS_RX_PIN    A0   // unused, SoftwareSerial constructor just needs a pin
#define IBUS_CHANNELS  14
#define IBUS_INTERVAL  7    // ms between frames, matches real FlySky iBUS rate

SoftwareSerial ibusSerial(IBUS_RX_PIN, IBUS_TX_PIN);
uint16_t ibusChannels[IBUS_CHANNELS];
unsigned long lastIbusTime = 0;

void sendIBUS()
{
  uint8_t packet[32];
  packet[0] = 0x20;
  packet[1] = 0x40;

  for (uint8_t i = 0; i < IBUS_CHANNELS; i++) {
    packet[2 + i * 2] = ibusChannels[i] & 0xFF;
    packet[3 + i * 2] = (ibusChannels[i] >> 8) & 0xFF;
  }

  uint16_t checksum = 0xFFFF;
  for (uint8_t i = 0; i < 30; i++) {
    checksum -= packet[i];
  }
  packet[30] = checksum & 0xFF;
  packet[31] = (checksum >> 8) & 0xFF;

  ibusSerial.write(packet, 32);
}
// -----------------------------------------------

void ResetData()
{
data.roll = 127;   
data.pitch = 127;  
data.throttle = 12;
data.yaw = 127; 
data.aux1 = 0;   
data.aux2 = 127;   
}
void setup()
{
  radio.begin();
  radio.startListening(); 
  radio.openReadingPipe (1,pipeIn);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_8);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(108);
  ResetData();
  Serial.begin(250000);
  
  ch1.attach(9);
  ch2.attach(2);
  ch3.attach(3);
  ch4.attach(4);
  ch5.attach(5);
  ch6.attach(6);

  ibusSerial.begin(115200);
  for (uint8_t i = 0; i < IBUS_CHANNELS; i++) {
    ibusChannels[i] = 1500;     // neutral default; only ch1-6 are real, 7-14 stay filler
  }

  delay(1000);
}
unsigned long lastRecvTime = 0;
void recvData()
{
while ( radio.available() ) {
radio.read(&data, sizeof(Signal));
lastRecvTime = millis();   
}
}
void loop()
{
recvData();
unsigned long now = millis();
if ( now - lastRecvTime > 1000 ) {
ResetData(); 
}

ch_width_1 = map(data.roll,     0, 255, 1000, 2000);     
ch_width_2 = map(data.pitch,    0, 255, 1000, 2000);     
ch_width_3 = map(data.throttle, 0, 255, 1000, 2000);     
ch_width_4 = map(data.yaw,      0, 255, 1000, 2000); 
        
ch_width_5 = map(data.aux1,     0,   1, 1000, 2000);     
ch_width_6 = map(data.aux2,     0, 255, 1000, 2000);     

ch1.writeMicroseconds(ch_width_1);
ch2.writeMicroseconds(ch_width_2);
ch3.writeMicroseconds(ch_width_3);
ch4.writeMicroseconds(ch_width_4);
ch5.writeMicroseconds(ch_width_5);
ch6.writeMicroseconds(ch_width_6);

// iBUS frame, sent on its own millis() timer - PWM above keeps running regardless
if ( now - lastIbusTime >= IBUS_INTERVAL ) {
  lastIbusTime = now;
  ibusChannels[0] = ch_width_1;
  ibusChannels[1] = ch_width_2;
  ibusChannels[2] = ch_width_3;
  ibusChannels[3] = ch_width_4;
  ibusChannels[4] = ch_width_5;
  ibusChannels[5] = ch_width_6;
  sendIBUS();
}

Serial.print(data.pitch);
Serial.print(" ");
Serial.print(data.throttle);
Serial.print(" ");
Serial.print(data.yaw);
Serial.print(" ");
Serial.print(data.roll);
Serial.print(" ");
Serial.print(data.aux1);
Serial.print(" ");
Serial.print(data.aux2);
Serial.print(" ");
Serial.print(ch_width_1);
Serial.print(" ");
Serial.print(ch_width_2);
Serial.print(" ");
Serial.print(ch_width_3);
Serial.print(" ");
Serial.print(ch_width_4);
Serial.print(" ");
Serial.print(ch_width_5);
Serial.print(" ");
Serial.print(ch_width_6);
Serial.println(" ");

}
