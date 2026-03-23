import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import numpy as np
import matplotlib.pyplot as plt

# 修改后的模型（简化Transformer为CNN+LSTM或纯CNN）
class LightweightCNN(nn.Module):
    def __init__(self, num_sensors=8, seq_len=100, num_classes=4):
        super().__init__()
        self.cnn = nn.Sequential(
            nn.Conv1d(num_sensors, 32, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.MaxPool1d(2),
            nn.Conv1d(32, 32, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.MaxPool1d(2),
            nn.Flatten(),
            nn.Linear(32 * (seq_len//4), 64),
            nn.ReLU(),
            nn.Linear(64, num_classes)
        )
    
    def forward(self, x):
        # x: [batch, seq_len, num_sensors] -> [batch, num_sensors, seq_len]
        x = x.permute(0, 2, 1)
        return self.cnn(x)

# 保存预训练权重
model = LightweightCNN()
torch.save(model.state_dict(), "cnn_model.pth")