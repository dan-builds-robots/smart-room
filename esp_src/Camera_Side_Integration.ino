#include <WiFi.h>
#include <Wire.h>
#include <ESP32WebServer.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"



#if !(defined ESP32 )
#error Please select the ArduCAM ESP32 UNO board in the Tools/Board
#endif
//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined (OV2640_MINI_2MP)||defined (OV5640_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP_PLUS) \
    || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) \
    ||(defined (ARDUCAM_SHIELD_V2) && (defined (OV2640_CAM) || defined (OV5640_CAM) || defined (OV5642_CAM))))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

// set GPIO17 as the slave select :
int BUTTON1 = 45;
int LOOP_SPEED = 20000;
unsigned long primary_timer;
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000;
char encoded[7000];
char body[7000]; //for
char network[] = "MIT";
char password[] = "";
const char POST_URL[] = "POST http://608dev-2.net/sandbox/sc/aoejile/final_project/image_server.py HTTP/1.1\r\n"; 
const uint16_t IN_BUFFER_SIZE = 7000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 6000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE];
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request



const int signal_in = 14;

const int CS = 34;
int offset = 0;
const int CAM_POWER_ON = 10;
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
  ArduCAM myCAM(OV2640, CS);
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
  ArduCAM myCAM(OV5640, CS);
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
  ArduCAM myCAM(OV5642, CS);
#endif

//you can change the value of wifiType to select Station or AP mode.
//Default is AP mode.
// int wifiType = 0; // 0:Station  1:AP



static const size_t bufferSize = 2048;
static uint8_t buffer[bufferSize] = {0xFF};
uint8_t temp = 0, temp_last = 0;
int i = 0;
bool is_header = false;

// ESP32WebServer server(80);

void start_capture(){
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}


const int img_buffer_size = 60000;
static uint8_t fullimg[img_buffer_size];
void camCapture(ArduCAM myCAM){
 memset(fullimg, 0, img_buffer_size);
  // WiFiClient client = server.client();
  uint32_t fifoLen = myCAM.read_fifo_length();
  if (fifoLen >= MAX_FIFO_SIZE)  //8M
    Serial.println(F("Over size."));
  if (fifoLen == 0)  //0 kb
    Serial.println(F("Size is 0."));
  
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  if (!WiFi.isConnected()) return;

  int realLen = 0; // FFD8 for start and FFD9 for end
  uint8_t prev = SPI.transfer(0x00);
  bool foundStart = false;
  bool complete = false;
  int i = 0;
  while (true) {
    uint8_t current = SPI.transfer(0x00);
    if (!foundStart) {
      if (prev == 0xFF && current == 0xD8) { // found the start of the image!
        //Serial.println("found start of image!");
        foundStart = true;
        fullimg[0] = 0xFF;
        fullimg[1] = 0xD8;
        realLen = 2;
      }
    } else {
      fullimg[realLen++] = current;
      if (realLen >= img_buffer_size) {
        //Serial.println("went too far... quitting");
        break;
      }
      if (prev == 0xFF && current == 0xD9) { // found the end of the image!
        //Serial.println("found end of image!");
        complete = true;
        break;
      }
    }
    prev = current;
    if (i >= fifoLen) {
      //Serial.println("breaking because reached end of fifo");
      break;
    }
    i++;
  }
  myCAM.CS_HIGH();
  if (!complete) {
    //Serial.println("incomplete image, returning");
    return;
  }
  Serial.printf("Got full image! len = %d\n", realLen);

int inputLen = realLen;
int encodedLen = base64_enc_len(inputLen);

  
//   note input is consumed in this step: it will be empty afterwards
base64_encode(encoded, (char*)fullimg, realLen); 
// Serial.printf("hahahaha");
Serial.println("encoded"); 
// body should be json 
Serial.println(encoded);
// char body[100]; //for body
    // test_room_temp = temperature;
    sprintf(body, "{\"image\":\"%s\"}", encoded); //generate body, posting temp, humidity to server
    int body_len = strlen(body); //calculate body length (for header reporting)
    sprintf(request_buffer, POST_URL);
    strcat(request_buffer, "Host: 608dev-2.net\r\n");
    strcat(request_buffer, "Content-Type: application/json\r\n");
    sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
    strcat(request_buffer, "\r\n"); //new line from header to body
    strcat(request_buffer, body); //body
    strcat(request_buffer, "\r\n"); //new line
    Serial.println(request_buffer);
    do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    Serial.println(response_buffer);

  // String response = "HTTP/1.1 200 OK\r\n";
  // response += "Content-Type: image/jpeg\r\n";
  // response += "Content-len: " + String(realLen) + "\r\n\r\n";
  // client.write(&fullimg[0], realLen);
  // server.sendContent(response);
}

void serverCapture(){
  delay(1000);
start_capture();
Serial.println(F("CAM Capturing"));

int total_time = 0;

total_time = millis();
while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
total_time = millis() - total_time;
Serial.print(F("capture total_time used (in miliseconds):"));
Serial.println(total_time, DEC);

total_time = 0;

Serial.println(F("CAM Capture Done."));
total_time = millis();
camCapture(myCAM);
total_time = millis() - total_time;
Serial.print(F("send total_time used (in miliseconds):"));
Serial.println(total_time, DEC);
Serial.println(F("CAM send Done."));
}


void setup() {
pinMode(signal_in,INPUT_PULLUP);
uint8_t vid, pid;
uint8_t temp;
  //set the CS as an output:
  pinMode(CS,OUTPUT);
  pinMode(CAM_POWER_ON , OUTPUT);
  digitalWrite(CAM_POWER_ON, HIGH);
#if defined(__SAM3X8E__)
Wire1.begin();
#else
Wire.begin();
#endif
Serial.begin(115200);
Serial.println(F("ArduCAM Start!"));



// initialize SPI:
SPI.begin();
SPI.setFrequency(4000000); //4MHz

//Check if the ArduCAM SPI bus is OK
myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
temp = myCAM.read_reg(ARDUCHIP_TEST1);
if (temp != 0x55){
Serial.println(F("SPI1 interface Error!"));
while(1);
}

//Check if the ArduCAM SPI bus is OK
myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
temp = myCAM.read_reg(ARDUCHIP_TEST1);
if (temp != 0x55){
Serial.println(F("SPI1 interface Error!"));
while(1);
}
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
//Check if the camera module type is OV2640
myCAM.wrSensorReg8_8(0xff, 0x01);
myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
Serial.println(F("Can't find OV2640 module!"));
else
Serial.println(F("OV2640 detected."));
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
//Check if the camera module type is OV5640
myCAM.wrSensorReg16_8(0xff, 0x01);
myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
if((vid != 0x56) || (pid != 0x40))
Serial.println(F("Can't find OV5640 module!"));
else
Serial.println(F("OV5640 detected."));
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
//Check if the camera module type is OV5642
myCAM.wrSensorReg16_8(0xff, 0x01);
myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
if((vid != 0x56) || (pid != 0x42)){
Serial.println(F("Can't find OV5642 module!"));
}
else
Serial.println(F("OV5642 detected."));
#endif


//Change to JPEG capture mode and initialize the OV2640 module
myCAM.set_format(JPEG);
myCAM.InitCAM();
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
myCAM.OV2640_set_JPEG_size(OV2640_160x120);
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
myCAM.OV5640_set_JPEG_size(OV5640_320x240);
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
myCAM.OV5640_set_JPEG_size(OV5642_320x240);  
#endif

myCAM.clear_fifo_flag();
WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);


  uint8_t count1 = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count1 < 12) {
    delay(500);
    Serial.print(".");
    count1++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
}
primary_timer = millis();
}
void loop() {
   if (digitalRead(signal_in) == 0){
    Serial.println("INCOMING KNOCK SIGNAL DETECTED");
    serverCapture();
    Serial.println("success");
    delay(1500);
    }

}
  
