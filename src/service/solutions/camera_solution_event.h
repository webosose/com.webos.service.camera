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

typedef struct jvalue* jvalue_ref;
struct CameraSolutionEvent {
  virtual void onInitialized(void) {}
  virtual void onEnabled(void) {}
  virtual void onDone(jvalue_ref) {}
  virtual void onDisabled(void) {}
  virtual void onReleased(void) {}
};
