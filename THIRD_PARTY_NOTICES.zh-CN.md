# 第三方声明

[English](THIRD_PARTY_NOTICES.md) | [简体中文](THIRD_PARTY_NOTICES.zh-CN.md)

项目应用源码采用 [MIT](LICENSE)。仓库不内置第三方依赖源码或二进制；构建时下载的依赖保留其上游许可和声明。

| 组件 | 来源与许可范围 |
|---|---|
| Zephyr 4.1.0 | [上游源码](https://github.com/zephyrproject-rtos/zephyr/tree/v4.1.0)，主要采用 Apache-2.0，保留逐文件及第三方声明 |
| Espressif/Xtensa HAL、Mbed TLS | 版本由 Zephyr 导入的 west manifest 指定；以各依赖 LICENSE 和逐文件声明为准 |
| ESP32 Wi-Fi blobs | 通过 `west blobs fetch hal_espressif` 单独下载，受 blob 元数据及 Espressif 上游条款约束 |
| Zephyr SDK 0.17.0 | [上游发布](https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.17.0)，编译器及运行库各有许可 |

现有 LCD 驱动带 MIT SPDX 标记。GitHub Dark 仅为配色参考，未包含 GitHub 标志或品牌素材。产品和芯片名称用于说明硬件，不代表厂商背书。

这里只分发应用源码。自行分发链接后的二进制前，须核对全部链接组件的许可；项目 MIT 不替代这些条款。本地构建固件还嵌入网络凭据。
