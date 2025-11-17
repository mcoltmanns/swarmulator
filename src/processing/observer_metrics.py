import numpy as np
import torch
from sklearn.preprocessing import StandardScaler

from observer import Observer, train, predict

def learnability(artifacts, t, lookback, train_size, train_epochs, obs_width, file, device):
    """
    artifacts before t will be used to build the training set, try to predict t
    system should become more predictable as the lookback increases (observer can see farther into the past)
    writes metadata (training info, scores, etc.) to a group named "learnability_<t>_meta"
    returns normalized learnability score for t
    :param artifacts: bag-of-words simulation artifacts
    :param t: time to predict at
    :param lookback: how many artifacts make up a feature/how long is the past
    :param train_size: training set size
    :param train_epochs: how many epochs to train for (more than 25 shouldn't be necessary)
    :param obs_width: width of observer hidden layers (more than 256 gives OOM) (best results seem to be 128)
    :param file: h5 file or group object to write to
    :param device: torch device to run on
    :return:
    """
    write_group = file.require_group(f"learnability_{t}_meta")
    artifact_dim = len(artifacts[0])
    train_size = min(min(train_size, len(artifacts)), t) # the number of data points available for training

    scaler = StandardScaler()
    scaler.fit(artifacts)
    artifacts = scaler.transform(artifacts)

    lookbacks = []
    losses = []
    score = 0
    most_uncertainty = np.inf

    # iterate over lookback lengths
    # start with a lookback of 1 and double until greater than maximum
    l = 1
    while l <= lookback:
        train_start = max(0, t - train_size) # index to the start of training data
        x_train = np.zeros((train_size, l, artifact_dim)) # training inputs are train_size-long sets of lookback-long frames of state
        y_train = np.zeros((train_size, artifact_dim)) # training outputs are always one state
        # copy in the inputs/outputs
        for i in range(train_size):
            t = train_start + i # current time index
            x_train[i, :, :] = artifacts[t : t + l] # the feature is all the artifacts from time up to but not including time + lookback
            y_train[i] = artifacts[t + l] # the label is the artifact at time + lookback (the thing immediately after the training set)
        # send to gpu
        x_train = torch.from_numpy(x_train.astype(np.float32)).to(device)
        y_train = torch.from_numpy(y_train.astype(np.float32)).to(device)
        # initialize observer
        observer = Observer(artifact_dim, obs_width, 2)
        observer.to(device)
        # train on the data
        train_losses = train(observer, 0.01, x_train, y_train, epochs=train_epochs)
        write_group.require_group("training loss").create_dataset(f'time {t} lookback {l}', data=train_losses)

        # now try to predict the artifact at t
        x = np.zeros((1, l, artifact_dim)) # we are predicting one thing based on the stuff that came before it
        y = np.zeros((1, artifact_dim))
        x[0] = artifacts[t - l : t]
        y[0] = artifacts[t]
        x = torch.from_numpy(x.astype(np.float32)).to(device)
        y = torch.from_numpy(y.astype(np.float32)).to(device)
        pred, loss = predict(observer, x, y)
        loss = loss[0].item()

        lookbacks.append(l)
        losses.append(loss)
        # if the current loss was less than our worse loss, the data got more learnable
        if loss < most_uncertainty:
            score += 1
            most_uncertainty = loss

        l *= 2

    score /= len(lookbacks)

    write_group.create_dataset("lookbacks", data=lookbacks)
    write_group.create_dataset("prediction loss", data=losses)

    return score

def novelty(artifacts, t, lookback, train_size, train_epochs, predict_size, obs_width, file, device):
    """
    Calculate the novelty at a given time.
    A dataset is novel if its unpredictability increases as time goes on.
    Train on lookback-artifact pairs in range (train_size, t), predict on lookback-artifact pairs in range (t, predict_size).
    Writes metadata (training info, prediction losses, etc.) to a group named "novelty_<t>_meta".
    Returns normalized novelty score for t.
    :param artifacts: bag-of-words simulation artifacts
    :param t: time to predict at
    :param lookback: how many artifacts make up a feature/how long is the past
    :param train_size: training set size
    :param train_epochs: how many epochs to train for (more than 50 shouldn't be necessary)
    :param predict_size: prediction set size
    :param obs_width: width of observer hidden layers (more than 256 gives OOM) (best results seem to be 128)
    :param file: h5 file or group object to write to
    :param device: torch device to run on
    :return:
    """
    write_group = file.require_group(f"novelty_{t}_meta")
    artifact_dim = len(artifacts[0])
    # number of data points to train on
    # may not be greater than the total number of artifacts, and may not be greater than t (since we train on points before t)
    train_size = min(min(train_size, len(artifacts)), t)
    # number of data points to predict on
    # may not be greater than the total number of artifacts left over after training
    predict_size = min(predict_size, len(artifacts) - train_size)

    #print(f'Novelty at {t} - training on {t - train_size} to {t - 1}, predicting from {t} to {t + predict_size}.')

    # normalize the input
    scaler = StandardScaler()
    scaler.fit(artifacts)
    artifacts = scaler.transform(artifacts)

    # get the data to train on
    x_train = np.zeros((train_size, lookback, artifact_dim))
    y_train = np.zeros((train_size, artifact_dim))
    for i in range(train_size):
        t_ = t - train_size + i # index from the start of the training range
        x_train[i, :, :] = artifacts[t_ : t_ + lookback] # input is everything from the current start index right up to that index plus the lookback
        y_train[i] = artifacts[t_ + lookback] # output is the point immediately after the training range

    # move training data to device
    x_train = torch.from_numpy(x_train.astype(np.float32)).to(device)
    y_train = torch.from_numpy(y_train.astype(np.float32)).to(device)

    observer = Observer(artifact_dim, obs_width, 2)
    observer.to(device)

    # train
    train_losses = train(observer, 0.01, x_train, y_train, epochs=train_epochs)
    # save the data from the training session
    write_group.create_dataset("training loss v epoch", data=train_losses)

    # now the observer is trained on the data from t - train_size to t - 1
    # now we try to predict artifacts after t based on the data before them
    # want uncertainty to increase

    # put together test data
    x = np.zeros((predict_size, lookback, artifact_dim))
    y = np.zeros((predict_size, artifact_dim))
    for i in range(predict_size):
        t_predict = t + i # the artifact we want to predict
        hist_start = t_predict - lookback # the index where the history for this prediction starts
        x[i] = artifacts[hist_start : t_predict]
        y[i] = artifacts[t_predict]

    # move test data to device
    x = torch.from_numpy(x.astype(np.float32)).to(device)
    y = torch.from_numpy(y.astype(np.float32)).to(device)
    # run the predictor
    predictions, losses = predict(observer, x, y)

    # count the number of times we get worse at predicting the future
    # is this the way to do it? produces extremely low scores. somehow need to normalize relative to the length of the future you're trying to predict in
    score = 0
    least_uncertainty = 0
    for loss in losses:
        if loss > least_uncertainty:
            score += 1
            least_uncertainty = loss
    score /= len(losses) # then normalize that score

    write_group.create_dataset("prediction loss", data=losses)

    return score

"""
arts_file = h5.File(artifact_path, 'r')
data_file = h5.File(data_save_path, 'w')
arts = arts_file['features']
times = arts_file['times']

sample_inds = np.linspace(15_000, len(arts) - 15_000, samples, endpoint=False, dtype=int)
times_sampled = times[sample_inds]

novelties = []
learnabilities = []
with tqdm(desc="Running", total=len(times_sampled), unit="sample") as pbar:
    for i in sample_inds:
        n = novelty(arts, i, 15_000, 15_000, data_file, obs_hidden_dim)
        novelties.append(n)
        l = learnability(arts, i, 15_000, data_file, obs_hidden_dim)
        learnabilities.append(l)
        pbar.update()

data_file.create_dataset('novelty', data=novelties)
data_file.create_dataset('learnability', data=learnabilities)
data_file.create_dataset('time', data=times_sampled)
data_file.close()
"""
