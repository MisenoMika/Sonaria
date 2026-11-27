# Sonaria 测试音频与乐谱

## 钢琴单音练习音频（practice 模式）

所有下表 WAV 均为 16 kHz、16 bit、单声道，100 BPM，彻底无伴奏，结尾附 1 秒静音。音色使用本机 GeneralUser GS v1.47 SoundFont 的 Grand Piano 真实采样，以统一参考样本变调、峰值归一化及短起止淡变生成；每个时刻只播放一个音，不叠加前一音的尾音。不是对原 MP3 去伴奏，也不是完整的多采样钢琴模拟。

| 文件 | 配套曲目 | 时长 | 预期错误次数 | 预期完成 |
| --- | --- | ---: | ---: | --- |
| [mary_had_a_little_lamb_monophonic.wav](mary_had_a_little_lamb_monophonic.wav) | Mary | 20.2 s | 0 | 26/26 |
| [twinkle_twinkle_monophonic.wav](twinkle_twinkle_monophonic.wav) | Twinkle | 29.8 s | 0 | 42/42 |
| [mary_had_a_little_lamb_wrong1.wav](mary_had_a_little_lamb_wrong1.wav) | Mary | 20.8 s | 1 | 26/26 |
| [mary_had_a_little_lamb_wrong2.wav](mary_had_a_little_lamb_wrong2.wav) | Mary | 21.4 s | 2 | 26/26 |
| [twinkle_twinkle_wrong1.wav](twinkle_twinkle_wrong1.wav) | Twinkle | 30.4 s | 1 | 42/42 |
| [twinkle_twinkle_wrong2.wav](twinkle_twinkle_wrong2.wav) | Twinkle | 31.0 s | 2 | 42/42 |

错音是**插入**到预期正确音之前的一个四分音符（0.6 秒），随后立即弹回正确音，继续整首曲子。这样适合验证「弹错不推进，弹对才继续」的练习规则；若直接替换并省略正确音，后续会一直错位。

### 故意错误的位置

时间从 WAV 播放起始计算，包含前面已插入的错误音造成的延迟。序号为原谱面（不是包含插入音的实际弹奏序号）。

| 文件 | 错音起始时间 | 原谱位置 | 正确音 | 故意错音 |
| --- | ---: | --- | --- | --- |
| Mary wrong1 | 1.2 s | 第 3/26 音，第 1 小节第 3 拍 | C5 | C#5 |
| Mary wrong2 | 4.8 s | 第 8/26 音，第 3 小节第 1 拍 | D5 | D#5 |
| Mary wrong2 | 12.0 s | 第 17/26 音，第 5 小节第 4 拍 | D5 | C5 |
| Twinkle wrong1 | 1.2 s | 第 3/42 音，第 1 小节第 3 拍 | G4 | F4 |
| Twinkle wrong2 | 2.4 s | 第 5/42 音，第 2 小节第 1 拍 | A4 | G#4 |
| Twinkle wrong2 | 7.2 s | 第 11/42 音，第 3 小节第 4 拍 | E4 | D#4 |

例如播放 Twinkle wrong2，默认非 debug 练习模式预期仅产生以下两条弹奏错误：

```text
ERROR song=twinkle note=5/42 bar=2 beat=1 expected=A4 actual=G#4
ERROR song=twinkle note=11/42 bar=3 beat=4 expected=E4 actual=D#4
```

## 实物测试

烧录 `practice`（`SONARIA_PITCH_MODE=mono`），选择相应曲目并重新开始（每个文件需独立开始）：

```text
select twinkle
start
```

播放 Twinkle 文件；Mary 使用 `select mary`。如果当前还在练习，先 `stop` 再 `select`。观察 OLED 错音时停留原目标、纠正后推进，最终 COMPLETE 与累计错误数。所有六个 WAV 已通过生产版 C 练习引擎数字 PCM 仿真，实际麦克风播放可能受环境和起音影响。

## 重新生成

在项目根目录运行：

```sh
python tests/score_fixtures.py mary assets/mary_had_a_little_lamb_monophonic.wav
python tests/score_fixtures.py twinkle assets/twinkle_twinkle_monophonic.wav
python tests/score_fixtures.py twinkle assets/twinkle_twinkle_wrong1.wav --wrong 3:F4
python tests/score_fixtures.py twinkle assets/twinkle_twinkle_wrong2.wav --wrong '5:G#4' --wrong '11:D#4'
python tests/score_fixtures.py mary assets/mary_had_a_little_lamb_wrong1.wav --wrong '3:C#5'
python tests/score_fixtures.py mary assets/mary_had_a_little_lamb_wrong2.wav --wrong '8:D#5' --wrong 17:C5
```

命令行默认钢琴音色，使用 GeneralUser GS v1.47（本机路径 `/usr/share/minuet/soundfonts/GeneralUser-v1.47.sf2`，无须声卡或启动音频服务）。其他机器可通过 `--soundfont /path/to/GeneralUser.sf2` 或 `SONARIA_SOUNDFONT` 指定文件；未随仓库复制完整 SoundFont。`--timbre harmonic` 可还原旧的谐波合成音色（不依赖音源，原算法合成测试仍使用此模式）。`--wrong INDEX:NOTE` 可重复指定，错误音插入原谱位置前；`--bpm` 可改变速度。

采用统一参考采样是为了建立可重复的钢琴练习基准。试验直接按最近音高切换多套钢琴采样时，部分长音的自然振幅波动会触发现有起音算法的额外事件；本次没有修改固件，真实钢琴复杂起音仍是后续鲁棒性测试点。

## 原始参考资料

- [Mary 乐谱](mary-had-a-little-lamb.png)、[原始 MP3](mary_had_a_little_lamb.mp3)：原录音低于上声部谱面一个八度，且有伴奏。
- [Twinkle 乐谱](twinkle-twinkle-little-star.png)、[原始 MP3](twinkle_twinkle.mp3)：原录音包含钢琴伴奏。

原始 MP3 和乐谱保持不变。原始伴奏音频适合主旋律模式，不推荐直接作为单音乐谱练习的正确基准。
