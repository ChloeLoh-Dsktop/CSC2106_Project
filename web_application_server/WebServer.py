from flask import Flask, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO, emit
from datetime import datetime  # Import if you need to use datetime

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret_key'  # It's important to set a secret key for WebSocket encryption
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+pymysql://root:Vov37717@localhost/dummydata'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
socketio = SocketIO(app, cors_allowed_origins="*")  # Initialize your SocketIO app, allowing CORS for development

class ParkingData(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    parking_lot_id = db.Column(db.Integer, nullable=False)
    timestamp = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)
    occupancy_status = db.Column(db.Boolean, nullable=False)

    def to_dict(self):
        return {
            'id': self.id,
            'parking_lot_id': self.parking_lot_id,
            'timestamp': self.timestamp.isoformat(),
            'occupancy_status': self.occupancy_status
        }

@app.route('/api/sensor_data', methods=['GET'])
def get_sensor_data():
    sensor_data = ParkingData.query.all()
    return jsonify({'sensor_data': [data.to_dict() for data in sensor_data]})

@app.route('/')
def index():
    return 'Welcome to the Smart Parking System!'

# Example WebSocket event
@socketio.on('connect')
def test_connect():
    print('Client connected')

@socketio.on('disconnect')
def test_disconnect():
    print('Client disconnected')

@socketio.on('request_update')
def handle_request_update():
    sensor_data = ParkingData.query.all()
    emit('update_data', {'sensor_data': [data.to_dict() for data in sensor_data]})



if __name__ == '__main__':
    socketio.run(app, port=8000, debug=True)