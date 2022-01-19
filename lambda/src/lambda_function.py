import requests
import json
import os

def lambda_handler(event, context):
    print("Received event: " + json.dumps(event))
    data = event["decoded"]["payload"]
    TG_TOKEN = os.environ["TG_TOKEN"]
    TG_CHAT_ID = os.environ["TG_CHAT_ID"]
    text = ""
    alert = False

    if data["label"] == 1:
        alert = True
        text = "Human detected at location {lat}, {lon}".format(lat = data["latitude"],lon = data["logitude"] )
    elif data["label"] == 2:
        alert = True
        text = "Animal detected at location {lat}, {lon}".format(lat = data["latitude"],lon = data["logitude"] )
    else:
        print("No event to alert")
        
    if alert:
        url = "https://api.telegram.org/bot{TG_TOKEN}/sendMessage".format(TG_TOKEN=TG_TOKEN)
        payload = json.dumps({
            "chat_id": TG_CHAT_ID,
            "text": text
        })
        headers = {
            'Content-Type': 'application/json'
        }

        response = requests.request("POST", url, headers=headers, data=payload)
        print(response.text)
    
    return "complete"
