# Sonaria

## 硬件拓扑

```text
    INMP441                 ESP32-S3-DevKitC-1               SSD1351 OLED
┌────────────┐            ┌──────────────────┐           ┌────────────┐
│        VDD ├────────────┤ 3V3          5V  ├───────────┤ VCC        │
│        GND ├────────────┤ GND          GND ├───────────┤ GND        │
│        L/R ├──────┬─────┤                  │           │            │
│        SCK ├──────┼─────┤ GPIO4            │           │            │
│         WS ├──────┼─────┤ GPIO5            │           │            │
│         SD ├──────┼─────┤ GPIO6            │           │            │
│            │      │     │                  │           │            │
│            │      │     │ GPIO11           ├───────────┤ DIN        │
│            │      │     │ GPIO12           ├───────────┤ CLK        │
│            │      │     │ GPIO10           ├───────────┤ CS         │
│            │      │     │ GPIO9            ├───────────┤ DC         │
│            │      │     │ GPIO8            ├───────────┤ RST        │
└────────────┘      └─────┤ GND (L/R接此处)  │           └────────────┘
                          └────────┬─────────┘
                                   │
                                  USB
                                   │
                                   PC
```

## 工程结构

```text
main/                       # 固件入口与测试模式选择
components/
  audio_input/              # INMP441 I2S 音频采集
  pitch_analysis/           # YIN、音符转换、结果平滑（纯 C）
  oled_display/             # SSD1351 驱动与基础绘图
  tuner_display/            # 调音器界面与显示任务

tests/                      # PC 自动化测试与测试音频生成
docs/                       # 实现方案、接线及测试记录
```

在项目根目录构建完整调音器并烧录：

```sh
idf.py -DSONARIA_TEST=tuner reconfigure build
idf.py -p /dev/ttyUSB0 flash monitor
```
