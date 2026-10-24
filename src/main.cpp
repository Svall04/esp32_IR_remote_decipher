#include <Arduino.h>
#include <TM1637Display.h>
// creo la stuct che holda il mio messaggio da mandare in queue
struct IR_code {
  uint8_t size = 0;
  uint16_t message = 0;
};
static IR_code curr_message;
const int IR_PIN = 27, SIZE_IR_CODE = 11; //"out" pin
int64_t g_t0, g_t1;
QueueHandle_t IR_message_queue = xQueueCreate(100, sizeof(IR_code));
void IR_ISR(void) {
  int64_t g_t1 = esp_timer_get_time();
  if (g_t1 - g_t0 > 30000) {
    // this corresponds to a new message being recived
    curr_message.size = 0;
    curr_message.message = 0;
    // A good practice can be to reset the message to zero every time the
    // interupt is raised after a long amount of time.
  } else {                    // This corresponds to the content of the message
    if (g_t1 - g_t0 > 6000) { // Let's say this is 1.
      curr_message.size += 1;
      curr_message.message |= (1 << curr_message.size);

    } else if (g_t1 - g_t0 > 4000) {
      curr_message.size += 1;
    }
  }
  if (curr_message.size == SIZE_IR_CODE) {
    // Send code to the queue
    xQueueSendFromISR(IR_message_queue, (void *)&curr_message, 0);
  }
  g_t0 = g_t1;
}

static TM1637Display display = TM1637Display(26, 25);
void setup() {
  display.setBrightness(0);
  esp_timer_init();
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT_PULLUP);
  attachInterrupt(IR_PIN, IR_ISR, CHANGE);
  g_t0 = esp_timer_get_time();
}

static bool display_on = true; // just for show
static int i = 10;             // pt2
void loop() {
  IR_code recived_message;
  if (xQueueReceive(IR_message_queue, (void *)&recived_message, 0) == pdPASS) {
    Serial.print("Ho ricevuto un messaggio! eccolo a te:  ");
    Serial.println(recived_message.message);

    if (recived_message.message == 1962 | recived_message.message == 1966) {
      display_on = !display_on;
      display.setBrightness(7 * display_on);
      i += display_on;
      // Serial.print(g_t0);
      // sleep(3);
      display.showNumberDecEx(i);
    }
  }
}
