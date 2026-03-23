import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import numpy as np
import matplotlib.pyplot as plt

# 设置matplotlib中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'Arial Unicode MS', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题

# ==========================================
# 1. 模拟测控场景数据：多传感器时序故障数据
# ==========================================
class SensorDataset(Dataset):
    def __init__(self, num_samples=1000, seq_len=100, num_sensors=8, num_classes=4):
        """
        模拟多传感器时序数据（改进版）
        :param num_samples: 样本数量
        :param seq_len: 时序长度（每个样本的时间步数）
        :param num_sensors: 传感器数量（空间维度）
        :param num_classes: 故障类型数量（含正常）
        """
        self.num_samples = num_samples
        self.seq_len = seq_len
        self.num_sensors = num_sensors
        self.num_classes = num_classes
        
        # 生成数据
        self.data, self.labels = self._generate_data()
    
    def _generate_data(self):
        data = []
        labels = []
        
        for _ in range(self.num_samples):
            label = np.random.randint(0, self.num_classes)
            t = np.linspace(0, 10, self.seq_len)
            signal = np.zeros((self.seq_len, self.num_sensors))
            
            # 1. 生成基础正常信号（带相位差）
            for i in range(self.num_sensors):
                phase_shift = i * 0.5
                base_signal = np.sin(2 * np.pi * t + phase_shift)
                signal[:, i] = base_signal
            
            # 2. 根据标签添加噪声
            if label == 0:
                # 正常：只添加小噪声
                noise = 0.1 * np.random.randn(self.seq_len, self.num_sensors)
                signal = signal + noise
            else:
                # 故障：正常信号 + 故障噪声
                fault_noise = self._generate_fault_noise(t, label, self.seq_len, self.num_sensors)
                signal = signal + fault_noise
            
            data.append(signal)
            labels.append(label)
        
        return torch.tensor(np.array(data), dtype=torch.float32), torch.tensor(np.array(labels), dtype=torch.long)
    
    def _generate_fault_noise(self, t, fault_type, seq_len, num_sensors):
        """
        生成故障噪声
        :param t: 时间轴
        :param fault_type: 故障类型 (1, 2, 3)
        :param seq_len: 序列长度
        :param num_sensors: 传感器数量
        """
        fault_noise = np.zeros((seq_len, num_sensors))
        
        if fault_type == 1:
            # 故障类型1：周期性冲击
            for i in range(num_sensors):
                impulse_freq = 2.0
                impulse = np.sin(2 * np.pi * impulse_freq * t) * np.exp(-0.5 * t)
                fault_noise[:, i] = 0.8 * impulse + 0.3 * np.random.randn(seq_len)
        
        elif fault_type == 2:
            # 故障类型2：突发冲击（随机位置）
            for i in range(num_sensors):
                impulse_pos = np.random.randint(20, 60)
                impulse = np.zeros(seq_len)
                impulse[impulse_pos:impulse_pos+10] = np.exp(-np.arange(10)/3)
                fault_noise[:, i] = 1.2 * impulse + 0.25 * np.random.randn(seq_len)
        
        elif fault_type == 3:
            # 故障类型3：高频振动
            for i in range(num_sensors):
                high_freq = 8.0
                high_freq_noise = np.sin(2 * np.pi * high_freq * t) * np.exp(-0.2 * t)
                fault_noise[:, i] = 0.6 * high_freq_noise + 0.2 * np.random.randn(seq_len)
        
        return fault_noise
    
    def __len__(self):
        return self.num_samples
    
    def __getitem__(self, idx):
        return self.data[idx], self.labels[idx]

# ==========================================
# 数据可视化导出功能
# ==========================================
def visualize_and_export_data(num_samples_per_class=3, seq_len=100, num_sensors=8, num_classes=4, save_path='传感器数据可视化.png'):
    """
    可视化并导出正常数据和故障数据（改进版）
    :param num_samples_per_class: 每类显示的样本数
    :param seq_len: 时序长度
    :param num_sensors: 传感器数量
    :param num_classes: 类别数量
    :param save_path: 保存图片的路径
    """
    # 类别名称映射
    class_names = {
        0: '正常 (Normal)',
        1: '故障类型1 (周期性冲击)',
        2: '故障类型2 (突发冲击)',
        3: '故障类型3 (高频振动)'
    }
    
    # 生成数据 - 确保每类都有足够样本
    class_data = {i: [] for i in range(num_classes)}
    
    # 为每类单独生成指定数量的样本
    for class_idx in range(num_classes):
        for _ in range(num_samples_per_class):
            label = class_idx
            t = np.linspace(0, 10, seq_len)
            signal = np.zeros((seq_len, num_sensors))
            
            # 1. 生成基础正常信号（带相位差）
            for sensor_idx in range(num_sensors):
                phase_shift = sensor_idx * 0.5
                base_signal = np.sin(2 * np.pi * t + phase_shift)
                signal[:, sensor_idx] = base_signal
            
            # 2. 根据标签添加噪声
            if label == 0:
                # 正常：只添加小噪声
                noise = 0.1 * np.random.randn(seq_len, num_sensors)
                signal = signal + noise
            else:
                # 故障：正常信号 + 故障噪声
                fault_noise = SensorDataset._generate_fault_noise(None, t, label, seq_len, num_sensors)
                signal = signal + fault_noise
            
            class_data[class_idx].append(signal)
    
    # 创建子图
    fig, axes = plt.subplots(num_classes, num_samples_per_class, figsize=(20, 12))
    fig.suptitle('传感器时序数据可视化 - 正常 vs 故障数据对比（改进版）', fontsize=16, fontweight='bold')
    
    t = np.linspace(0, 10, seq_len)
    colors = plt.cm.tab10(np.linspace(0, 1, num_sensors))
    
    for class_idx in range(num_classes):
        for sample_idx in range(min(num_samples_per_class, len(class_data[class_idx]))):
            ax = axes[class_idx, sample_idx] if num_classes > 1 else axes[sample_idx]
            data = class_data[class_idx][sample_idx]
            
            # 绘制每个传感器的信号
            for sensor_idx in range(num_sensors):
                ax.plot(t, data[:, sensor_idx], color=colors[sensor_idx], alpha=0.7, linewidth=1, label=f'传感器{sensor_idx+1}')
            
            ax.set_title(f'{class_names[class_idx]} - 样本{sample_idx+1}', fontsize=10)
            ax.set_xlabel('时间 (s)', fontsize=8)
            ax.set_ylabel('信号幅值', fontsize=8)
            ax.grid(True, alpha=0.3)
            
            # 只在第一行显示图例
            if class_idx == 0 and sample_idx == 0:
                ax.legend(loc='upper right', fontsize=7, ncol=2)
    
    plt.tight_layout()
    plt.savefig(save_path, dpi=300, bbox_inches='tight')
    print(f"数据可视化已保存至: {save_path}")
    plt.show()
    
    # 额外创建一个对比图：正常 vs 各类故障的平均波形
    fig2, axes2 = plt.subplots(2, 2, figsize=(14, 10))
    fig2.suptitle('正常数据与故障数据平均波形对比（改进版）', fontsize=14, fontweight='bold')
    axes2 = axes2.flatten()
    
    for class_idx in range(num_classes):
        ax = axes2[class_idx]
        
        # 计算平均波形
        if len(class_data[class_idx]) > 0:
            avg_data = np.mean(class_data[class_idx], axis=0)
            
            # 显示所有传感器
            for sensor_idx in range(num_sensors):
                ax.plot(t, avg_data[:, sensor_idx], alpha=0.8, linewidth=1.5, label=f'传感器{sensor_idx+1}')
            
            ax.set_title(class_names[class_idx], fontsize=11)
            ax.set_xlabel('时间 (s)', fontsize=9)
            ax.set_ylabel('平均信号幅值', fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.legend(loc='upper right', fontsize=6, ncol=2)
    
    plt.tight_layout()
    comparison_path = save_path.replace('.png', '对比图.png')
    plt.savefig(comparison_path, dpi=300, bbox_inches='tight')
    print(f"对比图已保存至: {comparison_path}")
    plt.show()
    
    # 导出数据到CSV
    export_data_to_csv(class_data, seq_len, num_sensors, num_classes)
    
    return class_data

def export_data_to_csv(class_data, seq_len, num_sensors, num_classes, csv_path='传感器数据导出.csv'):
    """
    将数据导出为CSV格式
    """
    import csv
    
    with open(csv_path, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        
        # 写入表头
        header = ['类别', '样本编号'] + [f'传感器{i+1}_时刻{t}' for t in range(seq_len) for i in range(num_sensors)]
        writer.writerow(header)
        
        # 写入数据
        for class_idx in range(num_classes):
            class_name = f'类别{class_idx}'
            for sample_idx, data in enumerate(class_data[class_idx]):
                row = [class_name, f'样本{sample_idx+1}']
                # 将每个传感器在每个时刻的数据展开为一行
                for t in range(seq_len):
                    for sensor_idx in range(num_sensors):
                        row.append(data[t, sensor_idx])
                writer.writerow(row)
    
    print(f"数据已导出至CSV: {csv_path}")

# ==========================================
# 物理约束与可解释性模块
# ==========================================
class PhysicsConstrainedLoss(nn.Module):
    """
    物理约束损失函数
    基于测控场景的物理规律添加约束
    """
    def __init__(self, num_sensors=8, seq_len=100):
        super(PhysicsConstrainedLoss, self).__init__()
        self.num_sensors = num_sensors
        self.seq_len = seq_len
        
    def forward(self, predictions, features, labels):
        """
        计算物理约束损失
        :param predictions: 模型预测 [batch, num_classes]
        :param features: 中间特征 [batch, seq_len, features]
        :param labels: 真实标签 [batch]
        """
        # 1. 时序平滑性约束 - 相邻时刻特征不应突变
        if features.dim() == 3:
            temporal_diff = torch.diff(features, dim=1)  # [batch, seq_len-1, features]
            smoothness_loss = torch.mean(torch.square(temporal_diff))
        else:
            smoothness_loss = torch.tensor(0.0, device=predictions.device)
        
        # 2. 预测置信度约束 - 预测应该有明确的置信度
        probs = torch.softmax(predictions, dim=1)
        entropy = -torch.sum(probs * torch.log(probs + 1e-8), dim=1)
        confidence_loss = torch.mean(entropy)  # 希望熵小，即预测更确定
        
        # 3. 类别一致性约束 - 同类样本的特征应该相似
        consistency_loss = torch.tensor(0.0, device=predictions.device)
        if features.dim() == 3 and len(labels) > 1:
            unique_labels = torch.unique(labels)
            for label in unique_labels:
                mask = (labels == label)
                if mask.sum() > 1:
                    class_features = features[mask]
                    mean_feature = torch.mean(class_features, dim=0, keepdim=True)
                    var = torch.mean(torch.square(class_features - mean_feature))
                    consistency_loss += var
        
        # 4. 能量守恒约束 - 特征的总能量应该合理
        if features.dim() == 3:
            energy = torch.mean(torch.square(features), dim=[1, 2])
            energy_loss = torch.var(energy)  # 希望能量分布均匀
        else:
            energy_loss = torch.tensor(0.0, device=predictions.device)
        
        # 综合损失（权重可调）
        total_loss = (
            0.1 * smoothness_loss + 
            0.05 * confidence_loss + 
            0.1 * consistency_loss + 
            0.05 * energy_loss
        )
        
        return total_loss, {
            'smoothness': smoothness_loss.item(),
            'confidence': confidence_loss.item(),
            'consistency': consistency_loss.item(),
            'energy': energy_loss.item()
        }

class AttentionVisualizer:
    """
    注意力权重可视化器
    用于解释模型关注的时序位置
    """
    def __init__(self, model, seq_len=100):
        self.model = model
        self.seq_len = seq_len
        self.attention_weights = None
        self.hooks = []
        
        # 只有在模型不为None时才注册钩子
        if self.model is not None:
            self._register_hooks()
    
    def _register_hooks(self):
        def hook(module, input, output):
            # 从自定义的CustomMultiheadAttention获取注意力权重
            if hasattr(module, 'attention_weights') and module.attention_weights is not None:
                self.attention_weights = module.attention_weights
        
        # 尝试获取Transformer的注意力权重
        if self.model is None:
            return
            
        for name, module in self.model.named_modules():
            # 查找自定义的多头注意力层
            if isinstance(module, CustomMultiheadAttention):
                self.hooks.append(module.register_forward_hook(hook))
    
    def visualize_attention(self, sample_idx=0, save_path='注意力权重分析.png'):
        """
        可视化注意力权重
        注意：PyTorch的Transformer默认不返回注意力权重
        这里显示的是Transformer的输出特征作为替代
        """
        if self.attention_weights is None:
            print("未获取到注意力权重")
            print("提示：PyTorch的nn.Transformer默认不返回注意力权重")
            print("如需获取注意力权重，需要自定义Transformer层")
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('模型注意力权重分析', fontsize=14, fontweight='bold')
        
        # 假设注意力权重形状为 [num_heads, seq_len, seq_len]
        if self.attention_weights.dim() == 3:
            num_heads = self.attention_weights.shape[0]
            
            # 1. 平均注意力图
            ax1 = axes[0, 0]
            avg_attention = torch.mean(self.attention_weights, dim=0).numpy()
            im1 = ax1.imshow(avg_attention, cmap='hot', aspect='auto')
            ax1.set_title('平均注意力权重')
            ax1.set_xlabel('查询位置')
            ax1.set_ylabel('键位置')
            plt.colorbar(im1, ax=ax1)
            
            # 2. 各注意力头的分布
            ax2 = axes[0, 1]
            for head_idx in range(min(num_heads, 4)):
                head_attention = self.attention_weights[head_idx].numpy()
                ax2.plot(head_attention.mean(axis=1), label=f'头{head_idx+1}')
            ax2.set_title('各注意力头的时序关注分布')
            ax2.set_xlabel('时序位置')
            ax2.set_ylabel('平均注意力')
            ax2.legend()
            ax2.grid(True, alpha=0.3)
            
            # 3. 注意力热力图（前4个头）
            ax3 = axes[1, 0]
            attention_subset = self.attention_weights[:4].numpy()
            im3 = ax3.imshow(attention_subset.reshape(-1, self.seq_len), 
                            cmap='hot', aspect='auto')
            ax3.set_title('注意力权重热力图（前4个头）')
            ax3.set_xlabel('时序位置')
            ax3.set_ylabel('头×查询位置')
            plt.colorbar(im3, ax=ax3)
            
            # 4. 注意力统计
            ax4 = axes[1, 1]
            attention_stats = {
                '均值': float(torch.mean(self.attention_weights)),
                '标准差': float(torch.std(self.attention_weights)),
                '最大值': float(torch.max(self.attention_weights)),
                '最小值': float(torch.min(self.attention_weights))
            }
            ax4.axis('off')
            ax4.text(0.1, 0.8, '注意力权重统计', fontsize=12, fontweight='bold')
            for i, (key, value) in enumerate(attention_stats.items()):
                ax4.text(0.1, 0.6 - i*0.15, f'{key}: {value:.4f}', fontsize=10)
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"注意力权重可视化已保存至: {save_path}")
        plt.show()
    
    def remove_hooks(self):
        for hook in self.hooks:
            try:
                hook.remove()
            except:
                pass
        self.hooks.clear()

class FeatureImportanceAnalyzer:
    """
    特征重要性分析器
    分析哪些传感器/特征对预测最重要
    """
    def __init__(self, model, num_sensors=8):
        self.model = model
        self.num_sensors = num_sensors
        self.feature_importance = None
    
    def compute_importance(self, inputs, labels):
        """
        计算特征重要性（基于梯度）
        """
        inputs.requires_grad_(True)
        
        outputs = self.model(inputs)
        loss = outputs.gather(1, labels.unsqueeze(1)).sum()
        
        loss.backward()
        
        # 计算每个传感器的重要性
        if inputs.dim() == 3:  # [batch, seq_len, num_sensors]
            grad = inputs.grad.abs().mean(dim=[0, 1])  # [num_sensors]
        else:
            grad = inputs.grad.abs().mean(dim=0)
        
        self.feature_importance = grad.detach().cpu().numpy()
        
        inputs.requires_grad_(False)
        
        return self.feature_importance
    
    def visualize_importance(self, save_path='特征重要性分析.png'):
        """
        可视化特征重要性
        """
        if self.feature_importance is None:
            print("请先计算特征重要性")
            return
        
        fig, axes = plt.subplots(1, 2, figsize=(14, 5))
        fig.suptitle('特征重要性分析', fontsize=14, fontweight='bold')
        
        # 1. 传感器重要性柱状图
        ax1 = axes[0]
        sensor_names = [f'传感器{i+1}' for i in range(self.num_sensors)]
        colors = plt.cm.viridis(np.linspace(0, 1, self.num_sensors))
        bars = ax1.bar(sensor_names, self.feature_importance, color=colors, alpha=0.7)
        
        ax1.set_xlabel('传感器')
        ax1.set_ylabel('重要性分数')
        ax1.set_title('各传感器对预测的重要性')
        ax1.grid(True, alpha=0.3, axis='y')
        
        # 添加数值标签
        for bar, value in zip(bars, self.feature_importance):
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{value:.3f}', ha='center', va='bottom', fontsize=8)
        
        # 2. 重要性分布饼图
        ax2 = axes[1]
        ax2.pie(self.feature_importance, labels=sensor_names, autopct='%1.1f%%',
                colors=colors, startangle=90)
        ax2.set_title('传感器重要性占比')
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"特征重要性分析已保存至: {save_path}")
        plt.show()
        
        # 打印重要性排序
        sorted_indices = np.argsort(self.feature_importance)[::-1]
        print("\n传感器重要性排序:")
        for i, idx in enumerate(sorted_indices):
            print(f"{i+1}. 传感器{idx+1}: {self.feature_importance[idx]:.4f}")

class PhysicsConsistencyChecker:
    """
    物理一致性检查器
    检查模型输出是否符合物理规律
    """
    def __init__(self, num_sensors=8, seq_len=100):
        self.num_sensors = num_sensors
        self.seq_len = seq_len
    
    def check_signal_properties(self, signals, labels):
        """
        检查信号物理属性
        """
        results = {}
        
        # 1. 检查信号能量
        energy = torch.mean(torch.square(signals), dim=[1, 2])
        results['mean_energy'] = float(torch.mean(energy))
        results['energy_std'] = float(torch.std(energy))
        
        # 2. 检查信号频率特性
        fft_signals = torch.fft.fft(signals, dim=1)
        power_spectrum = torch.abs(fft_signals) ** 2
        dominant_freq = torch.argmax(power_spectrum, dim=1).float()
        results['mean_dominant_freq'] = float(torch.mean(dominant_freq))
        results['freq_std'] = float(torch.std(dominant_freq))
        
        # 3. 检查信号平滑度
        if signals.dim() == 3:
            diff = torch.diff(signals, dim=1)
            smoothness = torch.mean(torch.square(diff))
            results['smoothness'] = float(smoothness)
        
        # 4. 按类别统计
        unique_labels = torch.unique(labels)
        results['by_class'] = {}
        for label in unique_labels:
            mask = (labels == label)
            class_signals = signals[mask]
            class_energy = float(torch.mean(torch.square(class_signals)))
            results['by_class'][f'类别{label.item()}'] = {
                'energy': class_energy,
                'mean_amplitude': float(torch.mean(torch.abs(class_signals)))
            }
        
        return results
    
    def print_report(self, results):
        """
        打印物理一致性报告
        """
        print("\n" + "="*50)
        print("物理一致性检查报告")
        print("="*50)
        print(f"平均信号能量: {results['mean_energy']:.4f}")
        print(f"能量标准差: {results['energy_std']:.4f}")
        print(f"平均主频率: {results['mean_dominant_freq']:.2f}")
        print(f"频率标准差: {results['freq_std']:.2f}")
        if 'smoothness' in results:
            print(f"信号平滑度: {results['smoothness']:.4f}")
        
        print("\n各类别统计:")
        for class_name, stats in results['by_class'].items():
            print(f"{class_name}: 能量={stats['energy']:.4f}, 平均幅值={stats['mean_amplitude']:.4f}")
        print("="*50)

# ==========================================
# 自定义Transformer层（返回注意力权重）
# ==========================================
class CustomMultiheadAttention(nn.MultiheadAttention):
    """
    自定义多头注意力层，返回注意力权重
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.attention_weights = None
    
    def forward(self, query, key, value, key_padding_mask=None, need_weights=True, attn_mask=None, is_causal=False):
        # 调用父类的forward方法
        output, attn_weights = super().forward(
            query, key, value, 
            key_padding_mask=key_padding_mask, 
            need_weights=True,  # 强制返回注意力权重
            attn_mask=attn_mask,
            is_causal=is_causal  # 添加is_causal参数
        )
        
        # 保存注意力权重
        self.attention_weights = attn_weights.detach().cpu()
        
        return output, attn_weights

class CustomTransformerEncoderLayer(nn.TransformerEncoderLayer):
    """
    自定义Transformer编码器层，使用自定义注意力
    """
    def __init__(self, d_model, nhead, dim_feedforward=2048, dropout=0.1, activation="relu", batch_first=False):
        # 替换self_attn为自定义的多头注意力
        super().__init__(d_model, nhead, dim_feedforward, dropout, activation, batch_first)
        self.self_attn = CustomMultiheadAttention(d_model, nhead, dropout=dropout, batch_first=batch_first)

# ==========================================
# 2. 核心模型：CNN-Transformer（替代CNN-LSTM）
# ==========================================
class CNNTransformer(nn.Module):
    def __init__(self, num_sensors=8, seq_len=100, num_classes=4, 
                 cnn_out_channels=32, transformer_d_model=64, 
                 transformer_nhead=2, transformer_num_layers=2):
        """
        CNN提取空间特征 → Transformer建模时序依赖 → 分类
        :param num_sensors: 传感器数量（输入空间维度）
        :param seq_len: 时序长度
        :param num_classes: 故障类型数量
        :param cnn_out_channels: CNN输出通道数
        :param transformer_d_model: Transformer特征维度
        :param transformer_nhead: Transformer注意力头数
        :param transformer_num_layers: Transformer编码器层数
        """
        super(CNNTransformer, self).__init__()
        
        # --------------------------
        # CNN部分：提取多传感器空间特征
        # --------------------------
        self.cnn = nn.Sequential(
            # 输入: [batch, num_sensors, seq_len] (Conv1d要求通道在前)
            nn.Conv1d(in_channels=num_sensors, out_channels=cnn_out_channels, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.MaxPool1d(kernel_size=2, stride=2),  # 时序长度减半
            nn.Conv1d(in_channels=cnn_out_channels, out_channels=cnn_out_channels, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.MaxPool1d(kernel_size=2, stride=2)   # 时序长度再减半
        )
        
        # 计算CNN输出后的时序长度
        self.cnn_seq_len = seq_len // 2 // 2
        
        # --------------------------
        # 特征投影：将CNN输出映射到Transformer输入维度
        # --------------------------
        self.feature_proj = nn.Linear(cnn_out_channels, transformer_d_model)
        
        # --------------------------
        # Transformer Encoder：建模长时序依赖
        # --------------------------
        # 使用自定义Transformer层以获取注意力权重
        transformer_encoder_layer = CustomTransformerEncoderLayer(
            d_model=transformer_d_model,
            nhead=transformer_nhead,
            dim_feedforward=transformer_d_model * 2,
            dropout=0.1,
            batch_first=True  # 使用batch_first以获得更好的性能
        )
        self.transformer_encoder = nn.TransformerEncoder(
            transformer_encoder_layer,
            num_layers=transformer_num_layers
        )
        
        # --------------------------
        # 分类头：故障诊断输出
        # --------------------------
        self.classifier = nn.Sequential(
            nn.Linear(transformer_d_model, transformer_d_model // 2),
            nn.ReLU(),
            nn.Linear(transformer_d_model // 2, num_classes)
        )
    
    def forward(self, x):
        # x输入: [batch, seq_len, num_sensors]
        
        # --------------------------
        # 1. CNN空间特征提取
        # --------------------------
        # 调整维度为Conv1d要求的 [batch, num_sensors, seq_len]
        x = x.permute(0, 2, 1)
        # CNN前向传播
        x = self.cnn(x)  # 输出: [batch, cnn_out_channels, cnn_seq_len]
        
        # --------------------------
        # 2. 特征投影与维度调整
        # --------------------------
        # 调整维度为 [batch, cnn_seq_len, cnn_out_channels]
        x = x.permute(0, 2, 1)
        # 投影到Transformer输入维度
        x = self.feature_proj(x)  # 输出: [batch, cnn_seq_len, transformer_d_model]
        # 由于batch_first=True，Transformer输入为 [batch, seq_len, d_model]
        # 不需要permute
        
        # --------------------------
        # 3. Transformer时序建模
        # --------------------------
        x = self.transformer_encoder(x)  # 输出: [batch, cnn_seq_len, transformer_d_model]
        
        # --------------------------
        # 4. 分类输出
        # --------------------------
        # 取最后一个时刻的特征进行分类
        x = x[:, -1, :]  # 输出: [batch, transformer_d_model]
        # 分类头前向传播
        logits = self.classifier(x)  # 输出: [batch, num_classes]
        
        return logits

# ==========================================
# 3. 训练与推理流程（带物理约束）
# ==========================================
def train():
    # 超参数设置（符合测控场景的轻量配置）
    batch_size = 32
    seq_len = 100
    num_sensors = 8
    num_classes = 4
    epochs = 20
    lr = 1e-3
    
    # 设备配置（优先GPU，无GPU则CPU）
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"使用设备: {device}")
    
    # 初始化物理约束和可解释性工具
    physics_loss_fn = PhysicsConstrainedLoss(num_sensors=num_sensors, seq_len=seq_len).to(device)
    attention_viz = AttentionVisualizer(None, seq_len=seq_len)
    feature_analyzer = FeatureImportanceAnalyzer(None, num_sensors=num_sensors)
    physics_checker = PhysicsConsistencyChecker(num_sensors=num_sensors, seq_len=seq_len)
    
    # 数据加载
    dataset = SensorDataset(num_samples=1000, seq_len=seq_len, num_sensors=num_sensors, num_classes=num_classes)
    dataloader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
    
    # 模型初始化
    model = CNNTransformer(
        num_sensors=num_sensors,
        seq_len=seq_len,
        num_classes=num_classes,
        cnn_out_channels=32,
        transformer_d_model=64,
        transformer_nhead=2,
        transformer_num_layers=2
    ).to(device)
    
    # 更新可解释性工具的模型引用并重新注册钩子
    attention_viz.model = model
    attention_viz._register_hooks()  # 重新注册钩子
    feature_analyzer.model = model
    
    # 损失函数与优化器
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=lr)
    
    # 物理约束损失历史
    physics_loss_history = {
        'smoothness': [],
        'confidence': [],
        'consistency': [],
        'energy': []
    }
    
    # 训练循环 - 无动画，快速训练
    model.train()
    train_losses = []
    train_accs = []
    
    # 用于存储中间特征（用于物理约束）
    transformer_features = None
    hook_call_count = 0  # 记录钩子调用次数
    
    # 注册钩子函数获取中间特征
    def hook_transformer(module, input, output):
        nonlocal transformer_features, hook_call_count
        transformer_features = output.detach()  # 保持设备一致
        hook_call_count += 1
        if hook_call_count == 1:  # 只在第一次调用时打印
            print(f"    [钩子] 成功捕获Transformer输出，形状: {output.shape}")
    
    # 注册钩子
    handle_transformer = model.transformer_encoder.register_forward_hook(hook_transformer)
    print(f"已注册Transformer钩子到: {model.transformer_encoder}")
    
    for epoch in range(epochs):
        total_loss = 0.0
        correct = 0
        total = 0
        
        for batch_idx, (inputs, labels) in enumerate(dataloader):
            inputs, labels = inputs.to(device), labels.to(device)
            
            # 前向传播
            optimizer.zero_grad()
            outputs = model(inputs)
            
            # 计算物理约束损失
            if transformer_features is not None:
                # 由于batch_first=True，特征已经是[batch, seq_len, d_model]格式
                features_for_physics = transformer_features  # [batch, seq_len, d_model]
                physics_loss, physics_details = physics_loss_fn(outputs, features_for_physics, labels)
                
                # 记录物理约束损失
                for key, value in physics_details.items():
                    physics_loss_history[key].append(value)
                
                # 每10个batch打印一次物理约束信息
                if batch_idx % 10 == 0:
                    print(f"    物理约束损失: {physics_loss.item():.6f} (平滑:{physics_details['smoothness']:.4f}, "
                          f"置信:{physics_details['confidence']:.4f}, 一致:{physics_details['consistency']:.4f}, "
                          f"能量:{physics_details['energy']:.4f})")
            else:
                physics_loss = torch.tensor(0.0, device=device)
                if batch_idx % 10 == 0:
                    print(f"    警告：未获取到Transformer特征，物理约束损失为0 (钩子调用次数: {hook_call_count})")
            
            # 总损失 = 分类损失 + 物理约束损失
            classification_loss = criterion(outputs, labels)
            total_loss_batch = classification_loss + 0.5 * physics_loss  # 物理约束权重0.5（增加权重）
            
            # 反向传播与优化
            total_loss_batch.backward()
            optimizer.step()
            
            # 统计
            batch_loss = total_loss_batch.item()
            total_loss += batch_loss * inputs.size(0)
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()
        
        # 计算epoch指标
        epoch_loss = total_loss / total
        epoch_acc = correct / total
        train_losses.append(epoch_loss)
        train_accs.append(epoch_acc)
        
        print(f"轮次 [{epoch+1}/{epochs}], 损失: {epoch_loss:.4f}, 准确率: {epoch_acc:.4f}")
    
    # 移除钩子
    handle_transformer.remove()
    
    # 训练完成后绘制训练曲线
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle('训练过程总结', fontsize=14, fontweight='bold')
    
    # 损失曲线
    ax1.plot(train_losses, 'b-', linewidth=2, label='损失')
    ax1.set_xlabel('轮次')
    ax1.set_ylabel('损失')
    ax1.set_title('训练损失变化')
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    
    # 准确率曲线
    ax2.plot(train_accs, 'orange', linewidth=2, label='准确率')
    ax2.set_xlabel('轮次')
    ax2.set_ylabel('准确率')
    ax2.set_title('训练准确率变化')
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    
    plt.tight_layout()
    plt.savefig('训练总结.png', dpi=300, bbox_inches='tight')
    print("训练总结已保存至: 训练总结.png")
    plt.show()
    
    # 训练后的可解释性分析
    print("\n" + "="*50)
    print("开始可解释性分析...")
    print("="*50)
    
    # 1. 注意力权重可视化
    print("\n1. 分析模型注意力权重...")
    attention_viz.visualize_attention(save_path='注意力权重分析.png')
    
    # 2. 特征重要性分析
    print("\n2. 分析特征重要性...")
    test_inputs, test_labels = next(iter(dataloader))
    test_inputs, test_labels = test_inputs.to(device), test_labels.to(device)
    importance = feature_analyzer.compute_importance(test_inputs, test_labels)
    feature_analyzer.visualize_importance(save_path='特征重要性分析.png')
    
    # 3. 物理一致性检查
    print("\n3. 检查物理一致性...")
    test_dataset = SensorDataset(num_samples=100, seq_len=seq_len, num_sensors=num_sensors, num_classes=num_classes)
    test_dataloader = DataLoader(test_dataset, batch_size=32, shuffle=False)
    all_signals = []
    all_labels = []
    for inputs, labels in test_dataloader:
        all_signals.append(inputs)
        all_labels.append(labels)
    all_signals = torch.cat(all_signals, dim=0)
    all_labels = torch.cat(all_labels, dim=0)
    physics_results = physics_checker.check_signal_properties(all_signals, all_labels)
    physics_checker.print_report(physics_results)
    
    # 4. 物理约束损失趋势
    print("\n4. 物理约束损失趋势...")
    
    # 检查数据是否有效
    has_valid_data = False
    for key in physics_loss_history:
        if len(physics_loss_history[key]) > 0 and any(x > 0 for x in physics_loss_history[key]):
            has_valid_data = True
            break
    
    if not has_valid_data:
        print("警告：物理约束损失数据为空或全为零，跳过可视化")
    else:
        fig2, axes2 = plt.subplots(2, 2, figsize=(14, 10))
        fig2.suptitle('物理约束损失变化趋势', fontsize=14, fontweight='bold')
        
        # 平滑度损失
        if len(physics_loss_history['smoothness']) > 0:
            axes2[0, 0].plot(physics_loss_history['smoothness'], 'b-', alpha=0.7)
            axes2[0, 0].set_title(f'时序平滑性约束 (均值: {np.mean(physics_loss_history["smoothness"]):.4f})')
        else:
            axes2[0, 0].text(0.5, 0.5, '无数据', ha='center', va='center', transform=axes2[0, 0].transAxes)
            axes2[0, 0].set_title('时序平滑性约束 (无数据)')
        axes2[0, 0].set_xlabel('Batch')
        axes2[0, 0].set_ylabel('损失')
        axes2[0, 0].grid(True, alpha=0.3)
        
        # 置信度损失
        if len(physics_loss_history['confidence']) > 0:
            axes2[0, 1].plot(physics_loss_history['confidence'], 'orange', alpha=0.7)
            axes2[0, 1].set_title(f'预测置信度约束 (均值: {np.mean(physics_loss_history["confidence"]):.4f})')
        else:
            axes2[0, 1].text(0.5, 0.5, '无数据', ha='center', va='center', transform=axes2[0, 1].transAxes)
            axes2[0, 1].set_title('预测置信度约束 (无数据)')
        axes2[0, 1].set_xlabel('Batch')
        axes2[0, 1].set_ylabel('损失')
        axes2[0, 1].grid(True, alpha=0.3)
        
        # 一致性损失
        if len(physics_loss_history['consistency']) > 0:
            axes2[1, 0].plot(physics_loss_history['consistency'], 'green', alpha=0.7)
            axes2[1, 0].set_title(f'类别一致性约束 (均值: {np.mean(physics_loss_history["consistency"]):.4f})')
        else:
            axes2[1, 0].text(0.5, 0.5, '无数据', ha='center', va='center', transform=axes2[1, 0].transAxes)
            axes2[1, 0].set_title('类别一致性约束 (无数据)')
        axes2[1, 0].set_xlabel('Batch')
        axes2[1, 0].set_ylabel('损失')
        axes2[1, 0].grid(True, alpha=0.3)
        
        # 能量损失
        if len(physics_loss_history['energy']) > 0:
            axes2[1, 1].plot(physics_loss_history['energy'], 'purple', alpha=0.7)
            axes2[1, 1].set_title(f'能量守恒约束 (均值: {np.mean(physics_loss_history["energy"]):.4f})')
        else:
            axes2[1, 1].text(0.5, 0.5, '无数据', ha='center', va='center', transform=axes2[1, 1].transAxes)
            axes2[1, 1].set_title('能量守恒约束 (无数据)')
        axes2[1, 1].set_xlabel('Batch')
        axes2[1, 1].set_ylabel('损失')
        axes2[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig('物理约束损失趋势.png', dpi=300, bbox_inches='tight')
        print("物理约束损失趋势已保存至: 物理约束损失趋势.png")
        plt.show()
    
    # 移除注意力可视化器的钩子
    attention_viz.remove_hooks()
    
    return model

def inference(model, device):
    # 生成测试样本
    test_dataset = SensorDataset(num_samples=10, seq_len=100, num_sensors=8, num_classes=4)
    test_dataloader = DataLoader(test_dataset, batch_size=1, shuffle=False)
    
    model.eval()
    with torch.no_grad():
        for i, (inputs, labels) in enumerate(test_dataloader):
            inputs, labels = inputs.to(device), labels.to(device)
            outputs = model(inputs)
            _, predicted = torch.max(outputs.data, 1)
            print(f"测试样本 {i+1}: 真实标签 = {labels.item()}, 预测标签 = {predicted.item()}")

# ==========================================
# 主程序入口
# ==========================================
if __name__ == "__main__":
    # 第一步：数据可视化与导出
    print("=" * 50)
    print("正在生成数据可视化（改进版）...")
    print("=" * 50)
    visualize_and_export_data(num_samples_per_class=3, seq_len=100, num_sensors=8, num_classes=4)
    
    # 第二步：训练模型
    print("\n" + "=" * 50)
    print("开始训练模型...")
    print("=" * 50)
    trained_model = train()
    
    # 第三步：推理测试
    print("\n" + "=" * 50)
    print("开始推理测试...")
    print("=" * 50)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    inference(trained_model, device)
