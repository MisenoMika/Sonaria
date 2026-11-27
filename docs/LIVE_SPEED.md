# 实时单音识别速度

## 参数选择

本次仅调整 `mode live` 的音高跟踪参数；`mode practice` 继续保持原有乐谱判定策略。

| 参数 | 原实时模式 / 练习模式 | 新实时模式 |
|---|---:|---:|
| 采样率 | 16 kHz | 16 kHz |
| 帧长 | 1024 样本（64 ms） | 不变 |
| 帧移 | 512 样本（32 ms） | 256 样本（16 ms） |
| 连续确认帧数 | 4 | 2 |
| 首尾确认帧间隔 | 96 ms | 16 ms |
| 新音置信度 | ≥ 0.90 | 不变 |
| 同音保持置信度 | ≥ 0.85 | 不变 |
| 丢失后保持 | 128 ms | 48 ms |

YIN 在找到第一个满足原判据的局部极小值后结束计算，不再计算无用的后续延迟量；目标端使用 -O2。分析窗口不缩短，以保留低音识别能力。实时模式的常规 debug 日志仍约每 96 ms 输出一次，状态或音名改变时立即输出。

## PC 仿真

连续上行、下行半音序列 C4–C6–C4，共 49 音，无间隔、无尾音重叠；使用 GeneralUser GS 钢琴采样，16 kHz 单声道，加入固定随机种子的白噪声（PCM 振幅 ±300）。

命中定义：音符结束前至少出现一个正确的 CONFIRMED 输出。延迟为音符开始至首次正确确认的样本时间，不含 ESP32 运算、传输和 OLED 刷新。序列正确性还检查所有确认音名的转换是否与整个预期序列一致。

| 每音时长 | 原模式命中 | 16 ms / 3 帧 | 16 ms / 2 帧 | 新模式中位延迟 |
|---:|---:|---:|---:|---:|
| 80 ms | 0/49 | 15/49 | 49/49 | 72 ms |
| 100 ms | 0/49 | 49/49 | 49/49 | 68 ms |
| 120 ms | 0/49 | 49/49 | 49/49 | 64 ms |
| 150 ms | 16/49 | 49/49 | 49/49 | 70 ms |
| 200 ms | 49/49 | 49/49 | 49/49 | 64 ms |

新模式五组序列均完全正确。白噪声振幅增加至 ±2000 时，80 ms 仅 37/49 在音符结束前确认，100 ms 及以上均 49/49，序列完全正确；100 ms 中位延迟 76 ms，最大 96 ms。强噪声 ±4000 的钢琴测试无法确认，说明置信度门槛仍起作用，而不是保证任何环境下都识别。

推荐从每音 100–150 ms（约 7–10 音/秒）开始实机测试。80 ms 是较轻噪声下的测试结果，不作为保证。真实钢琴滑奏可能包含重叠余音，超出严格单声部模型。

## 测试方法

```sh
cmake -S tests -B /tmp/sonaria-host-tests
cmake --build /tmp/sonaria-host-tests
ctest --test-dir /tmp/sonaria-host-tests --output-on-failure
python tests/test_live_speed.py /tmp/sonaria-host-tests/simulate_live --piano --check --wav-dir /tmp/sonaria-live-piano
python tests/test_live_speed.py /tmp/sonaria-host-tests/simulate_live --piano --noise 2000 --check
```

钢琴测试需安装 GeneralUser SoundFont（也可通过 SONARIA_SOUNDFONT 指定路径）；默认不带 `--piano` 的测试使用合成谐波，无需 SoundFont。生成的 WAV 可通过扬声器播放，真实麦克风和 OLED 端需另测。

```sh
idf.py -DSONARIA_TEST=practice -DSONARIA_PITCH_MODE=mono reconfigure build
idf.py flash monitor
```

串口输入 `mode live` 和 `debug on`，观察 `DEBUG pitch` 中的 `compute_us`（应尽量低于 16000）及 `DEBUG display` 的 `queue_us`、`spi_us`。先播放 live_150ms.wav，再播放 live_100ms.wav 和 live_80ms.wav，核对屏幕和串口音名。OLED 仍使用最新结果队列，因此在实际 SPI 刷新过慢时可能跳过中间显示，而不是识别算法漏音。
