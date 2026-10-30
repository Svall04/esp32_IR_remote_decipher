#define IR_CODE_FOR_1 =
#define IR_CODE_FOR_2 =
#define IR_CODE_FOR_3 =
#define IR_CODE_FOR_4 =
#define IR_CODE_FOR_5 =
#define IR_CODE_FOR_6 =
#define IR_CODE_FOR_7 =
#define IR_CODE_FOR_8 =
#define IR_CODE_FOR_9 =

#define IR_CODE_FOR_RAISING_BRIGHTNESS = 0
#define IR_CODE_FOR_LOWERING_BRIGHTNESS = 1

#include <Arduino.h>
#include <TM1637Display.h>
// stuct che holda il mio messaggio da mandare in queue
static TM1637Display display = TM1637Display(26, 25);

struct IR_code {
  uint8_t size = 0;
  uint16_t message = 0;
};
// struct che contiene lo stato della macchina
struct Sys_state {
  uint8_t state = 0;
  uint8_t brightness = 0;
  Sys_state() {
    state = 0;
    brightness = 0;
  } // costruttore
  int turned_off() {
    state = 0;
    return 0;
  }
  int display_time() {
    state = 100;
    return 0;
  }
  int start_settig_time() {
    state = 110;
    return 0;
  }
  int display_raise_brightness() {
    brightness++;
    brightness %= 7;
    display.setBrightness(brightness);
    return 0;
  }
  int display_lower_brightness() {
    brightness--;
    brightness %= 7;
    display.setBrightness(brightness);
    return 0;
  };
};

static Sys_state sys;

//
// void set_display_brightness(void *parameters){
//  uint16_t
//}

static IR_code IR_message_from_remote;
const int IR_PIN = 27, SIZE_IR_CODE = 11; //"out" pin
int64_t g_t0, g_t1;
QueueHandle_t IR_message_queue = xQueueCreate(16, sizeof(IR_code));
QueueHandle_t Time_settig_queue = xQueueCreate(16, sizeof(uint8_t));
void IR_ISR(void);

void manage_remote_codes() {
  IR_code curr_message;
  // We only act if the button is pressed repetedly, not if it's pressed
  // continuosly.
  bool prev_remote_press;
  bool curr_remote_press;
  int number_to_be_sent;
  for (;;) {
    if (xQueueReceive(IR_message_queue, (void *)&curr_message, 0) == pdTRUE) {
      curr_remote_press = (curr_message.message & 4) == 4;
      if (prev_remote_press ^ curr_remote_press) {
        prev_remote_press = curr_remote_press;
        // This is to account for cont/alterned pressing of button.
        // the value of thi is stored in IR_code.message 3rd bit
        switch (curr_message.message | 4) {
        // to remove the uncdrtanty
        case IR_CODE_FOR_LOWERING_BRIGHTNESS:
          sys.display_lower_brightness();
          break;
        case IR_CODE_FOR_RAISING_BRIGHTNESS:
          sys.display_raise_brightness();
          break;
        case IR_CODE_FOR_SETTING_STUFF:
          if (sys.state / 100 == 1) { // this corresponds to sys in "time" mode
            sys.start_settig_time();
          }
        case IR_CODE_FOR_1:
          number_to_be_sent = 1;
          if (sys.state / 10 == 11)
            xQueueSend(Time_settig_queue, (void *)&number_to_be_sent, 0);
        }
      }
    }
  }
}
}
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

void IR_ISR(void) {
  int64_t g_t1 = esp_timer_get_time();
  if (g_t1 - g_t0 > 30000) {
    // this corresponds to a new message being recived
    IR_message_from_remote.size = 0;
    IR_message_from_remote.message = 0;
    // A good practice can be to reset the message to zero every time the
    // interupt is raised after a long amount of time.
  } else {                    // This corresponds to the content of the message
    if (g_t1 - g_t0 > 6000) { // Let's say this is 1.
      IR_message_from_remote.size += 1;
      IR_message_from_remote.message |= (1 << IR_message_from_remote.size);

    } else if (g_t1 - g_t0 > 4000) {
      IR_message_from_remote.size += 1;
    }
  }
  if (IR_message_from_remote.size == SIZE_IR_CODE) {
    // Send code to the queue
    xQueueSendFromISR(IR_message_queue, (void *)&IR_message_from_remote, 0);
  }
  g_t0 = g_t1;
}
