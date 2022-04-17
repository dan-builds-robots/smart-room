import datetime
import sqlite3
import pickle
import json


class ColorState:
    index = 1 #index in sql table
    def __init__(self):
        self.rgb = []
        self.room_lights = True
    def to_json(self):
        return json.dumps(self, default=vars)


class LockState:
    index = 2
    def __init__(self):
        self.should_unlock = False
        self.was_opened = datetime.datetime.now()
    def open_door(self):
        self.was_opened = datetime.datetime.now()
    def to_json(self):
        return json.dumps({"should_unlock":self.should_unlock, "was_opened": str(self.was_opened)})


def sql_connection(method):
    def con(self, *args, **kwargs):
        db = '/var/jail/home/jblt/smart_home_users.db'
        connection = sqlite3.connect(db)
        cursor = connection.cursor()
        cursor.execute("CREATE TABLE IF NOT EXISTS users_table (user text, color_state text, lock_state text)")
        retval = method(self, cursor, *args, **kwargs)
        connection.commit()
        connection.close()
        return retval
    return con


def get_user(cursor, user):
    user_entry = cursor.execute("SELECT * FROM users_table WHERE user=?", (user,)).fetchone()
    if user_entry is None:
        default_user = (user, pickle.dumps(ColorState()), pickle.dumps(LockState()))
        cursor.execute("INSERT into users_table VALUES (?,?,?)", default_user)
        user_entry = cursor.execute("SELECT * FROM users_table WHERE user=?", (user,)).fetchone()
    return (user_entry[0], pickle.loads(user_entry[1]), pickle.loads(user_entry[2]))


def save_user(cursor, user, *, color_state = None, lock_state = None):
    update_str = "UPDATE users_table SET "
    updated_user = []
    if color_state is None and lock_state is None:
        return
    if color_state is not None:
        updated_user.append(pickle.dumps(color_state))
        update_str += "color_state=?"
    elif lock_state is not None:
        updated_user.append(pickle.dumps(lock_state))
        update_str += ("" if color_state is None else ",") + "lock_state=? "
    updated_user.append(user)
    cursor.execute(update_str + "WHERE user=?", tuple(updated_user))

