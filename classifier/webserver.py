

import io
import torch
from torchvision import models
import torchvision.transforms as transforms
import torch.nn.functional as F
from PIL import Image
from flask import Flask, jsonify, request
from flask import render_template, Response
import numpy as np

import model
import data



app = Flask(__name__)

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
classifier = model.WasteNet(9).move(device)
classifier.load_state_dict(torch.load('weights/wastenet_t.pth', map_location=device))
classifier.eval()

b = torch.zeros(1, 3, 320, 320, device=device)
b = transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])(b)

@app.route('/predict', methods=['POST'])
def predict():
    if request.method == 'POST':
        global img_bytes
        global class_name
        img_bytes = request.data
        img = Image.open(io.BytesIO(img_bytes)).convert('RGB')
        img.show()
        x = data.get_transform('test')(img)
        x = x.unsqueeze(0)
        x = x.to(device)
        output = classifier(x)
        predict = torch.argmax(output, -1).item()
        return data.id_label_recyc(predict)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, threaded=True)
