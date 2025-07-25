/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRRecordingInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "video/guilib/VideoPlayActionProcessor.h"

using namespace PVR;

namespace
{
constexpr unsigned int CONTROL_BTN_FIND = 4;
constexpr unsigned int CONTROL_BTN_OK = 7;
constexpr unsigned int CONTROL_BTN_PLAY_RECORDING = 8;

} // unnamed namespace

CGUIDialogPVRRecordingInfo::CGUIDialogPVRRecordingInfo()
  : CGUIDialog(WINDOW_DIALOG_PVR_RECORDING_INFO, "DialogPVRInfo.xml"),
    m_recordItem(std::make_shared<CFileItem>())
{
}

bool CGUIDialogPVRRecordingInfo::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    return OnClickButtonOK(message) || OnClickButtonPlay(message) || OnClickButtonFind(message);
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogPVRRecordingInfo::OnClickButtonOK(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_OK)
  {
    Close();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRRecordingInfo::OnClickButtonPlay(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING)
  {
    Close();

    if (m_recordItem)
    {
      KODI::VIDEO::GUILIB::CVideoPlayActionProcessor proc{m_recordItem};
      proc.ProcessDefaultAction();
      if (proc.GetUserCancelled())
        Open();
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRRecordingInfo::OnClickButtonFind(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_FIND)
  {
    Close();

    if (m_recordItem)
      CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().FindSimilar(*m_recordItem);

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRRecordingInfo::OnInfo(int actionID)
{
  Close();
  return true;
}

void CGUIDialogPVRRecordingInfo::SetRecording(const CFileItem& item)
{
  m_recordItem = std::make_shared<CFileItem>(item);
}

CFileItemPtr CGUIDialogPVRRecordingInfo::GetCurrentListItem(int offset)
{
  return m_recordItem;
}
