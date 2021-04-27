

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
# net.load_state_dict(torch.load('model/mar31_1103.pth', map_location=device))
# classifier.load_state_dict(torch.load('model/apr23_2334.pth', map_location=device))
classifier.load_state_dict(torch.load('model/wastenet.pth', map_location=device))
classifier.eval()
bgremover = model.U2NET()
bgremover.load_state_dict(torch.load('model/u2net.pth', map_location=device))
bgremover.eval()

b = torch.zeros(1, 3, 320, 320, device=device)
b = transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])(b)

@app.route('/predict', methods=['POST'])
def predict():
    if request.method == 'POST':
        global img_bytes
        global class_name
        img_bytes = request.data
        img = Image.open(io.BytesIO(img_bytes)).convert('RGB')
        # img.show()
        x = transforms.Resize(320)(img)
        x = transforms.CenterCrop(320)(x)
        x = transforms.ToTensor()(x)
        x = transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])(x)
        x = x.unsqueeze(0)
        x = x.to(device)
        # mask, _, _, _, _, _, _ = bgremover(x)
        # mask = mask[:, 0, :, :]
        # x = x * mask + b * (1 - mask)
        # tmp = x.detach().cpu().numpy()
        # tmp = tmp[0]
        # tmp[0, :, :] = tmp[0, :, :] * 0.229 + 0.485
        # tmp[1, :, :] = tmp[1, :, :] * 0.224 + 0.456
        # tmp[2, :, :] = tmp[2, :, :] * 0.225 + 0.406
        # tmp = tmp.transpose([1, 2, 0])
        # tmp = Image.fromarray((tmp*255).astype(np.uint8)).convert('RGB')
        # tmp.show()
        x = F.interpolate(x, size=[224, 224])
        output = classifier(x)
        predict = torch.argmax(output, -1).item()
        return data.id_label(predict)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, threaded=True)
