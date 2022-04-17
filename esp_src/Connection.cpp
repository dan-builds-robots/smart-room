#include "Connection.h"

void connectToWifi(const char* network, const char* password){
  WiFi.begin(network, password);
  for(int attempts = 0; WiFi.status() != WL_CONNECTED && attempts < 10; attempts++){
    delay(500);
    Serial.print('.');
  }
  delay(500);
  if(WiFi.isConnected()){
    Serial.println("\nConnected To WiFi");
  }
  else{
    Serial.println("\nUnable To Connect");
    ESP.restart();
  }
}
