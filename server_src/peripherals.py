import json

import database

class Peripheral:
    def handle_post(self, req):
        raise NotImplementedError()

    def handle_get(self, req):
        raise NotImplementedError()

    def get_username(self, req):
        return req["values"]["user"]

def get_req_args(req, *names):
    retval = []
    for name in names:
        if name in req["args"]:
            retval.append(req["values"][name])
        else:
            retval.append(None)
    return tuple(retval)

class Locks(Peripheral):
    @database.sql_connection
    def handle_post(self, cursor, req):
        lock_state = self.get_lock_state(cursor, req)
        was_opened, should_unlock, knocked = get_req_args(req, "was_opened", "should_unlock", "knocked")
        if was_opened is not None:
            lock_state.open_door()
        if should_unlock is not None:
            lock_state.should_unlock = bool(should_unlock == "True")
        if knocked is not None:
            lock_state.knocked = bool(knocked == "True")
        database.save_user_lock(cursor, self.get_username(req), lock_state)
        return "Successfully Updated Lock State"

    @database.sql_connection
    def handle_get(self, cursor, req):
        lock_state = self.get_lock_state(cursor, req)
        retval = lock_state.to_json()
        #we need to update should_unlock to false for next request
        if lock_state.should_unlock:
            lock_state.should_unlock = False
        database.save_user_lock(cursor, self.get_username(req), lock_state)
        return retval

    def get_lock_state(self, cursor, req):
        return database.get_user(cursor, self.get_username(req))[database.LockState.SqlIndex]

class Colors(Peripheral):
    @database.sql_connection
    def handle_post(self, cursor, req):
        color_state = self.get_color_state(cursor, req)
        rgb_list, room_lights, mode = get_req_args(req, "rgb", "room_lights", "mode")
        if rgb_list is not None:
            color_state.rgb = json.loads(f"[{rgb_list}]")
        if room_lights is not None:
            color_state.room_lights = bool(room_lights == "True")
        if mode is not None:
            color_state.mode = int(mode)
        database.save_user_color(cursor, self.get_username(req), color_state)
        return f"Successfully Update Color State"

    @database.sql_connection
    def handle_get(self, cursor, req):
        return self.get_color_state(cursor, req).to_json()

    def get_color_state(self, cursor, req):
        return database.get_user(cursor, self.get_username(req))[database.ColorState.SqlIndex]

class Camera(Peripheral):
    
    @database.sql_connection
    def handle_post(self, cursor, req):
        new_camera = database.CameraState()
        new_camera.save_new_picture(req["data"].decode('utf-8'))
        database.save_user_camera(cursor, self.get_username(req), new_camera)
        return f"Successfully Uploaded image:{req['data']}"

    @database.sql_connection
    def handle_get(self, cursor, req):
        return self.get_camera_state(cursor, req).to_json()

    def get_camera_state(self, cursor, req):
        return database.get_user(cursor, self.get_username(req))[database.CameraState.SqlIndex]