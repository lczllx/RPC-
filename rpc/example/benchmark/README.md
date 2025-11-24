# RPC 性能测试工具

## 概述

本目录包含 RPC 框架的性能测试工具，用于测试延迟、吞吐量、并发性能等指标。

## 编译

```bash
cd /path/to/rpc
cmake -S . -B build
cmake --build build
```

编译后会生成两个可执行文件：
- `build/example/benchmark/benchmark_server` - 性能测试服务端
- `build/example/benchmark/benchmark_client` - 性能测试客户端

## 使用方法

### 1. 启动服务端

```bash
# 基本用法（端口 8889，不使用服务发现）
./build/example/benchmark/benchmark_server

# 指定端口
./build/example/benchmark/benchmark_server 9999

# 启用服务发现（需要先启动注册中心）
./build/example/benchmark/benchmark_server 8889 1 8080
```

参数说明：
- 参数1: 服务端端口（默认 8889）
- 参数2: 是否启用服务发现（0/1，默认 0）
- 参数3: 注册中心端口（默认 8080）

### 2. 运行性能测试

#### 单线程延迟测试

测试单线程下的请求延迟和 QPS：

```bash
# 测试 add 方法，10000 个请求
./build/example/benchmark/benchmark_client single add 10000

# 测试 echo 方法（空操作，测试框架开销）
./build/example/benchmark/benchmark_client single echo 10000

# 测试 heavy_compute 方法（耗时操作）
./build/example/benchmark/benchmark_client single heavy_compute 10000
```

#### 多线程并发测试

测试多线程并发下的性能：

```bash
# 4 个线程，总共 10000 个请求
./build/example/benchmark/benchmark_client multi add 10000 4

# 8 个线程，总共 50000 个请求
./build/example/benchmark/benchmark_client multi add 50000 8
```

#### 吞吐量测试

在固定时间内持续发送请求，测试最大吞吐量：

```bash
# 持续 10 秒
./build/example/benchmark/benchmark_client throughput add 0 0 10

# 持续 60 秒
./build/example/benchmark/benchmark_client throughput add 0 0 60
```

### 3. 完整参数说明

```bash
./build/example/benchmark/benchmark_client <test_type> <method> <requests> <threads> <duration> <use_discover> <server_ip> <server_port> <registry_port>
```

参数说明：
- `test_type`: 测试类型（single/multi/throughput）
- `method`: 方法名（add/echo/heavy_compute）
- `requests`: 请求总数（throughput 模式忽略）
- `threads`: 线程数（single 模式忽略）
- `duration`: 持续时间（秒，仅 throughput 模式）
- `use_discover`: 是否使用服务发现（0/1，默认 0）
- `server_ip`: 服务器 IP（默认 127.0.0.1）
- `server_port`: 服务器端口（默认 8889）
- `registry_port`: 注册中心端口（默认 8080）

## 测试指标说明

测试结果包含以下指标：

### 基本指标
- **总请求数**: 发送的请求总数
- **成功请求**: 成功返回的请求数
- **失败请求**: 失败的请求数
- **成功率**: 成功请求占总请求的百分比
- **测试时长**: 测试持续的时间（毫秒）
- **QPS**: 每秒处理的请求数（Queries Per Second）

### 延迟指标（微秒）
- **最小值**: 最短响应时间
- **平均值**: 平均响应时间
- **P50**: 50% 的请求响应时间小于此值（中位数）
- **P90**: 90% 的请求响应时间小于此值
- **P95**: 95% 的请求响应时间小于此值
- **P99**: 99% 的请求响应时间小于此值
- **最大值**: 最长响应时间

## 测试场景

### 1. 框架开销测试

使用 `echo` 方法测试框架本身的开销（空操作）：

```bash
./build/example/benchmark/benchmark_server 8889
./build/example/benchmark/benchmark_client single echo 100000
```

### 2. 轻量级操作测试

使用 `add` 方法测试简单计算的性能：

```bash
./build/example/benchmark/benchmark_client single add 100000
```

### 3. 耗时操作测试

使用 `heavy_compute` 方法测试耗时操作的性能：

```bash
./build/example/benchmark/benchmark_client single heavy_compute 10000
```

### 4. 并发性能测试

测试不同并发度下的性能：

```bash
# 1 线程
./build/example/benchmark/benchmark_client multi add 50000 1

# 4 线程
./build/example/benchmark/benchmark_client multi add 50000 4

# 8 线程
./build/example/benchmark/benchmark_client multi add 50000 8

# 16 线程
./build/example/benchmark/benchmark_client multi add 50000 16
```

### 5. 压力测试

持续发送请求，观察系统在长时间高负载下的表现：

```bash
# 持续 60 秒
./build/example/benchmark/benchmark_client throughput add 0 0 60
```

## 性能优化建议

根据测试结果，可以从以下方面优化：

1. **延迟优化**
   - 如果 P99 延迟过高，检查网络层和序列化性能
   - 优化消息分发器的查找效率
   - 减少不必要的内存拷贝

2. **吞吐量优化**
   - 如果 QPS 较低，检查服务端线程模型
   - 优化消息序列化/反序列化
   - 使用连接池减少连接开销

3. **并发优化**
   - 如果多线程性能提升不明显，检查锁竞争
   - 优化共享数据结构
   - 考虑无锁数据结构

## 示例输出

```
========== RPC 性能测试 ==========
测试类型: single
方法名: add
服务器: 127.0.0.1:8889
使用服务发现: 否
单线程测试，请求数: 10000

========== 性能测试结果 ==========
总请求数: 10000
成功请求: 10000
失败请求: 0
成功率: 100.00%
测试时长: 1234 ms
QPS: 8103.73

延迟统计 (微秒):
  最小值: 45.23 us
  平均值: 123.45 us
  P50:    98.76 us
  P90:    234.56 us
  P95:    345.67 us
  P99:    567.89 us
  最大值: 1234.56 us
==================================
```

## 注意事项

1. 测试前确保服务端已启动
2. 测试环境应尽量干净，避免其他程序干扰
3. 多次测试取平均值，避免偶然因素影响
4. 不同硬件环境测试结果会有差异
5. 建议在 Release 模式下编译测试（`cmake -DCMAKE_BUILD_TYPE=Release`）




