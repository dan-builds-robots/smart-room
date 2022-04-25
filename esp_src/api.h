#ifndef API
#define API
#include "Connection.h"

struct RGB { uint8_t r{}, g{}, b{}; };
struct LightInfo{
    RGB leds;
    bool room_lights = false;
};

LightInfo get_light_info(const char* user) {
    StringBuffer<100> uri = "/sandbox/sc/jblt/smart_home.py?";
    uri.append_printf("user=%s&get_state=lights", user);
    auto res = UrlEncodedRequest<>("608dev-2.net").get(uri.c_ptr()).toJsonDoc();

    LightInfo retval;
    retval.room_lights = res["room_lights"];
    retval.leds.r = res["rgb"][0];
    retval.leds.g = res["rgb"][1];
    retval.leds.b = res["rgb"][2];
    return retval;
}

void send_image(const char* user, const char* base64_str) {
    StringBuffer<100> uri = "/sandbox/sc/jblt/smart_home.py?";
    uri.append_printf("user=%s&change_state=camera", user);
    auto res = UrlEncodedRequest<>("608dev-2.net").post(uri.c_ptr(), base64_str);
}

void update_light_switch(const char* user, bool isOn) {
    StringBuffer<100> uri = "/sandbox/sc/jblt/smart_dorm.py?";
    uri.append_printf("user=%s&change_state=lights", user);
    uri.append_printf("&room_lights=%s", isOn ? "True" : "False");
    auto res = UrlEncodedRequest<>("608dev-2.net").post(uri.c_ptr(), "");
}

void notify_door_opening(const char* user){
    StringBuffer<100> uri = "/sandbox/sc/jblt/smart_dorm.py?";
    uri.append_printf("user=%s&change_state=locks", user);
    uri.append_printf("&was_opened=True");
    auto res = UrlEncodedRequest<>("608dev-2.net").post(uri.c_ptr(), "");
}

bool should_unlock_door(const char* user) {
    StringBuffer<100> uri = "/sandbox/sc/jblt/smart_home.py?";
    uri.append_printf("user=%s&get_state=locks", user);
    auto res = UrlEncodedRequest<>("608dev-2.net").get(uri.c_ptr()).toJsonDoc();

    bool should_unlock = res["should_unlock"];
    return should_unlock;
}

#endif