# MiniVT

- [简体中文](./docs/README_CHN.md)
- [English](README.md)

MiniVT is a simple showcase of Intel CPU's virtualization technology VT-x on the Windows platform.

## Introduction

- MiniVT is an open-source Windows system driver developed over Intel CPU's virtualization technology, VT-x.
- The project was inspired by Ddvp from JoenChen. Huge shout out for his work. 👏
- The driver is primarily written in C/C++ and x86-64 ASM.
- It provides a minimal implementation of the VT-x technology on the Windows operating system.
- When loaded, the driver enables virtualization of the OS and acts as the VMM (Virtual Machine Monitor), becoming a system hypervisor.
- It allows the control over access, execution, and privileges of the system's registers, memory, and many instructions.

- The driver also provides an easy framework for various types of virtualization.
  - If needed, you can implement other parts like Extended Page Table (EPT), which enables kernel-level monitoring and control of memory operations.

## System Requirements

Early test results have shown that this driver can run on the following systems:

- Windows XP (with Service Pack 3)
- Windows 7 (with Service Pack 1)
- Windows 10 Version 1607 (build 14393)

## Notice

This project was originally published in a tutorial on [Kanxue](https://bbs.pediy.com) by Xiaobao on August 2, 2016. It has been archived on GitHub and will NOT receive any future updates.

## x86 Version

[MiniVT](https://github.com/xiaobao520123/MiniVT)

## References

- [[Original] Video tutorial for VT virtualization architecture (Lessons 1-6) (Fixed Links)](https://bbs.kanxue.com/thread-211973-1.htm)