from django.shortcuts import render
import requests

def dashboard(request):
    flask_api_url = 'http://127.0.0.1:5000/api/sensor_data'
    try:
        response = requests.get(flask_api_url)
        if response.status_code == 200:
            data = response.json()
            print(data)  # Debugging: Print the raw data received from Flask
        else:
            data = {"error": "Could not fetch data from the Flask app"}
    except requests.exceptions.RequestException as e:
        data = {"error": str(e)}
    
    # Debugging: Print the structure being passed to the template
    print({'data': data})  
    return render(request, 'dashboard.html', {'data': data})
