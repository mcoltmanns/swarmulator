import numpy as np
import torch

from observer import Observer, train, predict, device


def learnability(artifacts, t, past_length, step, train_size, epochs=25, obs_width=128):
    """
    calculate the learnability score of a series of artifacts at index t
    models with an access to a longer past should become more accurate

    :param artifacts: a uniform-size time series
    :param t: the index at which to calculate learnability
    :param past_length: maximum for how far back to look behind t in order to predict it (in indexes, after granularity control applied)
    :param step: time series step size/granularity control (1 for all artifacts, 2 to skip every other, etc)
    :param train_size: how many training sets to build (before index t, after granularity applied)
    """
    artifact_length = artifacts.shape[1]

    lookbacks = []
    losses = []
    train_losses = []
    score = 0
    most_uncertainty = np.inf

    current_past_length = 1
    while current_past_length <= past_length:
        # starting index of the training data
        training_start = max(0, t - train_size * step)
        # array of features of the training data
        # past-length long snapshots of artifacts
        x_train = np.zeros((train_size, current_past_length, artifact_length))
        # array of labels of the training data
        # single artifacts
        y_train = np.zeros((train_size, artifact_length))

        # copy the training data
        for i in range(train_size):
            j = i * step + training_start
            x_train[i, :, :] = artifacts[j:j + current_past_length * step]
            y_train[i] = artifacts[j + current_past_length * step]

        # send data to device
        x_train = torch.from_numpy(x_train.astype(np.float32)).to(device)
        y_train = torch.from_numpy(y_train.astype(np.float32)).to(device)

        # initialize the observer
        observer = Observer(artifact_length, obs_width, 2)
        observer.to(device)

        # train the observer
        tls = train(observer, 0.01, x_train, y_train, epochs=epochs)
        train_losses.append(tls)

        # now predict
        x = np.zeros((1, current_past_length, artifact_length))
        y = np.zeros((1, artifact_length))
        x[0] = [artifacts[i] for i in range(t - current_past_length * step, t, step)]
        y[0] = artifacts[t]
        x = torch.from_numpy(x.astype(np.float32)).to(device)
        y = torch.from_numpy(y.astype(np.float32)).to(device)
        pred, loss = predict(observer, x, y)
        loss = loss[0].item()

        lookbacks.append(current_past_length)
        losses.append(loss)

        if loss < most_uncertainty:
            score += 1
            most_uncertainty = loss

        current_past_length *= 2

    score /= len(lookbacks)

    return score, lookbacks, losses, train_losses


def novelty(artifacts, t, past_length, step, train_size, predict_size, epochs=25, obs_width=128):
    """
    Caclulate novelty at a given time
    A dataset is novel if its unpredictability increases as time goes on
    Train on lookback-artifact pairs in range (train_size, t, step), predict on lookback-artifact pairs in range (t, predict_size, step)
    """

    artifact_length = artifacts.shape[1]

    training_start = max(0, t - train_size * step)

    x_train = np.zeros((train_size, past_length, artifact_length))
    y_train = np.zeros((train_size, artifact_length))
    for i in range(train_size):
        j = i * step + training_start
        x_train[i, :, :] = artifacts[j:j + past_length * step]
        y_train[i] = artifacts[j + past_length * step]

    x_train = torch.from_numpy(x_train.astype(np.float32)).to(device)
    y_train = torch.from_numpy(y_train.astype(np.float32)).to(device)

    observer = Observer(artifact_length, obs_width, 2)
    observer.to(device)

    train_losses = train(observer, 0.01, x_train, y_train, epochs=epochs)

    x = np.zeros((predict_size, past_length, artifact_length))
    y = np.zeros((predict_size, artifact_length))
    for i in range(predict_size):
        j = i * step + t
        past_start = j - past_length * step
        x[i] = artifacts[past_start:j]
        y[i] = artifacts[j]

    x = torch.from_numpy(x.astype(np.float32)).to(device)
    y = torch.from_numpy(y.astype(np.float32)).to(device)

    preds, losses = predict(observer, x, y)

    score = 0
    least_uncertainty = -np.inf
    for loss in losses:
        if loss > least_uncertainty:
            score += 1
            least_uncertainty = loss
    score /= len(losses)

    return score, losses, train_losses
