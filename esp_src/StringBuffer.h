#ifndef STRING_BUFFER
#define STRING_BUFFER
#include "ArduinoJson.h"

template<size_t size_>
struct StringBuffer{
    StringBuffer() = default;
    StringBuffer(const char* str){ 
        strncpy(data, str, size_);
        current = strlen(str);
        if (current > size_) current = size_;
    }
    const char* c_ptr() const { return data; }
    void clear(){ current = 0; data[0] = '\0'; }
    bool push(char c) {
        if (current + 1 >= size) {
            Serial.println("Could not push character to buffer.");
            return false;
        }
        data[current] = c;
        data[current+1] = '\0';
        return true;
    }

    auto toJsonDoc() -> StaticJsonDocument<size_> {
        StaticJsonDocument<size_> doc;
        deserializeJson(doc, data);
        return doc;
    }

    template<typename...Args>
    bool append_printf(const char* format, Args...args){
        if(format[0] == '\0') return true;
        auto err = snprintf(data + current, size_ - current, format, args...);
        if(err > 0 && current + err <= size_) {
            current += err;
            return true; //returns true if wasnt encoding error
        }
        current = size_;
        Serial.println("String Buffer was not large enough. Trying to add:");
        Serial.println(format);
        return false; 
    }
private:
  char data[size_] = "";
  size_t size = size_;
  size_t current = 0;
};


#endif