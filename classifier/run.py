
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

    total_loss = 0.
    n_correct1 = 0.
    n_correct2 = 0.
    count = 0.
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
        if scheduler != None:
            scheduler.step()
        total_loss += loss.item() * labels.size(0)
        predict = torch.argmax(output, -1)
        n_correct1 += (predict == labels).sum().item()
        n_correct2 += (~(data.isrecyclable(predict) ^ data.isrecyclable(labels))).sum().item()
        count += labels.size(0)
        pbar.update(1)

    tqdm.close(pbar)

    avg_loss = total_loss / float(count)
    accuracy1 = n_correct1 / float(count)
    accuracy2 = n_correct2 / float(count)

    return avg_loss, accuracy1, accuracy2

@torch.no_grad()
def validate(model, test_data, loss_weight=None):

    total_loss = 0.
    n_correct1 = 0.
    n_correct2 = 0.
    count = 0.
    model.eval()

    pbar = tqdm(total=len(test_data), position=0, leave=False, file=sys.stdout)

    for images, labels in test_data:
        images = images.to(model.device)
        labels = labels.to(model.device)
        output = model(images)
        total_loss += F.cross_entropy(output, labels, weight=loss_weight).item() * labels.size(0)
        predict = torch.argmax(output, -1)
        n_correct1 += (predict == labels).sum().item()
        n_correct2 += (~(data.isrecyclable(predict) ^ data.isrecyclable(labels))).sum().item()
        count += labels.size(0)
        pbar.update(1)

    tqdm.close(pbar)

    avg_loss = total_loss / float(count)
    accuracy1 = n_correct1 / float(count)
    accuracy2 = n_correct2 / float(count)

    return avg_loss, accuracy1, accuracy2

def train(model, optimizer, max_epoch, train_data,
          validation=None, scheduler=None, lr_step='epoch',
          checkpoint_dir=None, max_tolerance=-1, loss_weight=None):

    best_loss = 99999.
    tolerated = 0

    _scheduler = None
    if lr_step == 'batch':
        _scheduler = scheduler

    log = np.zeros([max_epoch, 6], dtype=np.float)

    for e in range(max_epoch):

        print('Epoch #{:d}'.format(e+1))

        log[e, 0], log[e, 1], log[e, 2] = train_single_epoch(model, optimizer, train_data, _scheduler, loss_weight)

        print('Train Loss: {:.3f}'.format(log[e, 0]))
        print('Train Obj Accs: {:.3f}'.format(log[e, 1]))
        print('Train Bin Accs: {:.3f}\n'.format(log[e, 2]))

        if lr_step == 'epoch' and scheduler is not None:

            scheduler.step()

        if validation is not None:

            log[e, 3], log[e, 4], log[e, 5] = validate(model, validation, loss_weight)

            print('Val Loss: {:.3f}'.format(log[e, 3]))
            print('Val Obj Accs: {:.3f}'.format(log[e, 4]))
            print('Val Bin Accs: {:.3f}\n'.format(log[e, 5]))

            if checkpoint_dir != None and (best_loss > log[e, 3]):
                best_loss = log[e, 3]
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

@torch.no_grad()
def get_confusion_matrix(model, test_data):

    num_class = test_data.dataset.n_class
    cf_mtx1 = np.zeros([num_class, num_class])
    cf_mtx2 = np.zeros([2, 2])

    model.eval()
    for images, labels in test_data:
        images = images.to(model.device)
        labels = labels.to(model.device)
        output = model(images)
        predict = torch.argmax(output, -1)

        for l, p in zip(labels, predict):
            cf_mtx1[l, p] += 1

        labels = data.isrecyclable(labels) * 1
        predict = data.isrecyclable(predict) * 1

        for l, p in zip(labels, predict):
            cf_mtx2[l, p] += 1

    return cf_mtx1, cf_mtx2

def plot_confusion_matrix(confusion_matrix, labels=None):
    num_class = confusion_matrix.shape[0]
    if labels == None:
        labels = data.id_label(slice(num_class))
    df_cm = pd.DataFrame(confusion_matrix, index=labels, columns=labels)
    sn.heatmap(df_cm, annot=True, fmt='g')
    plt.show()

if __name__ == '__main__':

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
