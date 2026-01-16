import numpy as np
import torch

from observer import Observer, train, predict, device


def learnability(artifacts, t, past_length, step, training_start, epochs=25, obs_width=128, n_samples=10_000):
    """
    calculate the learnability score of a series of artifacts at index t
    models with an access to a longer past should become more accurate

    :param artifacts: a uniform-size time series
    :param t: the index at which to calculate learnability
    :param past_length: maximum for how far back to look behind t in order to predict it (in indexes, after granularity control applied)
    :param step: time series step size/granularity control (1 for all artifacts, 2 to skip every other, etc)
    :param training_start: data in the range [train_start, t) is used to train the model. if the range provides more samples than the maximum (10_000), the 10k most recent samples are used.

    no more than 10k samples or we run out of memory!
    """
    artifact_length = artifacts.shape[1]

    lookbacks = []
    losses = []
    train_losses = []
    score = 0
    most_uncertainty = np.inf

    current_past_length = 1

    while current_past_length <= past_length:
        """
        for a label at index i:
        y = artifacts[i]
        x = [artifacts[i - step], artifacts[i - 2 * step], ... , artifacts[i - current_past_length * step]]
        """
        first_i = training_start + current_past_length * step
        last_i = t - 1

        if first_i > last_i:
            print(f"no valid training windows in range (first was {first_i}, last was {last_i}, skip is {step})")
            current_past_length *= 2
            continue

        first_i = max(first_i, last_i - n_samples + 1)
        n_samples = last_i - first_i + 1

        # print(f'{n_samples} samples, start {first_i}, end {last_i}, length {current_past_length}')

        x_train = np.zeros((n_samples, current_past_length, artifact_length))
        y_train = np.zeros((n_samples, artifact_length))

        for idx, i in enumerate(range(first_i, last_i + 1)):
            x = [artifacts[j] for j in range(i - step, i - current_past_length * step - 1, -step)]
            x_train[idx] = x
            y_train[idx] = artifacts[i]

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


def novelty(artifacts, t, past_length, step, training_start, predict_size, epochs=25, obs_width=128, n_samples=10_000):
    """
    Caclulate novelty at a given time
    A dataset is novel if its unpredictability increases as time goes on
    Train on lookback-artifact pairs in range (train_size, t, step) (or however much of that fits in memory), predict on lookback-artifact pairs in range (t, predict_size, step)
    """

    artifact_length = artifacts.shape[1]

    first_i = training_start + past_length * step
    last_i = t - 1

    if first_i > last_i:
        print(f"no valid training windows in range (first was {first_i}, last was {last_i}, skip is {step})")
        return

    first_i = max(first_i, last_i - n_samples + 1)
    n_samples = last_i - first_i + 1

    # print(f'{n_samples} samples, start {first_i}, end {last_i}, length {past_length}')

    x_train = np.zeros((n_samples, past_length, artifact_length))
    y_train = np.zeros((n_samples, artifact_length))

    for idx, i in enumerate(range(first_i, last_i + 1)):
        x = [artifacts[j] for j in range(i - step, i - past_length * step - 1, -step)]
        x_train[idx] = x
        y_train[idx] = artifacts[i]

    x_train = torch.from_numpy(x_train.astype(np.float32)).to(device)
    y_train = torch.from_numpy(y_train.astype(np.float32)).to(device)

    observer = Observer(artifact_length, obs_width, 2)
    observer.to(device)

    train_losses = train(observer, 0.01, x_train, y_train, epochs=epochs)

    x = np.zeros((predict_size, past_length, artifact_length))
    y = np.zeros((predict_size, artifact_length))
    for idx, i in enumerate(range(t, t + predict_size)):
        x_ = [artifacts[j] for j in range(i - step, i - past_length * step - 1, -step)]
        x[idx] = x_
        y[idx] = artifacts[i]

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
