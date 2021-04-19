
import torch.nn.functional as F
import numpy as np
import torch
import os
import seaborn as sn
import pandas as pd
import data
import matplotlib.pyplot as plt
from tqdm import tqdm
import sys

def train_single_epoch(model, optimizer, train_data, scheduler=None, loss_weight=None):

    dataset_size = len(train_data.dataset)

    total_loss = 0.
    model.train()

    pbar = tqdm(total=len(train_data), position=0, leave=False, file=sys.stdout)

    for images, labels in train_data:
        optimizer.zero_grad()
        images = images.to(model.device)
        labels = labels.to(model.device)
        output = model(images)
        loss = F.cross_entropy(output, labels, weight=loss_weight)
        loss.backward()
        optimizer.step()
        total_loss += loss.item() * labels.size(0)
        if scheduler != None:
            scheduler.step()
        pbar.update(1)

    tqdm.close(pbar)

    return total_loss / float(dataset_size)

@torch.no_grad()
def validate(model, test_data, loss_weight=None):

    dataset_size = len(test_data.dataset)

    total_loss = 0.
    n_correct = 0.
    model.eval()

    pbar = tqdm(total=len(test_data), position=0, leave=False, file=sys.stdout)

    for images, labels in test_data:
        images = images.to(model.device)
        labels = labels.to(model.device)
        output = model(images)
        total_loss += F.cross_entropy(output, labels, weight=loss_weight).item() * labels.size(0)
        predict = torch.argmax(output, -1)
        n_correct += (predict == labels).sum().item()
        pbar.update(1)

    tqdm.close(pbar)

    avg_loss = total_loss / float(dataset_size)
    accuracy = n_correct / float(dataset_size)

    return avg_loss, accuracy

@torch.no_grad()
def get_confusion_matrix(model, test_data, num_class=10):

    cf_mtx = np.zeros([num_class, num_class])

    model.eval()
    for images, labels in test_data:
        images = images.to(model.device)
        labels = labels.to(model.device)
        output = model(images)
        predict = torch.argmax(output, -1)

        for l, p in zip(labels, predict):
            cf_mtx[l, p] += 1

    return cf_mtx

def plot_confusion_matrix(confusion_matrix):
    num_class = confusion_matrix.shape[0]
    df_cm = pd.DataFrame(confusion_matrix, index=data.id_label(slice(num_class)),
                                columns=data.id_label(slice(num_class)))
    # plt.figure(figsize=(10,7))
    # sn.set(font_scale=1.4) # for label size
    sn.heatmap(df_cm, annot=True, fmt='g') # font size
    plt.show()

def train(model, optimizer, max_epoch, train_data,
          validation=None, scheduler=None, lr_step='epoch',
          checkpoint_dir=None, max_tolerance=-1, loss_weight=None):

    best_loss = 99999.
    tolerated = 0

    _scheduler = None
    if lr_step == 'batch':
        _scheduler = scheduler

    log = np.zeros([max_epoch, 3], dtype=np.float)

    for e in range(max_epoch):

        print('Epoch #{:d}'.format(e+1))

        log[e, 0] = train_single_epoch(model, optimizer, train_data, _scheduler, loss_weight)

        print('Train Loss: {:.3f}'.format(log[e, 0]))

        if lr_step == 'epoch' and scheduler is not None:

            scheduler.step()

        if validation is not None:

            log[e, 1], log[e, 2] = validate(model, validation, loss_weight)

            print('Val Loss: {:.3f}'.format(log[e, 1]))
            print('Val Accs: {:.3f}'.format(log[e, 2]))

            if checkpoint_dir != None and (best_loss > log[e, 1]):
                best_loss = log[e, 1]
                if not os.path.exists(checkpoint_dir):
                    os.makedirs(checkpoint_dir)
                checkpoint_path = os.path.join(checkpoint_dir, 'checkpoint'+str(e+1)+'.pth')
                torch.save(model.state_dict(), checkpoint_path)
                print('Best Loss! Saved.')
            elif max_tolerance >= 0:
                tolerated += 1
                if tolerated > max_tolerance:
                    return log[0:e, :]

    return log

@torch.no_grad()
def inference(model, image):

    model.eval()
    transform = data.get_transform('test')
    x = transform(image)
    x = x.unsqueeze(0)
    x = x.to(model.device)

    output = model(x)
    predict = torch.argmax(output, -1).item()

    return predict
#
# def run(model, image):
#
#     image = config.IMAGE_TRANFORM_INFERENCE(image)
#     image = image.unsqueeze(0)
#     image = image.to(model.device)
#
#     with torch.no_grad():
#         model.eval()
#         output = model(image)
#         predict = torch.argmax(output, -1).item()
#
#     return predict

if __name__ == '__main__':


# if __name__ == '__main__':
#
    import argparse
    import data
    from torch.utils.data import DataLoader
    from torch.optim import Adam
    import model
    from PIL import Image

    parser = argparse.ArgumentParser()
    parser.add_argument('--image', type=str, default=None)
    parser.add_argument('--weights', type=str, required=True)
    args = parser.parse_args()
#
#     dataset = data.WasteNetDataset(args.data_dir)
#     trainset, valset, testset = dataset.split(0.7, 0.1, 0.2)
#     trainloader = DataLoader(trainset, batch_size=64, shuffle=True, drop_last=True)
#     valloader = DataLoader(valset, batch_size=64, shuffle=False)
#     testloader = DataLoader(testset, batch_size=64, shuffle=False)
#
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    net = model.WasteNet().to(device)
#     if args.weights != None:
    net.load_state_dict(torch.load(args.weights, map_location=device))
    label = inference(net, Image.open(args.image).convert('RGB'))
    print(data.id_label(label))
#     optimizer = Adam(net.parameters(), lr=1e-4)
#     train(net, optimizer, args.n_epoch, trainloader, valloader, args.save, -1)
#
#     torch.save(net.state_dict(), os.path.join(args.save, 'final.pth'))
