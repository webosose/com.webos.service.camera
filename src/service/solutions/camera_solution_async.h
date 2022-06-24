/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolution.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution
 *
 */
#pragma once
#include <thread>
#include <memory>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <cstring>
#include "camera_solution.h"
#include "camera_hal_if.h"

class CameraSolutionAsync : public CameraSolution {
public:
  struct Buffer {
    uint8_t* data_ {nullptr};
    uint32_t size_ {0};
    Buffer(uint8_t* data, uint32_t size);
    ~Buffer(void);
  };
  using Queue = std::queue<std::unique_ptr<Buffer>>;
  using Thread = std::unique_ptr<std::thread>;
  enum WaitResult { OK, TIMEOUT, ERROR };
public:
  CameraSolutionAsync(void);
  virtual ~CameraSolutionAsync(void);
public:
  // interface override
  virtual void setEnableValue(bool enableValue) override;
  virtual void processForSnapshot(buffer_t inBuf) override;
  virtual void processForPreview(buffer_t inBuf) override;
protected:
  virtual void run(void);
  virtual void processing(void) = 0;
protected:
  void startThread(void);
  void stopThread(void);
  void notify(void);
  WaitResult wait(void);
  bool checkAlive(void);
  void setAlive(bool bAlive);
protected:
  void pushJob(buffer_t inBuf);
  void popJob(void);
protected:
  std::condition_variable cv_;
  std::mutex m_;
  std::mutex mtxJob_;
  Queue queueJob_;
  Thread threadJob_;
  std::atomic<bool> bAlive_ {false};
};
