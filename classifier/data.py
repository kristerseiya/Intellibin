
import torch
from torch.utils.data import Dataset
from torchvision import transforms
import glob
from PIL import Image
import os
import numpy as np

class AddNoise():
    def __init__(self, max=15):
        self.max = max / 255.

    def __call__(self, x):
        return x + torch.randn_like(x) * self.max * torch.rand(1).item()

def get_transform(mode):
    if mode == 'train':
        transform = transforms.Compose([transforms.RandomResizedCrop(200, scale=(0.4, 1.0)),
                                              transforms.RandomAffine(180, shear=15),
                                              transforms.ColorJitter(0.8, 0.7, 0.7, 0.12),
                                              transforms.RandomVerticalFlip(),
                                              transforms.RandomHorizontalFlip(),
                                              transforms.ToTensor(),
                                              AddNoise(15),
                                              transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
                                             ])

    elif mode in ['val', 'test']:
        transform = transforms.Compose([transforms.Resize(200),
                                              transforms.CenterCrop(200),
                                              transforms.ToTensor(),
                                              transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
                                             ])

    elif mode == 'none':
        transform = transforms.Compose([])

    return transform

def get_label(dir):
    if dir.startswith('cardboard') or dir.startswith('paper') or dir.startswith('other_paper'):
        return 0
    elif dir.startswith('glass'):
        return 1
    elif dir.startswith('metal'):
        return 2
    elif dir.startswith('plastic_bag'):
        return 3
    elif dir.startswith('plastic_bottle'):
        return 4
    elif dir.startswith('other_plastic') or dir.startswith('plastic_box'):
        return 5
    elif dir.startswith('disposable_cup'):
        return 6
    elif dir.startswith('snack_wrapper') or dir.startswith('food_wrap'):
        return 7
    elif dir.startswith('dirty_plate') or dir.startswith('food'):
        return 8
    elif dir.startswith('battery') or dir.startswith('electronic') or dir.startswith('napkin') or dir.startswith('other') \
            or dir.startswith('pizza_box') or dir.startswith('styrofoam'):
        return 9
    return -1

def id_label(label):
    classes = ['paper', 'glass', 'metal', 'plastic bag', 'plastic bottle',
               'other plastic', 'disposable cup', 'food / snack wraps',
               'food contamination','other']
    return classes[label]

def isrecyclable(label):
    if label in [0, 1, 2, 4, 5]:
        return True
    return False

class WasteNetSubset(Dataset):
    def __init__(self, dataset, indices, mode='none'):
        self.dataset = dataset
        self.indices = indices
        self.transform = get_transform(mode)

    def __getitem__(self, idx):
        imgs, labels = self.dataset[self.indices[idx]]
        return self.transform(imgs), labels

    def __len__(self):
        return len(self.indices)

    def print_stats(self):
        (unique, counts) = np.unique(self.dataset.labels[self.indices], return_counts=True)
        for u, c in zip(unique, counts):
            print('{:s}: {:d}'.format(id_label(u), c))

class WasteNetDataset(Dataset):
    def __init__(self, root_dirs, mode='none', store='ram', exclude='google'):
        super().__init__()
        self.images = list()
        self.labels = list()
        self.store = store

        if type(root_dirs) != list:
            root_dirs = list(root_dirs)

        if type(exclude) != list:
            exclude = list(exclude)

        for root_dir in root_dirs:
            for file_path in glob.glob(os.path.join(root_dir, '*/**.png')):

                ignore = False
                dir = file_path.split('/')[-2]
                for exc in exclude:
                    if dir.endswith(exc):
                        ignore = True

                if not ignore:
                    label = get_label(dir)
                    if label != -1:
                        if store == 'ram':
                            fptr = Image.open(file_path).convert('RGB')
                            file_copy = fptr.copy()
                            fptr.close()
                            self.images.append(file_copy)
                        elif store == 'disk':
                            self.images.append(file_path)
                        self.labels.append(label)

        self.transform = get_transform(mode)

    def set_mode(self, mode):
        self.transform = get_transform(mode)

    def __len__(self):
        return len(self.labels)

    def __getitem__(self, idx):
        if self.store == 'ram':
            return self.transform(self.images[idx]), self.labels[idx]
        return self.transform(Image.open(self.images[idx]).convert('RGB')), self.labels[idx]

    def print_stats(self):
        (unique, counts) = np.unique(self.labels, return_counts=True)
        for u, c in zip(unique, counts):
            print('{:s}: {:d}'.format(id_label(u), c))

    def split(self, train_r, val_r, test_r):
        ratios = np.array([train_r, val_r, test_r]) / (train_r + val_r + test_r)
        total_num = len(self.labels)
        indices = list(range(total_num))
        np.random.shuffle(indices)

        split1 = int(np.floor(total_num * ratios[0]))
        split2 = int(np.floor(total_num * ratios[1]))

        train_set = WasteNetSubset(self, indices[:split1], 'train')
        val_set = WasteNetSubset(self, indices[split1:split1+split2], 'val')
        test_set = WasteNetSubset(self, indices[split1+split2:], 'test')

        return train_set, val_set, test_set
