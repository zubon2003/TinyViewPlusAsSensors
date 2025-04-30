#define BLUEPILL_RESET_PIN 7
uint8_t readByte;

void setup() {
  // put your setup code here, to run once:


pinMode(BLUEPILL_RESET_PIN,OUTPUT);
digitalWrite(BLUEPILL_RESET_PIN,LOW);

DDRB = 0b11111111;
DDRC = 0b11111111;

//SET ALL(1-4) NODE RSSI=50
PORTB = 0b00001010;
PORTC = 0b00001010;

//RESET BLUEPILL
digitalWrite(BLUEPILL_RESET_PIN,HIGH);
delay(100);
digitalWrite(BLUEPILL_RESET_PIN,LOW);
readByte = 0;
delay(3000);
PORTB = 0b00000000;
PORTC = 0b00000000;
Serial.begin(115200);    
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    readByte = Serial.read();
    PORTB = readByte;
    PORTC = (readByte >> 4);
	}
}
