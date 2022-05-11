import datetime
import sqlite3
import pickle
import json


class ColorState:
    SqlIndex = 1 #index in sql table
    def __init__(self):
        self.rgb = []
        self.mode = 0
        self.room_lights = True
    def to_json(self):
        return json.dumps(self, default=vars)


class LockState:
    SqlIndex = 2
    def __init__(self):
        self.should_unlock = False
        self.kocked = False
        self.was_opened = datetime.datetime.now()
    def open_door(self):
        self.was_opened = datetime.datetime.now()
    def to_json(self):
        return json.dumps({"should_unlock":self.should_unlock, "was_opened": str(self.was_opened)})

class CameraState:
    SqlIndex = 3
    def __init__(self):
        self.base64_image = ""
        self.date_taken = datetime.datetime.now()
    def save_new_picture(self, new_image):
        self.base64_image = new_image
        self.date_taken = datetime.datetime.now()
    def to_json(self):
        return json.dumps({"date_taken":str(self.date_taken), "image_data": self.base64_image})


def sql_connection(method):
    def con(self, *args, **kwargs):
        db = '/var/jail/home/jblt/smart_home_users.db'
        connection = sqlite3.connect(db)
        cursor = connection.cursor()
        cursor.execute("CREATE TABLE IF NOT EXISTS users_table (user text, color_state text, lock_state text, camera_state text)")
        retval = method(self, cursor, *args, **kwargs)
        connection.commit()
        connection.close()
        return retval
    return con


def get_user(cursor, user):
    user_entry = cursor.execute("SELECT * FROM users_table WHERE user=?", (user,)).fetchone()
    if user_entry is None:
        default_user = (user, pickle.dumps(ColorState()), pickle.dumps(LockState()), pickle.dumps(CameraState()))
        cursor.execute("INSERT into users_table VALUES (?,?,?,?)", default_user)
        user_entry = cursor.execute("SELECT * FROM users_table WHERE user=?", (user,)).fetchone()
    return (user_entry[0], pickle.loads(user_entry[1]), pickle.loads(user_entry[2]), pickle.loads(user_entry[3]))


def save_user_color(cursor, user, color_state):
    cursor.execute("UPDATE users_table SET color_state=? WHERE user=?", (pickle.dumps(color_state), user))

def save_user_lock(cursor, user, lock_state):
    cursor.execute("UPDATE users_table SET lock_state=? WHERE user=?", (pickle.dumps(lock_state), user))

def save_user_camera(cursor, user, camera_state):
    cursor.execute("UPDATE users_table SET camera_state=? WHERE user=?", (pickle.dumps(camera_state), user))
