import sqlite3
import json
import datetime

smart_room_db = "/var/jail/home/dajayi/smart_room/smart_room.db" # just come up with name of database

def request_handler(request):

    # database stuff
    conn = sqlite3.connect(smart_room_db)
    c = conn.cursor()
    # c.execute('''CREATE TABLE IF NOT EXISTS thermostat_table (light_status real, mod_temp real, target_temp real, status text, time timestamp);''') # run a CREATE TABLE command
    c.execute('''CREATE TABLE IF NOT EXISTS smart_room_table (light_status real, time timestamp);''') # create table if it doesn't exist

    if request['method'] == 'GET':
        pass
    elif request['method'] == 'POST':

        # get the light status
        lights_status = request['form']['light_status']

        # insert it into the database
        time = datetime.datetime.now()
        conn.execute('''INSERT into smart_room_table VALUES(?,?);''', (lights_status, time))    
    
        conn.commit()
        conn.close()
        return "successfully posted to database"


    conn.commit()
    conn.close()
    return "success"