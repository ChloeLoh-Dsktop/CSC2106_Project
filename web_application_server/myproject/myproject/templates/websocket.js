const socket = new WebSocket('ws://127.0.0.1:8000/ws/sensor_data/');

socket.onmessage = function(e) {
    const data = JSON.parse(e.data);
    console.log(data);
    // Update the DOM with new data
};

socket.onclose = function(e) {
    console.error('WebSocket closed unexpectedly');
};

// You can also send messages to the server if needed
// socket.send(JSON.stringify({message: 'Hello'}));
