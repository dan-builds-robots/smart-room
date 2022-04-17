#ifndef CONNECTION
#define CONNECTION
#include "WiFi.h"
#include "StringBuffer.h"

void connectToWifi(const char* network, const char* password);

/*
Construts with the host name of ur site i.e: google.com. 
Then you can call get request with url encoded arguments in the uri.
Or call a post request and put the url encoded arguments in the body.
*/

template<size_t MAX_HEADER_SIZE = 490, size_t MAX_BODY_SIZE = 250, size_t MAX_RESPONSE_SIZE = 500>
class UrlEncodedRequest {
public:
  class Header{
  public:
    enum class Method {POST, GET};

    Header(Method method, const char* host, const char* uri){
      static const char* format = 
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n";
      buffer.append_printf(format, methodToStr(method), uri, host );
    }
    Header& contentLength(size_t len){ 
      buffer.append_printf("Content-Length:%d\r\n", len);
      return *this;
    }
    Header& contentType(const char* type){
      buffer.append_printf("Content-Type:%s\r\n", type);
      return *this;
    }
    StringBuffer<MAX_HEADER_SIZE> build(){
      buffer.append_printf("\r\n");
      return buffer;
    }
  private:
    static const char* methodToStr(Method method){
      switch(method){
        case Method::POST: return "POST";
        case Method::GET: return "GET";
      }
    }
    StringBuffer<MAX_HEADER_SIZE> buffer;
  };

  UrlEncodedRequest(const char* host) : host(host) {}
  using Response = StringBuffer<MAX_RESPONSE_SIZE>;

  Response post(const char* uri, const char* body){ 
    return sendRequest(Header::Method::POST, uri, body);
  }
  Response get(const char* uri){ 
    return sendRequest(Header::Method::GET, uri, "");
  }

private:

  Response sendRequest(enum Header::Method method, const char* uri, const char* body){
    auto header = Header(method, host, uri)
      .contentType("application/x-www-form-urlencoded")
      .contentLength(strlen(body))
      .build();
    StringBuffer<MAX_HEADER_SIZE + MAX_BODY_SIZE> request;
    request.append_printf(header.c_ptr());
    request.append_printf(body);
    return sendText(request.c_ptr());
  }

  //class code
  static uint8_t char_append(char* buff, char c, uint16_t buff_size) {
    int len = strlen(buff);
    if (len>buff_size) return false;
    buff[len] = c;
    buff[len+1] = '\0';
    return true;
  }
  Response sendText(const char* msg, int timeout = 5000){
    WiFiClient client;
    char response[MAX_RESPONSE_SIZE] = "";
    if(client.connect(host, 80)){
      client.print(msg);

      memset(response, 0, MAX_RESPONSE_SIZE); //Null out (0 is the value of the null terminator '\0') entire buffer
      uint32_t count = millis();
      while (client.connected()) { //while we remain connected read out data coming back
        client.readBytesUntil('\n',response, MAX_RESPONSE_SIZE);
        if (strcmp(response,"\r")==0) { //found a blank line!
          break;
        }
        memset(response, 0, MAX_RESPONSE_SIZE);
        if (millis()-count>timeout) break;
      }
      memset(response, 0, MAX_RESPONSE_SIZE);  
      count = millis();
      while (client.available()) { //read out remaining text (body of response)
        char_append(response, client.read(), MAX_RESPONSE_SIZE);
      }
    }
    else{
      Serial.println("Get Request Failed. Could not connect to host.");
    }
    client.stop();
    return Response(response);
  }

  const char* host;
};

#endif