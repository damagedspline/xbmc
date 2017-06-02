/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WINDOW_EVENTS_WIN10_H
#define WINDOW_EVENTS_WIN10_H

#pragma once

#include "windowing/WinEvents.h"
#include "input/touch/TouchTypes.h"
#include <concurrent_queue.h>

class CGenericTouchSwipeDetector;

class CWinEventsWin10 : public IWinEvents
{
public:
  void MessagePush(XBMC_Event *newEvent);
  bool MessagePump();
  virtual size_t GetQueueSize();

private:

  Concurrency::concurrent_queue<XBMC_Event> m_events;

  static int m_originalZoomDistance;
  static Pointer m_touchPointer;
  static CGenericTouchSwipeDetector *m_touchSwipeDetector;
};

#endif // WINDOW_EVENTS_WIN10_H
