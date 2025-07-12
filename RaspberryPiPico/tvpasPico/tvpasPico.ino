
#include <Keyboard.h>
#define START_BUTTON_DURATION 1100          // 0.8秒（ミリ秒）
#define KEY_RELEASE 30                     // 押してから10ms後にキーをリリース
#define BUTTON_DEBOUNCE_TIME 200                  // 500msの間は同一ボタンの入力を受け付けない
#define TVP_DEBOUNCE_TIME 200                     // 500msの間は同一TVPの入力を受け付けない

enum ButtonType { LAP1, LAP2, LAP3, START, CANCEL1, CANCEL2, CANCEL3 };
const uint8_t ButtonPin[] = { 11, 15, 16, 20, 10, 14, 17 };

uint32_t buttonDebounceMillis[sizeof(ButtonPin) / sizeof(ButtonPin[0])] = { 0 };
bool buttonWaitDebounce[sizeof(ButtonPin) / sizeof(ButtonPin[0])] = { false };

#define NUM_TVP_CHANNELS 4
uint32_t tvpDebounceMillis[NUM_TVP_CHANNELS] = { 0 };
bool tvpWaitDebounce[NUM_TVP_CHANNELS] = { false };

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

  // TVPのデバウンス解除処理
  for (uint8_t i = 0; i < NUM_TVP_CHANNELS; i++) {
    if (tvpWaitDebounce[i] && millis() > tvpDebounceMillis[i]) {
      tvpWaitDebounce[i] = false;
    }
  }

  if (Serial.available() > 0) {
    readByte = Serial.read();
    if (readByte == 0xFF) Serial.write(0xFF);
    char keyChar;
    uint8_t tempReadByte = readByte;

    for (uint8_t i = 0; i < NUM_TVP_CHANNELS; i++) {
      keyChar = '0' + i + 1;
      if ((tempReadByte & mask) > 0) {
        if (!tvpWaitDebounce[i]) {
          processTvp(keyChar);
          tvpWaitDebounce[i] = true;
          tvpDebounceMillis[i] = millis() + TVP_DEBOUNCE_TIME;
        }
      }
      tempReadByte = tempReadByte >> 2;
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