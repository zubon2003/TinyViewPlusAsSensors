
#include <Keyboard.h>
#define START_BUTTON_DURATION 1100
#define KEY_RELEASE 30
#define BUTTON_DEBOUNCE_TIME 200

enum ButtonType { LAP1, LAP2, LAP3, START, CANCEL1, CANCEL2, CANCEL3 };
const uint8_t ButtonPin[] = { 11, 15, 16, 20, 10, 14, 17 };

uint32_t buttonDebounceMillis[sizeof(ButtonPin) / sizeof(ButtonPin[0])] = { 0 };
bool buttonWaitDebounce[sizeof(ButtonPin) / sizeof(ButtonPin[0])] = { false };

uint32_t keyReleaseMillis = 0;

uint8_t readByte;
uint8_t mask = 0b00000011;



void setup() {
  for (uint8_t i = 0; i < sizeof(ButtonPin) / sizeof(ButtonPin[0]); i++) pinMode(ButtonPin[i], INPUT_PULLUP);
  Keyboard.begin();
  readByte = 0;
  Serial.ignoreFlowControl();
  Serial.begin(115200);    
}

void loop() {
// put your main code here, to run repeatedly:
  if (isButtonPushed(START)) startSequence();

  processButton(LAP1, KEY_LEFT_ALT, '1');
  processButton(LAP2, KEY_LEFT_ALT, '2');
  processButton(LAP3, KEY_LEFT_ALT, '3');
  processButton(CANCEL1, KEY_LEFT_CTRL, '1');
  processButton(CANCEL2, KEY_LEFT_CTRL, '2');
  processButton(CANCEL3, KEY_LEFT_CTRL, '3');

  if (Serial.available() > 0) {
    readByte = Serial.read();
    if (readByte == 0xFF) Serial.write(0xFF);
    char keyChar;

    for (uint8_t i = 0;i <= 3;i++){
      readByte = readByte >> i*2;
      keyChar = '0' + i + 1;
      if ((readByte && mask) > 0) processTvp(keyChar);
    }  
	}

  if ((millis() > keyReleaseMillis) && keyReleaseMillis > 0) Keyboard.releaseAll();
}
//----------------------------------------------------
// ボタン処理関数
//----------------------------------------------------
void processButton(ButtonType button, uint8_t modifierKey, char key) {
  if (!buttonWaitDebounce[button] && isButtonPushed(button)) {
    Keyboard.press(modifierKey);
    Keyboard.press(key);
    keyReleaseMillis = millis() + KEY_RELEASE;
    buttonWaitDebounce[button] = true;
    buttonDebounceMillis[button] = millis() + BUTTON_DEBOUNCE_TIME;
  }

  // デバウンス解除処理
  if (buttonWaitDebounce[button] && millis() > buttonDebounceMillis[button]) {
    buttonWaitDebounce[button] = false;
  }
}

void processTvp(char key) {
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(key);
  keyReleaseMillis = millis() + KEY_RELEASE;
}

bool isButtonPushed(ButtonType button) {
  return digitalRead(ButtonPin[button]) == LOW;
}
//----------------------------------------------------
// シーケンス処理
//----------------------------------------------------
void startSequence() {
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(KEY_LEFT_CTRL);
  delay(100);
  Keyboard.press('1');
  delay(100);
  Keyboard.release(KEY_LEFT_ALT);
  Keyboard.release(KEY_LEFT_CTRL);
  Keyboard.release('1');

  delay(START_BUTTON_DURATION);
  Keyboard.press(' ');
  delay(100);
  Keyboard.release(' ');
}