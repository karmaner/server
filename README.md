# server
模块

## basic
### 日志模块

基础用法
LOG_XXX(fmt, ...);
LOG_XXX_STREAM << "thing" << std::endl; // YES it is a ostream!

### 配置模块
配置文件

yaml使用解析模块
使用yaml-cpp来解析Yaml文件, 对Log日志的配置进行了封装。

特化了容器 <--> yaml文件双向转化

__LogInit() 特化配置文件处理工具

### 线程模块

简单的封装了pthrad线程，让它支持RAII

### 协程模块
### 协程调度模块
### Hook模块
### socket模块
### 流模块


## stream
### 二进制流模块
### rock协议
### 其他


## http
### http解析
### http连接
### https协议
### ws协议

## io模块

## 分布式

## 数据库