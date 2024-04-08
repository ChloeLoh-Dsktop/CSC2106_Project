# routing.py
from django.urls import path
from .consumers import SensorDataConsumer

websocket_urlpatterns = [
    path('ws/sensor_data/', SensorDataConsumer.as_asgi()),
]