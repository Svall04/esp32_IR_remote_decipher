#define IR_CODE_FOR_0 3438
#define IR_CODE_FOR_1 2798
#define IR_CODE_FOR_2 2926
#define IR_CODE_FOR_3 2670
#define IR_CODE_FOR_4 2990
#define IR_CODE_FOR_5 750
#define IR_CODE_FOR_6 878
#define IR_CODE_FOR_7 622
#define IR_CODE_FOR_8 942
#define IR_CODE_FOR_9 3312

#define IR_CODE_FOR_RAISING_BRIGHTNESS 366
#define IR_CODE_FOR_LOWERING_BRIGHTNESS 1390

#define IR_CODE_FOR_SETTING_STUFF 3694

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
  uint16_t number_showing = 0;
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
    brightness %= 8;
    display.setBrightness(brightness);
    display.showNumberDec(number_showing);
    // to be updated, a number must be shown on the dispaly.
    return 0;
  }
  int display_lower_brightness() {
    brightness--;
    brightness %= 8;
    display.showNumberDec(number_showing);
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

void manage_remote_codes(void *parameters) {
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
          Serial.print("hiii\n");
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

void initialize() {
  xTaskCreatePinnedToCore(manage_remote_codes, "Code manager", 1024, NULL, 1,
                          NULL, 1);
}

void setup() {
  initialize();
  display.setBrightness(0);
  display.showNumberDec(8888);
  esp_timer_init();
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT_PULLUP);
  attachInterrupt(IR_PIN, IR_ISR, CHANGE);
  g_t0 = esp_timer_get_time();
}

static bool display_on = true; // just for show
static int i = 10;             // pt2
void loop() {}

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
