#include <Arduino.h>

const int IR_PIN = 27; // Connect the OUT pin of VS1838B to pin 11 of Arduino

int64_t g_t0;
bool g_flag = 0;

int64_t times[100];
int i = 0;
void IR_ISR(void) {
  Serial.println((int64_t)esp_timer_get_time());
  times[i] = esp_timer_get_time();
  Serial.println(times[i]);
  i++;
  g_t0 = esp_timer_get_time();
}

void setup() {
  esp_timer_init();
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT_PULLUP);
  attachInterrupt(IR_PIN, IR_ISR, RISING);
}

void loop() {
  for (int j = 0; j < i + 5; j++) {
    Serial.println(times[j]);
  }
  Serial.println("==============");
  Serial.print(g_t0);
  sleep(3);
}
