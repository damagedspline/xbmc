/*
 *      Copyright (C) 2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/*!
  * \brief Interface for remote control
  */
class IRemoteControl
{
public:
  virtual ~IRemoteControl() = default;

  /*!
   * \brief Call once during input manager initialization
   */
  virtual void Initialize() = 0;

  /*!
   * \brief Deinitialize remote control
   */
  virtual void Disconnect() = 0;

  /*!
   * \brief Get the button readed from IR
   *
   * \return A button code
   */
  virtual unsigned short GetButton() const = 0;

  /*!
   * \brief Get the time while a button was hold
   *
   * \return A hold time
   */
  virtual unsigned int GetHoldTime() const = 0;

  /*!
   * \brief Update button state 
   */
  virtual void Update() = 0;

  /*!
   * \brief Reset button state 
   */
  virtual void Reset() = 0;

  /*!
   * \brief Get the initialized status
   */
  virtual bool IsInitialized() const = 0;

  /*!
   * \brief Set enabled status
   */
  virtual void SetEnabled(bool) { }

  /*!
   * \brief Set a name for a device
   */
  virtual void SetDeviceName(const std::string&) { }

  /*!
   * \brief Add a command to send to a bus
   */
  virtual void AddSendCommand(const std::string&) { }

  /*!
   * \brief Get the in-use status
   */
  virtual bool IsInUse() const { return false; }
};
