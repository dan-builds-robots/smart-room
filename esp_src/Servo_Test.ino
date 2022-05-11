#include <SPI.h>
#include <WiFi.h> //Connect to WiFi Network
#include "api.h"

        //pin we use for PWM on LCD
const int LOOP_PERIOD = 50;     //speed of main loop
int loop_timer; 
char user[50] = "Ty";
char network[] = "MIT";  //SSID for 6.08 Lab
char password[] = ""; //Password for 6.08 Lab

//Door Handle variable
// int angle = 180;
const int DOOR_HANDLE_PIN = 8;
int last_get = 0;
int get_timer = 1500;
enum handle_states{IDLE, OPENING, HOLDING, CLOSING};
handle_states handle_state;

int open_timer = 0;
int open_duration = 1200;

int hold_timer = 0;
int hold_duration = 5000;

int close_timer = 0;
int close_duration = 700;

void setup() {
  //Connect to WIFI
  Serial.begin(115200); //for debugging if needed.
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }



  // Door handle setup
  ledcSetup(1, 102, 12); 
  ledcAttachPin(DOOR_HANDLE_PIN, 1);
  ledcWrite(1, 0);
  analogReadResolution(12);       // initialize the analog resolution
  Serial.begin(115200);           // Set up serial port
  delay(100); 
  pinMode(DOOR_HANDLE_PIN, OUTPUT); 
  handle_state = IDLE;


  loop_timer = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
  run_door_handle();
}

void run_door_handle(){
  bool unlock = false;
  if(millis()-  last_get >= get_timer){
    unlock = should_unlock_door(user);
    last_get = millis();
  }
  // if(unlock){
  //   Serial.println("unlock is true");
  // }
  // Serial.println(handle_state);
  if(handle_state == IDLE){
    write_to_servo(90);
    Serial.println("IDLE");
    if(unlock){
      handle_state = OPENING;
      open_timer = millis();
    }
  }else if(handle_state == OPENING){
    Serial.println("OPENING");
    if(millis()-open_timer < open_duration){
      write_to_servo(170);
    }
    else{
      handle_state = HOLDING;
      hold_timer = millis();
    }
  }else if(handle_state == HOLDING){
    Serial.println("HOLDING");
    if(millis()-hold_timer < hold_duration){
      write_to_servo(95);
    }
    else{
      handle_state = CLOSING;
      close_timer = millis();
    }
  }else if(handle_state == CLOSING){
    Serial.println("CLOSING");
    if(millis()-close_timer < close_duration){
      write_to_servo(50);
    }
    else{
      handle_state = IDLE;
    }
  }
}

void write_to_servo(int angle){
  if(angle == 90){
    ledcWrite(1, 0);
  }
  else{
  int duty = 0; //have duty express the duty cycle as a 0% to 100% value for display.
  duty = map(angle, 0, 180, 6, 24);
  int val = 0;
  val = map(duty, 0,100, 0,4095);
  ledcWrite(1, val);
}
}
