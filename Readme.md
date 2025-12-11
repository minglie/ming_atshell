
# 🧭 嵌入式终端命令交互系统 `AtShell`

`AtShell` 是一个适用于嵌入式系统的串口命令解析框架，支持两种交互模式（AT / MSH），用于设备调试、指令执行、格式化输出、命令历史与自动补全等功能。

---
## 🌐 在线演示

👉 [Wokwi 在线演示](https://wokwi.com/projects/402006915303326721)

📦️ [git仓库](https://github.com/minglie/ming_atshell)

---



## ✨ 模式说明

### 🟢 AT 模式（适用于串口助手）

- **格式：** `AT+fun(a,b,c)\r\n`
- **特点：**
  - 结构固定，适用于机器通信、协议调试
  - 支持固件升级（如配合 Xmodem1K）
- **示例：**
  ```cpp
  AT+c(55,01 02)
  ```

### 🔵 MSH 模式（类似 Linux Shell）

- **格式：** `fun a b`
- **特点：**
  - 支持直接通过命令序号执行对应的命令
  - 支持 tab 自动补全和历史记录 ↑↓
  - 类 Linux Shell 操作风格，适合调试
- **示例：**
  ```cpp
  help
  clean
  call 0x55 01 02
  ```

---




## 🔧 核心特性一览

| 特性                  | 描述 |
|-----------------------|------|
| 多命令注册           | 支持动态注册命令集 |
| 参数解析             | 支持 `,` 与空格分割参数 |
| 日志输出重定向       | 提供 info/debug/error 等日志级别 |
| 命令历史与补全       | MSH 模式下支持 ↑↓ 查阅、tab 补全 |
| 异步缓冲输出         | 适用于中断上下文延迟输出 |
| 模式可配置           | 通过宏配置启用 AT 或 MSH 模式 |

---

## 📁 文件结构说明

### `AtShell.h`

声明核心类 `AtShell` 及其接口：

```cpp
// 注册命令
bool Regist(AT_CMD_ENTRY_TypeDef cmd);

// 执行命令
bool Exec(char *str);

// 导入数据
int Import(uint8_t *buf, uint32_t len, uint32_t ms = 0);

// 格式化输出
int Printf(const char *format, ...);

// 返回状态响应
int Reply(uint8_t errCode);
```

快速注册命令：
```cpp
AT_SHELL_EXPORT(cmdName, desc, fun, userData);
```

---

### `AtShell.cpp`

实现串口数据解析、命令匹配与执行、日志输出等逻辑。

#### ✅ 默认内建命令

| 命令       | 描述             | 支持模式 |
|------------|------------------|----------|
| `help`     | 显示命令列表     | 全部     |
| `c(...)`   | 执行回调函数     | AT 模式  |
| `clean`    | 清屏操作         | MSH 模式 |
| `AT`       | 回显确认         | AT 模式  |

---

## 🧾 输出接口示例

```cpp
AT_printf("Result: %d\n", result);
AT_info("Info message: %s", msg);
```

---

## 🔗 C 语言接口（兼容非 C++ 项目）

```cpp
void at_init(ATWriteFun writeFun);
bool at_register(AT_CMD_ENTRY_TypeDef cmd);
int  at_import(uint8_t *buf, uint32_t len, uint32_t ms);
int  at_reply(uint8_t errCode);
```

---

## ⚙️ 编译宏配置表

| 宏定义                   | 功能说明                         |
|--------------------------|----------------------------------|
| `CON_AT_MSH`             | 0 表示 AT 模式，1 表示 MSH 模式 |
| `CON_AT_USE_CALLBACK`    | 支持 `AT+c(...)` 回调机制        |
| `CON_AT_USE_EXPORT`      | 启用异步缓冲输出（适用于中断）   |
| `RT_FINSH_ARG_MAX`       | 命令最大参数数量（默认 3）       |
| `FINSH_HISTORY_LINES`    | MSH 模式支持的历史命令条数       |

---

## 🧪 使用示例

```cpp
// 注册命令
AT_SHELL_EXPORT(mycmd, "custom test", my_function, NULL);

// 执行命令
g_atShell.Exec("mycmd 123 abc");

// 返回信息
AT_reply(CON_AT_R_SUCCESS);
AT_printf("Executed successfully\n");
```

---

## 📦 移植指南

本系统轻量、易移植，仅依赖标准 C 库函数：

> `string.h`, `stdarg.h`, `ctype.h`, `stdio.h`

可应用于：**串口 / USB CDC / BLE / TCP 等通信场景**

### ✅ 移植步骤

#### ① 初始化输出函数（串口发送）

```cpp
at_init(my_write_fun);  // 设置写函数
```

#### ② 导入串口接收数据

```cpp
at_import(rx_buf, len, get_ms());  // 在串口接收中调用
```

---

> `AtShell` 框架设计清晰，代码结构良好，适合在 MCU / Linux / RTOS 等嵌入式环境中作为命令行接口内核使用。
