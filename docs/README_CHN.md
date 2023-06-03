# MiniVT

- [简体中文](README_CHN.md)
- [English](../README.md)

MiniVT 是一个 Windows 平台上的英特尔处理器虚拟化技术的简单示例。

## 简介

- MiniVT 是一个开源的 Windows 系统驱动程序，它基于英特尔处理器的虚拟化技术。
- 最初启发来自 JoenChen 开发的 Ollydbg 插件 Ddvp，超级感谢。👏
- 这个驱动程序主要使用 C/C++ 和 x86-64 汇编语言编写。
- 它是在 Windows 平台 VT-x 技术的最小化实现。
- 加载后，驱动程序会启用操作系统的虚拟化功能，并充当 VMM（虚拟机监视程序），执行内核监管。
- 它可以控制系统寄存器、内存和许多程序指令的访问、执行和权限控制。

- 该驱动程序还为各种类型的虚拟化提供了简便的框架。
  - 如果需要，你可以添加和实现其他功能，如扩展页表（EPT）。EPT 可以实现内核级别的监控和对内存操作的控制。

## 系统要求

早期的测试结果显示，该驱动程序可在以下系统上运行：

- Windows XP（with Service Pack 3）
- Windows 7（with Service Pack 1）
- Windows 10 Version 1607（Build 14393）

## 注意事项

该项目最初是由小宝（Xiaobao）在 2016年8月2日在 [看雪论坛](https://bbs.pediy.com) 上发布的教程的项目。它现在被交由 Github 托管，不再维护。

## x86 版本

[MiniVT](https://github.com/xiaobao520123/MiniVT)

## 参考资料

- [[原创] VT虚拟化架构编写视频教程①~⑥课（已修复链接）](https://bbs.kanxue.com/thread-211973-1.htm)