#pragma once

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#ifdef _MSC_VER
// Requires Windows 7
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>
#include <tchar.h>
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <chrono>
#include <iostream>
#include <string>
#include <memory>
#include <random>
#include <map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

#define ASIO_STANDALONE 1

#include <asio.hpp>

// Some constants
constexpr std::size_t buf_size = 4096;
constexpr std::size_t outgoing_queue_size = 256;
constexpr std::size_t max_client_sockets = 10;

typedef asio::basic_waitable_timer<std::chrono::steady_clock> waitable_timer_t;
typedef asio::ip::udp::endpoint endpoint_t;
typedef asio::ip::udp::socket socket_t;

typedef std::lock_guard<std::mutex> guard;
typedef std::unique_lock<std::mutex> lock;

/**
 * Thread-local random generator
 * @return
 */
std::mt19937 &rng();

/**
 * Allocate a buffer with size `buf_size`
 * @return
 */
void *allocate_buffer();

/**
 * Deallocate a buffer previously allocated by `allocate_buffer`
 */
void deallocate_buffer(void *);

#endif	// !defined(COMMON_H_INCLUDED)