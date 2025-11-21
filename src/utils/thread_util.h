#pragma once

#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>

/**
 * @brief 设置当前线程的名称
 *
 * @param name 要设置的线程名称（最多15个字符，第16个字符会被自动设为'\0'）
 * @return int 成功返回0，失败返回非0
 */
int set_thread_name(const char* name);

/**
 * @brief 获取当前线程的名称
 *
 * @return const char* 线程名称的指针（静态缓冲区，不需要释放）
 */
const char* get_thread_name();

/**
 * @brief 获取当前线程的ID
 *
 * @return int 当前线程的ID
 */
pid_t get_thread_id();