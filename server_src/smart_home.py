""" Request Summary
Get Requests
https://608dev-2.net/sandbox/sc/jblt/smart_home.py?get_state=lights //gets current rgb, and light value in room
https://608dev-2.net/sandbox/sc/jblt/smart_home.py?get_state=locks //returns last time it was unlocked with esp, and if it should unlock
https://608dev-2.net/sandbox/sc/jblt/smart_home.py?get_state=camera //sends picture of what last set off the door

Post Requests
https://608dev-2.net/sandbox/sc/jblt/smart_home.py?change_state=lights&rgb=125,0,255&room_lights=True
https://608dev-2.net/sandbox/sc/jblt/smart_home.py?change_state=locks&should_unlock=True&was_opened=True
https://608dev-2.net/sandbox/sc/jblt/smart_home.py?change_state=camera //when ultrasonic-detector needs to send a new photo

Notes:
Need to figure out what to do for images
All methods explained above require a user=username field

enum class Mode { NORMAL = 0, XMAS = 1, RAINBOW = 2, SHIFTING = 3 } mode;

{'method': 'POST', 'args': ['name'], 'values': {'name': 'jonah'}, 'content-type': 'application/x-www-form-urlencoded', 'is_json': False, 'data': b'', 'form': {}}
"""

import sys
sys.path.append('/var/jail/home/jblt/home_helpers')
from peripherals import Locks, Colors, Camera
import traceback

peripherals = {"locks": Locks(), "lights": Colors(), "camera": Camera()}


def get(req):
    if "get_state" in req["args"]:
        peripheral = req["values"]["get_state"]
        return peripherals[peripheral].handle_get(req)
    else:
        return "Pleas provide a get_state option"


def post(req):
    if "change_state" in req["args"]:
        peripheral = req["values"]["change_state"]
        return peripherals[peripheral].handle_post(req)
    else:
        return "Pleas provide a change_state option"


def request_handler(req):
    """Flask Request Handler"""
    try:
        if "user" not in req["args"]:
            return "Please provide a user in request."
        if req["method"] == "POST":
            return post(req)
        elif req["method"] == "GET":
            return get(req)
        else:
            return "Invalid request method"
    except Exception as err:
        return str(traceback.format_exc())
