import torch
import torch.nn as nn
import numpy as np
from tqdm import tqdm

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

class Observer(nn.Module):
    def __init__(self, input_dim: int, hidden_dim: int, num_layers: int):
        super(Observer, self).__init__()
        self.M = hidden_dim
        self.L = num_layers

        self.nn = nn.LSTM(
            input_size=input_dim,
            hidden_size=hidden_dim,
            num_layers=num_layers,
            batch_first=True,
        )
        self.final = nn.Linear(hidden_dim, input_dim)

    def forward(self, x):
        h0 = torch.zeros(self.L, x.size(0), self.M).to(device)
        c0 = torch.zeros(self.L, x.size(0), self.M).to(device)

        out, (h, c) = self.nn(x, (h0.detach(), c0.detach()))
        out = self.final(out[:, -1, :])

        return out

def train(model, learning_rate, x_train, y_train, epochs=200):
    criterion = nn.MSELoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate, weight_decay=1e-5)

    train_losses = np.zeros(epochs)

    for epoch in range(epochs):
        optimizer.zero_grad()

        # forward
        outs = model(x_train)
        loss = criterion(outs, y_train)

        # backward
        loss.backward()
        optimizer.step()

        # train loss
        train_losses[epoch] = loss.item()

        if (epoch + 1) % 100 == 0:
            print(f"Epoch {epoch + 1}/{epochs}, Loss: {train_losses[epoch]:.3f}")

    return train_losses

def predict(model, x, y):
    outs = model(x)
    pairwise_losses = (outs - y) ** 2
    mean_losses = torch.mean(pairwise_losses, dim=1)
    return outs.detach().cpu(), mean_losses.detach().cpu()
