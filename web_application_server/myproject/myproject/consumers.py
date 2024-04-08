# consumers.py
import json
import aiohttp
from channels.generic.websocket import AsyncWebsocketConsumer

class SensorDataConsumer(AsyncWebsocketConsumer):
    async def connect(self):
        await self.accept()

    async def disconnect(self, close_code):
        pass

    async def receive(self, text_data):
        text_data_json = json.loads(text_data)
        # Check the message action
        action = text_data_json.get('action')
        
        if action == 'request_update':
            # Make an asynchronous HTTP request to the Flask server
            async with aiohttp.ClientSession() as session:
                async with session.get('http://127.0.0.1:5000/api/sensor_data') as response:
                    if response.status == 200:
                        sensor_data = await response.json()
                        # Send the sensor data back to the WebSocket client
                        await self.send(text_data=json.dumps({
                            'sensor_data': sensor_data
                        }))
                    else:
                        await self.send(text_data=json.dumps({
                            'error': 'Failed to fetch sensor data'
                        }))
