#include <mpu6050_esp32.h>
#include <math.h>
#include <WiFi.h> //Connect to WiFi Network


#include <Wire.h>
#include <SPI.h>



char network[] = "MIT";
char password[] = "";

//Adding server interfacing

//const char POST_URL[] = "POST https://608dev-2.net/sandbox/sc/jblt/smart_home.py?user=Agustin&change_state=locks&should_unlock=True&was_opened=False HTTP/1.1\r\n";

const char POST_URL[] = "POST https://608dev-2.net/sandbox/sc/jblt/smart_home.py?user=Agustin&knock_state=True HTTP/1.1\r\n";

const char USER[] = "agustin";

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t IN_BUFFER_SIZE = 7000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 6000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response


//Pin for communicating with other ESP
const int signal_out = 33;



const int door_opening_threshold = 3000000; //3 seconds in microseconds 


float x, y, z;
MPU6050 imu;
const float ZOOM = 9.81;
float old_acc_mag, older_acc_mag; 
float acc_mag = 0;
float avg_acc_mag = 0; 

const char TRUE[] = "True";
const char FALSE[] = "False";


//Are we accepting input from the stream or recording?

int record_bool;
int stream_bool;

const int AUDIO_IN = A3;
int indx = 0;
float avg_audio = 0;

const uint32_t FILTER_ORDER = 15; //size of filter

uint32_t primary_timer;
uint32_t door_opening_timer;
uint32_t recording_timer;
uint32_t streaming_timer;

float sample_rate = 40; //Hz
float sample_period = (int)(1e6 / sample_rate);
float values[20]; //used for remembering an arbitrary number of previous values for averaging filter

//Seems that the samples are all 41 floats long; we'll make our stream container that size as well
float audio_stream[42]; //container for values coming in from microphone data that will be run through correlate functions
float imu_stream[42]; //new stream to run correlation on, this time checking the behavior of the imu when knocking

float ref_knock1[] = {0.35,0.35,0.40,0.35,0.36,0.38,0.37,0.35,0.36,0.36,0.36,0.37,0.37,0.36,0.37,0.37,0.38,
0.39,0.36,0.38,0.39,0.40,0.39,0.41,0.40,0.41,0.40,0.39,0.40,0.41,0.40,0.40,0.41,0.39,0.39,0.40,0.38,0.36,0.38,0.38,0.37};

int ref_knock1_len = sizeof(ref_knock1)/4;

float ref_knock2[] = {0.37,
0.37,
0.35,
0.34,
0.35,
0.39,
0.36,
0.36,
0.39,
0.38,
0.38,
0.38,
0.40,
0.38,
0.38,
0.38,
0.38,
0.38,
0.49,
0.46,
0.45,
0.42,
0.46,
0.48,
0.46,
0.45,
0.46,
0.46,
0.44,
0.46,
0.47,
0.46,
0.45,
0.46,
0.37,
0.41,
0.41,
0.39,
0.38,
0.35,
0.35};
int ref_knock2_len = sizeof(ref_knock2)/4;


float ref_knock3[] = {0.39,
0.39,
0.39,
0.38,
0.39,
0.38,
0.40,
0.39,
0.40,
0.41,
0.40,
0.40,
0.41,
0.41,
0.40,
0.42,
0.39,
0.38,
0.38,
0.31,
0.30,
0.30,
0.27,
0.30,
0.33,
0.27,
0.30,
0.30,
0.30,
0.29,
0.31,
0.30,
0.30,
0.30,
0.29,
0.37,
0.38,
0.38,
0.40,
0.39,
0.34};
int ref_knock3_len = sizeof(ref_knock3)/4;

float ref_knock4[] = {0.37,
0.39,
0.35,
0.36,
0.36,
0.38,
0.36,
0.33,
0.32,
0.34,
0.34,
0.34,
0.33,
0.34,
0.34,
0.33,
0.32,
0.29,
0.34,
0.32,
0.28,
0.27,
0.32,
0.30,
0.33,
0.31,
0.30,
0.29,
0.31,
0.29,
0.30,
0.30,
0.30,
0.30,
0.31,
0.32,
0.35,
0.34,
0.31,
0.36,
0.34};

int ref_knock4_len = sizeof(ref_knock4)/4;

float ref_knock5[] = {0.34,0.32,0.35,0.33,0.36,0.36,0.36,
0.37,
0.41,
0.38,
0.39,
0.40,
0.40,
0.38,
0.39,
0.40,
0.39,
0.42,
0.40,
0.41,
0.38,
0.38,
0.41,
0.39,
0.38,
0.43,
0.38,
0.38,
0.39,
0.41,
0.40,
0.39,
0.40,
0.40,
0.40,
0.40,
0.41,
0.41,
0.38,
0.38,
0.37};

int ref_knock5_len = sizeof(ref_knock5)/4;

float ref_knock6[] = {0.34,
0.32,
0.35,
0.33,
0.36,
0.36,
0.36,
0.37,
0.41,
0.38,
0.39,
0.40,
0.40,
0.38,
0.39,
0.40,
0.39,
0.42,
0.40,
0.41,
0.38,
0.38,
0.41,
0.39,
0.38,
0.43,
0.38,
0.38,
0.39,
0.41,
0.40,
0.39,
0.40,
0.40,
0.40,
0.40,
0.41,
0.41,
0.38,
0.38,
0.37};

int ref_knock6_len = sizeof(ref_knock6)/4;


float ref_knock7[] = {0.07,
0.11,
0.08,
0.12,
0.16,
0.23,
0.22,
0.25,
0.28,
0.31,
0.31,
0.34,
0.38,
0.39,
0.42,
0.44,
0.40,
0.38,
0.43,
0.31,
0.26,
0.21,
0.25,
0.18,
0.15,
0.14,
0.16,
0.13,
0.12,
0.14,
0.13,
0.12,
0.13,
0.13,
0.13,
0.22,
0.27,
0.26,
0.26,
0.32,
0.35};
int ref_knock7_len = sizeof(ref_knock7)/4;

float ref_knock8[] = {0.36,
0.32,
0.36,
0.35,
0.33,
0.36,
0.34,
0.34,
0.37,
0.37,
0.37,
0.36,
0.37,
0.37,
0.37,
0.37,
0.36,
0.40,
0.38,
0.39,
0.39,
0.36,
0.41,
0.41,
0.40,
0.41,
0.41,
0.40,
0.41,
0.41,
0.41,
0.40,
0.41,
0.40,
0.40,
0.40,
0.40,
0.41,
0.38,
0.38,
0.36};
int ref_knock8_len = sizeof(ref_knock8)/4;

float ref_knock9[] = {0.33,
0.34,
0.36,
0.33,
0.32,
0.34,
0.34,
0.35,
0.34,
0.35,
0.34,
0.32,
0.33,
0.33,
0.32,
0.32,
0.35,
0.33,
0.32,
0.34,
0.35,
0.36,
0.34,
0.34,
0.35,
0.33,
0.37,
0.38,
0.36,
0.37,
0.38,
0.38,
0.37,
0.38,
0.38,
0.37,
0.37,
0.35,
0.36,
0.35,
0.35};
int ref_knock9_len = sizeof(ref_knock9)/4;

float ref_knock10[] = {0.39,
0.39,
0.36,
0.38,
0.37,
0.40,
0.39,
0.39,
0.40,
0.41,
0.42,
0.42,
0.37,
0.39,
0.39,
0.37,
0.36,
0.33,
0.35,
0.35,
0.35,
0.31,
0.33,
0.33,
0.32,
0.28,
0.28,
0.26,
0.32,
0.31,
0.30,
0.31,
0.32,
0.31,
0.32,
0.31,
0.31,
0.32,
0.31,
0.32,
0.31};
int ref_knock10_len = sizeof(ref_knock10)/4;



float ref_rattle1[] = {10.60,
10.50,
10.26,
10.23,
10.27,
10.27,
10.31,
10.38,
10.57,
10.27,
10.21,
10.25,
10.30,
10.34,
10.25,
10.31,
10.26,
10.18,
10.32,
10.42,
10.44,
10.37,
10.33,
10.26,
10.22,
10.26,
10.22,
10.23,
10.26,
10.24,
10.27,
10.26,
10.25,
10.25,
10.27,
10.26,
10.25,
10.25,
10.26,
10.25,
10.26};

int ref_rattle1_len = sizeof(ref_rattle1)/4;



float ref_rattle2[] = {10.49,
10.39,
10.37,
10.54,
10.45,
10.32,
10.27,
10.29,
10.26,
10.36,
10.43,
10.36,
10.31,
10.09,
10.19,
10.32,
10.37,
10.26,
10.32,
10.27,
10.24,
10.21,
10.26,
10.27,
10.15,
10.38,
10.24,
10.24,
10.29,
10.26,
10.25,
10.28,
10.27,
10.26,
10.24,
10.24,
10.25,
10.26,
10.28,
10.28,
10.25};
int ref_rattle2_len = sizeof(ref_rattle2)/4;


float ref_rattle3[] = {10.56,
10.44,
10.29,
10.42,
10.32,
10.29,
10.25,
10.27,
10.32,
10.47,
10.28,
10.16,
10.22,
10.35,
10.34,
10.27,
10.27,
10.25,
10.26,
10.23,
10.29,
10.38,
10.13,
10.29,
10.27,
10.24,
10.25,
10.28,
10.26,
10.27,
10.24,
10.25,
10.26,
10.25,
10.24,
10.24,
10.26,
10.26,
10.26,
10.27,
10.27};
int ref_rattle3_len = sizeof(ref_rattle3)/4;

void setup() {
  Serial.begin(115200);

  delay(100); //wait a bit (100 ms)
  Wire.begin();



  SPI.begin();



  

  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  
  
  delay(50); //pause to make sure comms get set up
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }


  pinMode(signal_out, OUTPUT);
  
  pinMode(AUDIO_IN, INPUT);
  analogReadResolution(12); 
  
  for (int i=0; i<10; i++){ 
    values[i] = 0.0;
  }
  for (int i=0; i<41; i++){ 
    audio_stream[i] = 0.0;
    imu_stream[i] = 0.0;
  }
  record_bool = 0;
  stream_bool = 0;
  Serial.println(ref_rattle1_len);
  delay(50);

  primary_timer = micros();

}

void loop() {
  // put your main code here, to run repeatedly:
  //Shift list of stored running values over, appending the newest one at the end
  digitalWrite(signal_out, HIGH);
    
  //We only want to permit data to go into our stream if the IMU detects a knock
  //This should help with the time and memory complexity
  
  if (getIMUdata() > 10.4 && stream_bool == 0){
    stream_bool = 1;
    Serial.println("Collecting from stream");
    streaming_timer = micros();
  }
  if (stream_bool == 1){
    //pull from the two streams, shifting floats over in the respective arrays
    for(int i = 1; i<41; i++){
      audio_stream[i-1] = audio_stream[i];
      imu_stream[i-1] = imu_stream[i];
      
    }
    audio_stream[40] = getfilteredAudio();
    imu_stream[40] = getIMUdata();

  }
  
  if (micros() - streaming_timer > 1000000 && stream_bool == 1){
    //Stop recording once a second has passed, run the correlation on stream array now
    stream_bool = 0;
    if (patternDetection() == 1){
      Serial.println("PATTERN DETECTED, SENDING TO SERVER"); //duh duh -- duh
      //Create a signal for interpretation by the other ESP
      digitalWrite(signal_out, LOW);
      //post_to_server();
      delay(1500);      
    }
    Serial.println("Leaving stream");
    delay(1500);
  }
  
  /////////////////////////////
  //Sample recording set up
  ////////////////////////////
//  
//  if (getIMUdata() > 10.4 && record_bool == 0){
//    record_bool = 1;
//    Serial.println("Starting recording");
//    recording_timer = micros();
//  }
//  if (record_bool == 1){
//    Serial.println(",");
//    for(int i = 1; i<41; i++){
//      audio_stream[i-1] = audio_stream[i];
//      imu_stream[i-1] = imu_stream[i];
//    }
//    audio_stream[40] = getfilteredAudio();
//    imu_stream[40] = getIMUdata();
//    
//    Serial.print(audio_stream[40]);
//    
//  }
//  if (micros() - recording_timer > 1000000 && record_bool == 1){
//    record_bool = 0;
//  }

  ////////////////////////////
  
}

int patternDetection(){
  //Correlation-based clap detection
  //Take correlation between the samples and the running average values

  //Audio buffers
  
  float on_buffer1A[ref_knock1_len]; float on_buffer1B[ref_knock1_len];
  float on_buffer2A[ref_knock2_len]; float on_buffer2B[ref_knock2_len];
  float on_buffer3A[ref_knock3_len]; float on_buffer3B[ref_knock3_len];
  float on_buffer4A[ref_knock4_len]; float on_buffer4B[ref_knock4_len];
  float on_buffer5A[ref_knock5_len]; float on_buffer5B[ref_knock5_len];
  float on_buffer6A[ref_knock6_len]; float on_buffer6B[ref_knock6_len];
  float on_buffer7A[ref_knock7_len]; float on_buffer7B[ref_knock7_len];
  float on_buffer8A[ref_knock8_len]; float on_buffer8B[ref_knock8_len];
  float on_buffer9A[ref_knock9_len]; float on_buffer9B[ref_knock9_len];
  float on_buffer10A[ref_knock10_len]; float on_buffer10B[ref_knock10_len];

  
  //IMU buffers
  float on_rbuffer1A[ref_rattle1_len]; float on_rbuffer1B[ref_rattle1_len];
  float on_rbuffer2A[ref_rattle2_len]; float on_rbuffer2B[ref_rattle2_len];
  float on_rbuffer3A[ref_rattle3_len]; float on_rbuffer3B[ref_rattle3_len];

  
  //Audio correlation values
  float corr1 = correlation(ref_knock1, audio_stream, on_buffer1A, on_buffer1B, ref_knock1_len);
  float corr2 = correlation(ref_knock2, audio_stream, on_buffer2A, on_buffer2B, ref_knock2_len);
  float corr3 = correlation(ref_knock3, audio_stream, on_buffer3A, on_buffer3B, ref_knock3_len);
  float corr4 = correlation(ref_knock4, audio_stream, on_buffer4A, on_buffer4B, ref_knock4_len);
  float corr5 = correlation(ref_knock5, audio_stream, on_buffer5A, on_buffer5B, ref_knock5_len);
  float corr6 = correlation(ref_knock6, audio_stream, on_buffer6A, on_buffer6B, ref_knock6_len);
  float corr7 = correlation(ref_knock7, audio_stream, on_buffer7A, on_buffer7B, ref_knock7_len);
  float corr8 = correlation(ref_knock8, audio_stream, on_buffer8A, on_buffer8B, ref_knock8_len);
  float corr9 = correlation(ref_knock9, audio_stream, on_buffer9A, on_buffer9B, ref_knock9_len);
  float corr10 = correlation(ref_knock10, audio_stream, on_buffer10A, on_buffer10B, ref_knock10_len);


  //IMU correlation values
  float rcorr1 = correlation(ref_rattle1, imu_stream, on_rbuffer1A, on_rbuffer1B, ref_rattle1_len);
  float rcorr2 = correlation(ref_rattle2, imu_stream, on_rbuffer2A, on_rbuffer2B, ref_rattle2_len);
  float rcorr3 = correlation(ref_rattle3, imu_stream, on_rbuffer3A, on_rbuffer3B, ref_rattle3_len);


  memset(on_rbuffer1A, 0, sizeof(on_rbuffer1A)); memset(on_rbuffer1B, 0, sizeof(on_rbuffer1B));
  memset(on_rbuffer2A, 0, sizeof(on_rbuffer2A)); memset(on_rbuffer2B, 0, sizeof(on_rbuffer2B));
  memset(on_rbuffer3A, 0, sizeof(on_rbuffer3A)); memset(on_rbuffer3B, 0, sizeof(on_rbuffer3B));
  

  
  memset(on_buffer1A, 0, sizeof(on_buffer1A)); memset(on_buffer1B, 0, sizeof(on_buffer1B));
  memset(on_buffer2A, 0, sizeof(on_buffer2A)); memset(on_buffer2B, 0, sizeof(on_buffer2B));
  memset(on_buffer3A, 0, sizeof(on_buffer3A)); memset(on_buffer3B, 0, sizeof(on_buffer3B));
  
  memset(on_buffer4A, 0, sizeof(on_buffer4A)); memset(on_buffer4B, 0, sizeof(on_buffer4B));
  memset(on_buffer5A, 0, sizeof(on_buffer5A)); memset(on_buffer5B, 0, sizeof(on_buffer5B));
  memset(on_buffer6A, 0, sizeof(on_buffer6A)); memset(on_buffer6B, 0, sizeof(on_buffer6B));
  memset(on_buffer7A, 0, sizeof(on_buffer7A)); memset(on_buffer7B, 0, sizeof(on_buffer7B));
  memset(on_buffer8A, 0, sizeof(on_buffer8A)); memset(on_buffer8B, 0, sizeof(on_buffer8B));
  memset(on_buffer9A, 0, sizeof(on_buffer9A)); memset(on_buffer9B, 0, sizeof(on_buffer9B));
  memset(on_buffer10A, 0, sizeof(on_buffer10A)); memset(on_buffer10B, 0, sizeof(on_buffer10B));
  Serial.println(abs(corr1));Serial.println(abs(corr2));Serial.println(abs(corr3));
  Serial.println(abs(corr4));Serial.println(abs(corr5));Serial.println(abs(corr6));
    Serial.println(abs(corr7));Serial.println(abs(corr7));Serial.println(abs(corr9));
  Serial.println(abs(corr10));
  Serial.println("________________________"); Serial.println(abs(rcorr1));Serial.println(abs(rcorr2));Serial.println(abs(rcorr3));
  if (abs(corr1) >= 0.73 || abs(corr2) >= 0.73 || abs(corr3) >= 0.73 || abs(corr4) >= 0.73 || abs(corr5) >= 0.73
  || abs(corr6) >= 0.73 || abs(corr7) >= 0.73 || abs(corr8) >= 0.73 || abs(corr9) >= 0.73 || abs(corr10) >= 0.73 
  || abs(rcorr1) >= 0.73 || abs(rcorr2) >= 0.73 || abs(rcorr3) >= 0.73){
    return 1;
  }
  else{
    return 0;
  }
}


void offset_and_normalize(float* inp, float* out, int inp_size){
  float total = 0.0;
  for(int i= 0; i<inp_size; i++){
    total += inp[i];
  }

  float mean = total/inp_size;
  float offset_data[inp_size];

  for(int i= 0; i<inp_size; i++){
    offset_data[i] = inp[i] - mean;
  }
  double denom = 0.0;
  
  for(int i= 0; i<inp_size; i++){
    denom += pow((inp[i]-mean),2);
  }
  double sqrt_denom = sqrt(denom);
  
 for(int i= 0; i<inp_size; i++){
    out[i] = offset_data[i]/sqrt_denom;
  }   
}

float correlation(float* in1, float* in2, float* on_in1, float* on_in2, int len){
  float corr = 0.0;
  offset_and_normalize(in1, on_in1, len);
  offset_and_normalize(in2, on_in2, len);
  
  for(int i = 0; i< len; i++){
    corr += on_in1[i] * on_in2[i];
  }
  return corr;
}


float averaging_filter(float input, float* stored_values, int order, int* index) {
  stored_values[*index] = input;
  float y = 0;
  for (int i=0; i<order+1; i++){
    y += 1.0/(1.0+order)*stored_values[i];
  }
  if (*index == order){
    *index = 0;
  }
  else{
    (*index)++;
  }
  return y;
}
float getfilteredAudio(){
  float input = (analogRead(AUDIO_IN) *3.3 / 4096) - 1.25;
  while (micros() > primary_timer && micros() - primary_timer < sample_period); //prevents rollover glitch every 71 minutes...not super needed
  primary_timer = micros();
  //return input;
  return averaging_filter(abs(input), values, FILTER_ORDER, &indx); //Compensate for the offset
}


float getIMUdata(){
  imu.readAccelData(imu.accelCount);
  x = ZOOM *imu.accelCount[0] * imu.aRes;
  y = ZOOM *imu.accelCount[1] * imu.aRes;
  z = ZOOM *imu.accelCount[2] * imu.aRes;
  acc_mag = sqrt(x * x + y * y + z * z);
  avg_acc_mag = 1.0 / 3.0 * (acc_mag + old_acc_mag + older_acc_mag);
  older_acc_mag = old_acc_mag;
  old_acc_mag = acc_mag;
  return abs(avg_acc_mag);
}


//void post_to_server(){
//    char body[100]; //for body
//
//    //Content of the body must follow the form: user=agustin&change_state=locks&should_unlock=True&was_opened=True
//
//    //We need to wait some time to see if the door is gonna be opening or not
//    //Start timer, if the IMU detects enough acceleration to consider the door to be opened within that time frame, then send it being true, else send it being false
//    Serial.println("Starting timer");
//    door_opening_timer = micros(); 
//        
////    if (micros() - door_opening_timer < door_opening_threshold){
////   
////      if (getIMUdata() > 11.0){
////        sprintf(body, "user=%s&change_state=locks&should_unlock=%s&was_opened=%s", USER, TRUE, TRUE);
////        Serial.println("Door considered open");
////      }
////       
////    }
////    else{
////      sprintf(body, "user=%s&change_state=locks&should_unlock=%s&was_opened=%s", USER, TRUE, FALSE);
////      Serial.println("Door considered closed");
////      
////    }
//    
//    
//    sprintf(body, "user=%s&change_state=locks&should_unlock=%s&was_opened=%s", USER, TRUE, FALSE);
//    
//    int body_len = strlen(body); //calculate body length (for header reporting)
//    sprintf(request_buffer, POST_URL);
//    strcat(request_buffer, "Host: 608dev-2.net\r\n");
//    strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
//    sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
//    strcat(request_buffer, "\r\n"); //new line from header to body
//    strcat(request_buffer, body); //body
//    strcat(request_buffer, "\r\n"); //new line
//    Serial.println(request_buffer);
//    Serial.println("_______________________________________________");
//    
//    do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//    
//    Serial.println(response_buffer);  
//}
