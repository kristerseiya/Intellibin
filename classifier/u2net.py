
import torch
from torch import nn
import torch.nn.functional as F
import model

if __name__ == '__main__':

        import argparse
        from PIL import Image
        from torchvision import transforms
        import numpy as np

        parser = argparse.ArgumentParser()
        parser.add_argument('--weight', required=True)
        parser.add_argument('--image', required=True)
        args = parser.parse_args()

        net = model.U2NET()
        net.load_state_dict(torch.load(args.weight, map_location=torch.device('cpu')))
        net.eval()

        src = Image.open(args.image).convert('RGB')
        img = src.resize([320, 320])

        x = transforms.ToTensor()(img)
        x = transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])(x)
        x = x.view(1, *x.size())
        mask, d2, d3, d4, d5, d6, d7 = net(x)
        mask = mask[:, 0, :, :]
        mask = (mask - torch.min(mask)) / (torch.max(mask) - torch.min(mask))
        mask = mask.squeeze(0)

        mask = mask.detach().cpu().numpy()
        img = np.array(img)
        res = img * mask[:, :, np.newaxis] + (np.ones_like(img)*255) * (1 - mask[:, :, np.newaxis])
        # mask = Image.fromarray((mask*255).astype(np.uint8)).convert('L')
        # mask = mask.resize(src.size)
        #
        # empty = np.ones([src.size[1], src.size[0], 3]) * 255
        # empty = Image.fromarray(empty.astype(np.uint8)).convert('RGBA')
        # # empty = np.zeros([*src.size, ])
        # # empty = Image.new("RGBA", src.size)
        # mask = np.ones([src.size[1], src.size[0]]) * 255
        # mask = Image.fromarray(mask.astype(np.uint8)).convert('L')
        # img = Image.composite(src, empty, mask)
        img = Image.fromarray(res.astype(np.uint8))
        img = img.resize(src.size)
        img.show()
