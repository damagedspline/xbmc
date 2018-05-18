/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
#pragma once

#define WINRT_STA_ASYNC_GUARD { if (winrt::impl::is_sta()) co_await winrt::resume_background(); }

namespace winrt
{
using namespace Windows::Foundation;

template <typename T> inline
auto wait(const T& async)
{
  if (async.Status() == winrt::AsyncStatus::Completed)
    return async.GetResults();

  impl::mutex m;
  impl::condition_variable cv;
  bool completed = false;

  async.Completed([&](auto&&, auto&&)
  {
    {
      impl::lock_guard<> const guard(m);
      completed = true;
    }

    cv.notify_one();
  });

  impl::lock_guard<> guard(m);
  cv.wait(m, [&] { return completed; });

  return async.GetResults();
}
}
