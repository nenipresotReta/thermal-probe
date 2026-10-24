# Thermal Probe

[English](README.md) | [简体中文](README.zh-CN.md)

ESP32-S3 + MAX31865 三线 PT100 温度计。提供本地显示、Wi-Fi、10 Hz 采集、平滑温度及一阶／二阶导数，通过 Modbus TCP 接入 [Thermal Scope](https://github.com/nenipresotReta/thermal-scope) 或 Artisan。

**编译、烧写前必须填写 `config/device.json`。** 先复制空白模板：

```sh
cp config/device.example.json config/device.json
```

在本机填写 **2.4 GHz WPA2** 网络名称和密码。SSID 为 1–32 个 UTF-8 字节，密码为 8–63 个 UTF-8 字节，不能包含控制字符。文件缺失、留空或无效时构建会失败，直接使用 CMake 构建也会校验。`flash.sh` 会重新验证并构建，防止更换配置后误刷旧凭据。

**编译后的固件包含 Wi-Fi 凭据。** 不要上传固件、生成头文件、构建目录、备份或真实配置。仓库只分发源码，不提供预编译固件。`.gitignore` 不能删除 Git 历史里已经存在的秘密，也不要强制添加被忽略文件。

## 文档

- [硬件接口](docs/interfaces.zh-CN.md)：引脚方向、供电要求、三线跳线导通关系，采用表格说明。
- [首次启动和故障排查](docs/troubleshooting.zh-CN.md)：验收方法、诊断顺序和脱敏反馈。
- [参与开发与公开前检查](CONTRIBUTING.zh-CN.md)。

## 硬件

已测试目标为 T-Display-S3 兼容 ESP32-S3 板，16 MB Flash，170 × 320 ST7789 屏幕（HD19006C30 替换屏）。使用自定义板定义，不能仅凭产品名认定所有变体都兼容。GPIO15 会在启动时输出高电平，作为此板的电源使能；不包含内置键盘代码。

| ESP32 | MAX31865 模块 |
|---|---|
| 3V3 | 模块规定的电源输入；只有 VIN 确认支持 3.3 V 工作时才接 VIN |
| GND | GND |
| GPIO12 | CLK |
| GPIO11 | SDI / MOSI |
| GPIO13 | SDO / MISO |
| GPIO10 | CS |
| 不接 | RDY；已测试模块的 3V3 输出 |

参考电阻为 **430 Ω**。三线 PT100 同一端的两根线分别接 F+、RTD+，另一端单线接 RTD−；闭合 `2/3 Wire`，保持 `2 Wire` 断开；三焊盘选择器中间接 `3`，中间与 `24` 断开。模块可能不同，断电并拆下探头后测量导通，不凭照片判断。探头不能同时连接原变送器和 MAX31865。固件使用三线补偿、50 Hz 抑制。

## 本地显示

横屏采用 GitHub Dark 风格，显示温度、**D1：°C/min，D2：°C/min²**、设备地址和温度曲线。屏幕字体标为 `C/MIN`、`C/MIN2`。显示时 D1 乘 60、D2 乘 3600；HTTP、Modbus 和采集仍采用秒单位。

- 短按 GPIO14 暂停／继续本地曲线，数值继续刷新。
- 按住至少 2 秒后松开，清除本地曲线和计时。
- RAM 保存 600 点，每 3 秒一条，约 30 分钟；重启清空。
- 温度轴默认 0–250 °C，遇超范围数据自动扩展。
- LCD 约 2 Hz 刷新，采集仍为 10 Hz。

LCD 使用 8 位 8080 总线：D0–D7 为 GPIO39/40/41/42/45/46/47/48；RESET=5、CS=6、DC=7、WR=8、RD=9、背光=38。按四行一组渲染以限制 RAM 使用。PSRAM 关闭，增加缓冲区／线程时要检查链接内存报告；静态占用中包含堆预留，不能直接当作运行时剩余空间。

应用文本格式化统一使用 `probe_printf` / `probe_snprintf` 互斥包装，防止此 SDK 的 newlib 浮点转换共享状态发生重入。LCD 标签每帧只计算一次。控制台使用 256 字节局部缓冲区，输出在解锁后进行，HTTP 发送同样不持有格式化锁。新增日志应少于 256 字节。

## 构建与烧写

需要 Python、CMake、Ninja、west、Zephyr **4.1.0**、Zephyr SDK **0.17.0** 的 ESP32-S3 Xtensa 工具链，以及操作系统对应的 Zephyr 主机依赖。从名为 `thermal-probe` 的源码目录的父目录执行：

```sh
# From the parent of a checkout named thermal-probe:
python3 -m venv .venv
. .venv/bin/activate
pip install west
west init -l thermal-probe
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
west blobs fetch hal_espressif
west sdk install --version 0.17.0 --toolchains xtensa-espressif_esp32s3_zephyr-elf --no-hosttools
cd thermal-probe
cp config/device.example.json config/device.json
# Fill config/device.json locally, then:
sh build.sh
sh flash.sh
```

SDK 安装工具会获取所选工具链；如使用已有 SDK，可按 Zephyr 说明设置 `ZEPHYR_SDK_INSTALL_DIR`。不要把源码放在不能完整读取依赖文件的目录中。

- 更换 SDK／板配置后用 `sh build.sh -p always` 完整重建。
- 同一板子的无屏版本：`sh build.sh -p always -- -DEXTRA_CONF_FILE=headless.conf`。
- 恢复有屏版本：`sh build.sh -p always -- -DEXTRA_CONF_FILE=""`。
- 选定构建配置后执行 `sh flash.sh`；它保留构建目录中的配置并重新构建。

无屏版本移除 LCD 驱动、UI 线程和本地历史，GPIO15 电源使能仍保留。它不代表支持任意 ESP32 板。

输出为 `build/zephyr/zephyr.bin`。使用 ESP simple boot，完整镜像从偏移 0 开始；优先使用配置好的 west 烧写工具。自动下载失败时，按住 BOOT，短按 RESET，再松开 BOOT 后烧写。不同板子的按键标识可能不同。

## 数据与网络

DHCP 分配地址，可从 LCD、USB 日志或路由器客户端列表查看。板子与服务器必须能够互相访问。仓库不包含真实固定地址、SSID 或 MAC。接口没有认证／TLS，适用于可信局域网，不应直接暴露互联网。

每 100 ms 采样一次，最近 101 个连续有效样本（约 10 秒）按实际时间做二次拟合，在最新时刻计算平滑温度和导数。平滑会影响瞬态响应，小数位数不等于测量精度。约 10 秒后导数可用，故障会重置窗口。数据单位为 °C、°C/s、°C/s²。

HTTP：`GET http://DEVICE_IP/api/temperature`，端口 80。字段为 `valid`、`derivatives_ready`、`time_s`、`raw_c`、`temperature_c`、`d1_c_per_s`、`d2_c_per_s2`。无效数值返回 null；`time_s` 是开机时长，不是日历时间。

Modbus TCP：端口 **502**，unit **1**，功能码 **3/4**，寄存器地址从 0 开始，**大端 Float32**，每值两个寄存器。

| 起始寄存器 | 数值 |
|---:|---|
| 0 | 原始温度 |
| 2 | 平滑温度 |
| 4 | D1 |
| 6 | D2 |

数据无效、超过 2 秒未更新或所请求导数未就绪时，返回异常码 4。支持 4 个 Modbus 客户端和 2 个 HTTP 客户端，空闲／I/O 超时 3 秒。HTTP 请求头支持跨 TCP 包，最大 512 字节，须在 3 秒内收齐。关闭 Wi-Fi 省电以提高响应，电池耗电会增加。不包含加热控制、云上传或持久化记录。

## Artisan

主设备选 MODBUS，类型 **TCP**，填写 DHCP 地址，端口 502。**BT 对应 Input 1**：unit 1、功能码 3、寄存器 2、Float32、°C、不缩放、大端字节／字序（取消 little-endian words）。未使用的 ET/Input 2 将 device ID 设为 0。可用 MODBUS_34 附加输入读取寄存器 4/6，作为导数数值，不作温度转换。Artisan 自己的 ΔBT 是另外计算的。

先用 1 秒轮询、0.3–1 秒超时，再按局域网情况调整；重试会延长失败轮询耗时。客户端频率不改变板子 10 Hz 采集。在 Thermal Scope 用 DEVICE → DISCOVER，或手动填写地址。

## 测试与源码发布

```sh
python3 -m unittest discover -s tests -p 'test_*.py'
cc -Wall -Wextra -Werror -Isrc src/filter.c tests/filter_test.c -lm -o /tmp/filter-test
/tmp/filter-test
cc -Wall -Wextra -Werror -Isrc src/modbus_codec.c tests/modbus_test.c -o /tmp/modbus-test
/tmp/modbus-test
cc -Wall -Wextra -Werror -Isrc src/http_request.c tests/http_request_test.c -o /tmp/http-request-test
/tmp/http-request-test
python3 tools/check_release.py
# Optional hardware checks (explicit target required):
python3 tools/check_wireless.py DEVICE_IP
python3 tools/check_multiclient.py DEVICE_IP --stress
```

CI 执行主机测试，以及干净环境的有屏／无屏 Zephyr 构建。CI 使用一次性占位凭据，删除生成配置和二进制，不上传固件附件。CI 不烧写实物，不能证明未测试板子的兼容性。

发布检查只允许明确列出的源码、模板和文档，扫描常见凭据模式且不打印凭据值。它检查暂存区，不覆盖所有历史、日志或附件。不同硬件和干净依赖安装都需要对应验证。

## 许可证

[MIT](LICENSE)。外部依赖与参考资料见[第三方声明](THIRD_PARTY_NOTICES.zh-CN.md)。
