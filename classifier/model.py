
import torch
from torch import nn
from torchvision.models import mobilenet_v2


class WasteNet(nn.Module):
    def __init__(self, n_class):
        super().__init__()
        self.featureExtracter = mobilenet_v2(pretrained=True).features
        self.classifier = nn.Sequential(nn.Dropout(0.2), nn.Linear(1280*7*7, n_class))
        self.device = torch.device('cpu')

    def forward(self, x):
        x = self.featureExtracter(x)
        x = x.view(-1, 1280*7*7)
        x = self.classifier(x)
        return x

    def move(self, device):
        newself = self.to(device)
        newself.device = device
        return newself
