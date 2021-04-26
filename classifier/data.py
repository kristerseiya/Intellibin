
import torch
from torch.utils.data import Dataset
from torchvision import transforms
from glob import glob
from PIL import Image
import os
import numpy as np
from tqdm import tqdm
import sys

class AddNoise():
    def __init__(self, max=15):
        self.max = max / 255.

    def __call__(self, x):
        return x + torch.randn_like(x) * self.max * torch.rand(1).item()

def get_transform(mode):
    if mode == 'train':
        transform = transforms.Compose([transforms.RandomResizedCrop(224, scale=(0.5, 1.0)),
                                              transforms.RandomAffine(180, shear=15),
                                              transforms.ColorJitter(0.8, 0.7, 0.7, 0.12),
                                              transforms.RandomVerticalFlip(),
                                              transforms.RandomHorizontalFlip(),
                                              transforms.ToTensor(),
                                              AddNoise(15),
                                              transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
                                             ])

    elif mode in ['val', 'test']:
        transform = transforms.Compose([transforms.Resize(224),
                                              transforms.CenterCrop(224),
                                              transforms.ToTensor(),
                                              transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
                                             ])

    elif mode == 'none':
        transform = transforms.Compose([])

    return transform

def get_label(obj_type):
    if obj_type.startswith('cardboard'):
        # return 0
        return 0
    elif obj_type.startswith('paper_bag'):
        # return 0
        return 0
    elif obj_type.startswith('paper_box'):
        return 1
    elif obj_type.startswith('paper'):
        return 1
    elif obj_type.startswith('glass'):
        # return 2
        return 2
    elif obj_type.startswith('metal_can'):
        # return 3
        return 3
    elif obj_type.startswith('metal'):
        # return 4
        return 3
    elif obj_type.startswith('plastic_bottle'):
        # return 5
        return 4
    elif obj_type.startswith('milk_jug'):
        # return 5
        return 4
    elif obj_type.startswith('plastic_box'):
        # return 6
        return 5
    elif obj_type.startswith('other_plastic'):
        # return 6
        return 5
    elif obj_type.startswith('plastic_bag'):
        # return 7
        return 6
    elif obj_type.startswith('disposable_cup'):
        # return 8
        return 7
    elif obj_type.startswith('snack_wrapper') or obj_type.startswith('food_wrap') or \
        obj_type.startswith('nontrans_plastic_bag_me'):
        # return 9
        return 8
    elif obj_type.startswith('dirty_plate') or obj_type.startswith('food'):
        # return 10
        return 8
    elif obj_type.startswith('pizza_box') or obj_type.startswith('styrofoam'):
        # return 11
        return 8
    elif obj_type.startswith('battery') or obj_type.startswith('electronic') or \
        obj_type.startswith('other'):
        # return 12
        return 8
    return 8


def id_label(label):
    classes = ['paper 1', 'paper 2', 'glass', 'metal', 'plastic bottle',
               'other plastic', 'plastic bag', 'disposable cup', 'other']
    return classes[label]

def isrecyclable(label):
    return label < 6

class WasteNetSubset(Dataset):
    def __init__(self, dataset, indices, mode='none'):
        super().__init__()
        self.images = dataset.images
        self.labels = dataset.labels
        self.indices = indices
        self.transform = get_transform(mode)
        self.n_class = dataset.n_class
        self.add_idx = list()

    def __getitem__(self, idx):
        if idx >= len(self.indices):
            imgs = self.images[self.indices[self.add_idx[idx-len(self.labels)]]]
            labels = self.labels[self.indices[self.add_idx[idx-len(self.labels)]]]
        else:
            imgs = self.images[self.indices[idx]]
            labels = self.labels[self.indices[idx]]
        return self.transform(imgs), labels

    def __len__(self):
        return len(self.indices) + len(self.add_idx)

    def set_mode(self, mode):
        self.transform = get_transform(mode)

    def print_stats(self):
        tmp = np.array(self.labels)
        (unique, counts) = np.unique(tmp[self.indices], return_counts=True)
        n_recyc = 0
        n_nonrecyc = 0
        print('{:d} classes\n'.format(len(unique)))
        for u, c in zip(unique, counts):
            print('{:s}: {:d}'.format(id_label(u), c))
            if isrecyclable(u):
                n_recyc += c
            else:
                n_nonrecyc += c
        print('\nRecyclable: {:d}'.format(n_recyc))
        print('Non-recyclable: {:d}'.format(n_nonrecyc))

    def repeat(self, nums):
        self.add_idx = list()
        for i in range(len(self.indices)):
            for _ in range(nums[self.labels[self.indices[i]]]-1):
                self.add_idx.append(i)


class WasteNetDataset(Dataset):
    def __init__(self, root_dirs, mode='none', exclude='google'):
        super().__init__()
        self.images = list()
        self.obj_types = list()
        self.store = 'RAM'
        self.transform = get_transform(mode)
        self.add_idx = list()

        if type(root_dirs) != list:
            root_dirs = list(root_dirs)

        if type(exclude) != list:
            exclude = list(exclude)

        file_paths = list()
        for root_dir in root_dirs:
            subdirs = glob(os.path.join(root_dir, '*'))
            for subdir in subdirs:
                ignore = False
                for exc in exclude:
                    if subdir.endswith(exc):
                        ignore = True
                        break
                if not ignore:
                    for ext in ['png', 'jpg']:
                        file_paths += glob(os.path.join(subdir, '*.'+ext))
        n_files = len(file_paths)

        pbar = tqdm(total=n_files, position=0, leave=False, file=sys.stdout)

        for file_path in file_paths:
            self.obj_types.append(file_path.split('/')[-2])
            if self.store == 'RAM':
                fptr = Image.open(file_path).convert('RGB')
                file_copy = fptr.copy()
                fptr.close()
                self.images.append(file_copy)
            elif self.store == 'DISK':
                self.images.append(file_path)
            pbar.update(1)

        tqdm.close(pbar)

    def create_labels(self):
        self.labels = list()
        for obj_type in self.obj_types:
            a = get_label(obj_type)
            self.labels.append(a)
        self.n_class = max(self.labels) + 1

    def set_mode(self, mode):
        self.transform = get_transform(mode)

    def __len__(self):
        return len(self.labels) + len(self.add_idx)

    def __getitem__(self, idx):
        if idx >= len(self.labels):
            imgs = self.images[self.add_idx[idx-len(self.labels)]]
            labels = self.labels[self.add_idx[idx-len(self.labels)]]
        else:
            imgs = self.images[idx]
            labels = self.labels[idx]
        return self.transform(imgs), labels
        #
        # if self.store == 'DISK':
        #     return self.transform(Image.open(self.images[idx]).convert('RGB')), self.labels[idx]
        # return self.transform(self.images[idx]), self.labels[idx]

    def print_stats(self):
        (unique, counts) = np.unique(self.labels, return_counts=True)
        for u, c in zip(unique, counts):
            print('{:s}: {:d}'.format(id_label(u), c))

    def print_stats(self):
        (unique, counts) = np.unique(self.labels, return_counts=True)
        n_recyc = 0
        n_nonrecyc = 0
        print('{:d} classes\n'.format(len(unique)))
        for u, c in zip(unique, counts):
            print('{:s}: {:d}'.format(id_label(u), c))
            if isrecyclable(u):
                n_recyc += c
            else:
                n_nonrecyc += c
        print('\nRecyclable: {:d}'.format(n_recyc))
        print('Non-recyclable: {:d}'.format(n_nonrecyc))

    def split(self, *r):
        ratios = np.array(r)
        ratios = ratios / ratios.sum()
        total_num = len(self.labels)
        indices = np.arange(total_num)
        np.random.shuffle(indices)

        subsets = list()
        start = 0
        for r in ratios[:-1]:
            split = int(total_num * r)
            subsets.append(WasteNetSubset(self, indices[start:start+split]))
            start = start + split
        subsets.append(WasteNetSubset(self, indices[start:]))

        return subsets

    def repeat(self, nums):
        self.add_idx = list()
        for i in range(len(self.labels)):
            for _ in range(nums[self.labels[i]]-1):
                self.add_idx.append(i)
