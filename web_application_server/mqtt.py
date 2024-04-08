from Crypto.Cipher import AES
import json
import paho.mqtt.subscribe as subscribe
import pymysql

# Function to connect to MySQL database
def connect_to_database():
    try:
        connection = pymysql.connect(host=db_host, user=db_user, password=db_password, db=db_name)
        cursor = connection.cursor()
        return connection, cursor
    except Exception as e:
        print("Error connecting to database:", e)

# Function to close database connection
def close_database(connection):
    try:
        connection.close()
    except Exception as e:
        print("Error closing database connection:", e)

# Function to update database with occupancy status
def update_database(lotNo, ultrasonicSensor, infraredSensor):
    try:
        # Calculate occupancy status
        occupancyStatus = 1 if ultrasonicSensor or infraredSensor else 0

        # Connect to database
        db_connection, db_cursor = connect_to_database()

        # Update database with occupancy status based on lotNo
        sql = "UPDATE parking_data SET occupancyStatus = %s WHERE lotNo = %s"
        values = (occupancyStatus, lotNo)
        db_cursor.execute(sql, values)
        db_connection.commit()
        print(f"Updated occupancy status for lotNo {lotNo} to {occupancyStatus}")

        # Close database connection
        close_database(db_connection)
    except Exception as e:
        print("Error updating database:", e)

# Function to decrypt the received payload
def decrypt_payload(payload):
    # Convert the list of integers to bytes
    received_cipher = bytes(payload)

    # Key used for encryption (must match the key used for encryption in the device)
    key = bytes([0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10])

    # Create an AES cipher object with the specified key and mode (ECB)
    cipher = AES.new(key, AES.MODE_ECB)

    # Decrypt the cipher text
    decrypted_payload = cipher.decrypt(received_cipher)

    # Extract the sensor values from the decrypted payload
    lotNo = int.from_bytes(decrypted_payload[0:4], byteorder='big', signed=False)
    ultrasonicSensor = int.from_bytes(decrypted_payload[4:8], byteorder='big', signed=False)
    infraredSensor = int.from_bytes(decrypted_payload[8:12], byteorder='big', signed=False)

    # Print the decrypted sensor values
    print("Decrypted lotNo:", lotNo)
    print("Decrypted ultrasonicSensor:", ultrasonicSensor)
    print("Decrypted infraredSensor:", infraredSensor)

    # Update database with occupancy status
    update_database(lotNo, ultrasonicSensor, infraredSensor)

# MySQL parameters
db_host = "localhost"
db_user = "root"
db_password = "Vov37717!"
db_name = "iot"

# Main loop to subscribe to MQTT messages
while True:
    try:
        m = subscribe.simple(topics=['v3/csc2106project@ttn/devices/+/up'], 
                     hostname="au1.cloud.thethings.network", port=1883, 
                     auth={'username':"csc2106project@ttn",'password':"NNSXS.5WWPYB4LV44GZHWCBGNQI7UBEORNATCL6RXJFKY.PVF3XB3UVMWSWC3KSLTRZAJTPY4GD33JF2DI2KJD5AKERUHSS6YA"})


        a = m.payload
        data = json.loads(a)

        if "decoded_payload" in data["uplink_message"]:
            decoded_payload = data["uplink_message"]["decoded_payload"]["bytes"]
            decrypt_payload(decoded_payload)

    except Exception as e:
        print("An error occurred:", e)
