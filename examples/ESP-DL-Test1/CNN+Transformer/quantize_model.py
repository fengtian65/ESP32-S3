import torch
import numpy as np
from torch.utils.data import DataLoader, Dataset
from esp_ppq import QuantizationSettingFactory
from esp_ppq.api import espdl_quantize_onnx

# ======================
# 1. 定义您的模型（这里用简化版CNN为例）
# ======================
class LightweightCNN(torch.nn.Module):
    def __init__(self, num_sensors=8, seq_len=100, num_classes=4):
        super().__init__()
        self.cnn = torch.nn.Sequential(
            torch.nn.Conv1d(num_sensors, 32, kernel_size=3, padding=1),
            torch.nn.ReLU(),
            torch.nn.MaxPool1d(2),
            torch.nn.Conv1d(32, 32, kernel_size=3, padding=1),
            torch.nn.ReLU(),
            torch.nn.MaxPool1d(2),
            torch.nn.Flatten(),
            torch.nn.Linear(32 * (seq_len//4), 64),
            torch.nn.ReLU(),
            torch.nn.Linear(64, num_classes)
        )
    
    def forward(self, x):
        # x: [batch, seq_len, num_sensors] -> [batch, num_sensors, seq_len]
        x = x.permute(0, 2, 1)
        return self.cnn(x)

# ======================
# 2. 准备数据
# ======================
class SensorDataset(Dataset):
    def __init__(self, num_samples=1000, seq_len=100, num_sensors=8, num_classes=4):
        self.num_samples = num_samples
        self.seq_len = seq_len
        self.num_sensors = num_sensors
        self.num_classes = num_classes
        # 生成随机数据
        self.data = torch.randn(num_samples, seq_len, num_sensors)
        self.labels = torch.randint(0, num_classes, (num_samples,))
    
    def __len__(self):
        return self.num_samples
    
    def __getitem__(self, idx):
        return self.data[idx], self.labels[idx]

# ======================
# 3. 量化流程
# ======================
def main():
    # 1. 初始化模型
    model = LightweightCNN()
    model.eval()

    # 2. 导出为 ONNX
    dummy_input = torch.randn(1, 100, 8)  # [batch, seq_len, num_sensors]
    onnx_path = "sensor_cnn.onnx"
    
    torch.onnx.export(
        model,
        dummy_input,
        onnx_path,
        export_params=True,
        opset_version=18,
        do_constant_folding=True,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={'input': {0: 'batch_size'}, 'output': {0: 'batch_size'}}
    )
    print(f"ONNX模型已导出至: {onnx_path}")

    # 3. 准备校准数据
    dataset = SensorDataset(num_samples=100)
    dataloader = DataLoader(dataset, batch_size=1, shuffle=False)
    
    def collate_fn(batch):
        if isinstance(batch, (list, tuple)) and len(batch) == 2:
            data, _ = batch
            return {'input': data}
        elif isinstance(batch, torch.Tensor):
            return {'input': batch}
        else:
            return {'input': torch.tensor(batch)}

    # 4. 配置量化
    quant_setting = QuantizationSettingFactory.espdl_setting()
    quant_setting.quantize_activation = True
    quant_setting.quantize_parameter = True

    # 5. 执行量化 (使用正确的 API)
    quantized_model = espdl_quantize_onnx(
        onnx_import_file=onnx_path,
        espdl_export_file='./model/sensor_cnn.espdl',
        calib_dataloader=dataloader,
        calib_steps=100,
        input_shape=[1, 100, 8],
        target='esp32s3',
        collate_fn=collate_fn,
        setting=quant_setting,
        device='cuda' if torch.cuda.is_available() else 'cpu',
    )
    
    print("量化完成！.espdl文件已生成在 ./model 目录下")

if __name__ == "__main__":
    main()