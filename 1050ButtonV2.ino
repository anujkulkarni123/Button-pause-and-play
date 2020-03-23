//Author: Anuj Kulkarni, Finn hafting
#include <IRLibDecodeBase.h>  //We need both the coding and
#include <IRLibSendBase.h>    // sending base classes
#include <IRLib_P01_NEC.h>    //Lowest numbered protocol 1st
#include <IRLib_P02_Sony.h>   // Include only protocols you want
#include <IRLib_P03_RC5.h>
#include <IRLib_P04_RC6.h>
#include <IRLib_P05_Panasonic_Old.h>
#include <IRLib_P07_NECx.h>
#include <IRLib_HashRaw.h>    //We need this for IRsendRaw
#include <IRLibCombo.h>       // After all protocols, include this
#include <TonePlayer.h>

//int irPin = 3;
int button = 7;
int switch1 = 5;
int switch2 = 6;
int speak = 9;
int reset = 1;
int pressTime, releaseTime, pressDuration;

// Now declare instances of the decoder and the sender.
IRdecode myDecoder;
IRsend mySender;

// Include a receiver either this or IRLibRecvPCI or IRLibRecvLoop
#include <IRLibRecv.h>
IRrecv myReceiver(2); //pin number for the receiver

TonePlayer tone1 (TCCR1A, TCCR1B, OCR1AH, OCR1AL, TCNT1H, TCNT1L);  // pin D9 (Uno), D11 (Mega)

// Storage for the recorded code
uint8_t codeProtocol;  // The type of code
uint32_t codeValue;    // The data bits if type is not raw
uint8_t codeBits;      // The length of the code in bits

//These flags keep track of whether we received the first code 
//and if we have have received a new different code from a previous one.
bool gotOne, gotNew; 

int pins[4] = {8,10,11,4};

void setup() {
  // put your setup code here, to run once:
  
  gotOne=false; gotNew=false;
  codeProtocol=UNKNOWN; 
  codeValue=0; 

  pinMode(button, INPUT);
  pinMode(pins[0], OUTPUT);
  pinMode(pins[1], OUTPUT);
  pinMode(pins[2], OUTPUT);
  pinMode(pins[3], OUTPUT);
  pinMode(switch1, INPUT);
  pinMode(switch2, INPUT);

  Serial.begin(9600);
  while(!Serial);
  Serial.println(F("Send a code from your remote and we will record it."));
  myReceiver.enableIRIn(); // Start the receiver

//show that arduino has started up
  bigTHigh(pins, 4, 200);
  bigDLow(pins, 4);
  delay(1000);
}

//various fuctions to play with the lights
void bigDHigh(int pins[], int pSize){
  for(int i=0;i<pSize;i++){
    digitalWrite(pins[i], HIGH);
    digitalWrite(speak, LOW);
  }
}
void bigDLow(int pins[], int pSize){
  for(int i=0;i<pSize;i++){
    digitalWrite(pins[i], LOW);
  }
}
void bigTHigh(int pins[], int pSize, int wait){
  for(int i=0;i<pSize;i++){
    digitalWrite(pins[i], HIGH);
    delay(wait);
  }
}
void bigTLow(int pins[], int pSize, int wait){
  for(int i=0;i<pSize;i++){
    digitalWrite(pins[i], LOW);
    delay(wait);
  }
}

//____________________________________________________________________________
// Stores the code for later playback
void storeCode(void) {
  gotNew=true;    gotOne=true;
  codeProtocol = myDecoder.protocolNum;
  Serial.print(F("Received "));
  Serial.print(Pnames(codeProtocol));
  if (codeProtocol==UNKNOWN) {
    Serial.println(F(" saving raw data."));
    myDecoder.dumpResults();
    codeValue = myDecoder.value;
  }
  else {
    if (myDecoder.value == REPEAT_CODE) {
      // Don't record a NEC repeat value as that's useless.
      Serial.println(F("repeat; ignoring."));
    } else {
      codeValue = myDecoder.value;
      codeBits = myDecoder.bits;
    }
    Serial.print(F(" Value:0x"));
    Serial.println(codeValue, HEX);
  }
}

//Function to send the recorded code
void sendCode(void) {
  if( !gotNew ) {//We've already sent this so handle toggle bits
    if (codeProtocol == RC5) {
      codeValue ^= 0x0800;
    }
    else if (codeProtocol == RC6) {
      switch(codeBits) {
        case 20: codeValue ^= 0x10000; break;
        case 24: codeValue ^= 0x100000; break;
        case 28: codeValue ^= 0x1000000; break;
        case 32: codeValue ^= 0x8000; break;
      }      
    }
  }
  gotNew=false;
  if(codeProtocol== UNKNOWN) {
    //The raw time values start in decodeBuffer[1] because
    //the [0] entry is the gap between frames. The address
    //is passed to the raw send routine.
    codeValue=(uint32_t)&(recvGlobal.decodeBuffer[1]);
    //This isn't really number of bits. It's the number of entries
    //in the buffer.
    codeBits=recvGlobal.decodeLength-1;
    Serial.println(F("Sent raw"));
  }
  mySender.send(codeProtocol,codeValue,codeBits);
  if(codeProtocol==UNKNOWN) return;
  Serial.print(F("Sent "));
  Serial.print(Pnames(codeProtocol));
  Serial.print(F(" Value:0x"));
  Serial.println(codeValue, HEX);
}
//____________________________________________________________________________

void loop() {
  // put your main code here, to run repeatedly:
//____________________________________________________________________________
if (digitalRead(switch1) == HIGH && digitalRead(button) == LOW) {
    pinMode(speak, OUTPUT);  //speaker
    sendCode();
    bigDHigh(pins, 4);
    delay(200);
    bigDLow(pins, 4);

  tone1.tone (440);
  delay(300);
  tone1.tone (554);
  delay (300);
  tone1.noTone ();
  
  delay(1000);

    pinMode(speak, INPUT);  //speaker
  }

//______________________________________________________________________________  
else if(digitalRead(switch2) == HIGH && digitalRead(button) == LOW){
    sendCode();
    bigDHigh(pins, 4);
    delay(200);
    bigDLow(pins, 4);
    delay(1000);
  }
//____________________________________________________________________________
else{
  bigDLow(pins, 4);
  pinMode(speak, INPUT);
  while(digitalRead(switch1) != HIGH && digitalRead(switch2) != HIGH){
  bigDHigh(pins, 4);
if (Serial.available()) {
    uint8_t C= Serial.read();
    if(C=='r')codeValue=REPEAT_CODE;
    if(gotOne) {
      sendCode();
      myReceiver.enableIRIn(); // Re-enable receiver
    }
  } 
  else if (myReceiver.getResults()) {
    myDecoder.decode();
    storeCode();
    myReceiver.enableIRIn();// Re-enable receiver
    bigDLow(pins, 4);
    delay(100);
  }
  else if (digitalRead(button) == LOW){
    Serial.println("reset");
    bigDLow(pins, 4);
    myReceiver.enableIRIn();
  gotOne=false; gotNew=false;
  codeProtocol=UNKNOWN; 
  codeValue=0; 
  delay(500);
  bigTHigh(pins, 4, 200);
  bigDLow(pins, 4);
    delay(200);
  }
  }
}
}

