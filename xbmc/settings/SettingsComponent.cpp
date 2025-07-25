/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsComponent.h"

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/AppParams.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#ifdef TARGET_DARWIN_EMBEDDED
#include "platform/darwin/ios-common/DarwinEmbedUtils.h"
#endif
#ifdef TARGET_WINDOWS
#include "platform/Environment.h"
#endif
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SubtitlesSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#ifdef TARGET_WINDOWS
#include "win32util.h"
#endif

namespace
{
void CreateUserDirs()
{
  XFILE::CDirectory::Create("special://home/");
  XFILE::CDirectory::Create("special://home/addons");
  XFILE::CDirectory::Create("special://home/addons/packages");
  XFILE::CDirectory::Create("special://home/addons/temp");
  XFILE::CDirectory::Create("special://home/media");
  XFILE::CDirectory::Create("special://home/system");
  XFILE::CDirectory::Create("special://masterprofile/");
  XFILE::CDirectory::Create("special://temp/");
  XFILE::CDirectory::Create("special://logpath");
  XFILE::CDirectory::Create("special://temp/temp"); // temp directory for python and dllGetTempPathA

  //Let's clear our archive cache before starting up anything more
  const std::string archiveCachePath =
      CSpecialProtocol::TranslatePath("special://temp/archive_cache/");
  if (XFILE::CDirectory::Exists(archiveCachePath) &&
      !XFILE::CDirectory::RemoveRecursive(archiveCachePath))
    CLog::Log(LOGWARNING, "Failed to remove the archive cache at {}", archiveCachePath);

  XFILE::CDirectory::Create(archiveCachePath);
}

bool InitDirectoriesLinux(bool bPlatformDirectories)
{
  /*
   The following is the directory mapping for Platform Specific Mode:

   special://xbmc/          => [read-only] system directory (/usr/share/kodi)
   special://home/          => [read-write] user's directory that will override special://kodi/ system-wide
   installations like skins, screensavers, etc.
   ($HOME/.kodi)
   NOTE: XBMC will look in both special://xbmc/addons and special://home/addons for addons.
   special://masterprofile/ => [read-write] userdata of master profile. It will by default be
   mapped to special://home/userdata ($HOME/.kodi/userdata)
   special://profile/       => [read-write] current profile's userdata directory.
   Generally special://masterprofile for the master profile or
   special://masterprofile/profiles/<profile_name> for other profiles.

   NOTE: All these root directories are lowercase. Some of the sub-directories
   might be mixed case.
   */

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  std::string appPath;
  std::string appName = CCompileInfo::GetAppName();
  std::string dotLowerAppName = "." + appName;
  StringUtils::ToLower(dotLowerAppName);
  const char* envAppHome = "KODI_HOME";
  const char* envAppBinHome = "KODI_BIN_HOME";
  const char* envAppTemp = "KODI_TEMP";

  std::string userHome;
  if (getenv("KODI_DATA"))
    userHome = getenv("KODI_DATA");
  else if (getenv("HOME"))
  {
    userHome = getenv("HOME");
    userHome.append("/" + dotLowerAppName);
  }
  else
  {
    userHome = "/root";
    userHome.append("/" + dotLowerAppName);
  }

  std::string strTempPath;
  if (getenv(envAppTemp))
    strTempPath = getenv(envAppTemp);
  else
    strTempPath = userHome + "/temp";


  std::string binaddonAltDir;
  if (getenv("KODI_BINADDON_PATH"))
    binaddonAltDir = getenv("KODI_BINADDON_PATH");

  const std::string appBinPath = CUtil::GetHomePath(envAppBinHome);
  // overridden by user
  if (getenv(envAppHome))
    appPath = getenv(envAppHome);
  else
  {
    // use build time default
    appPath = INSTALL_PATH;
    /* Check if binaries and arch independent data files are being kept in
     * separate locations. */
    if (!XFILE::CDirectory::Exists(URIUtils::AddFileToFolder(appPath, "userdata")))
    {
      /* Attempt to locate arch independent data files. */
      appPath = CUtil::GetHomePath(appBinPath);
      if (!XFILE::CDirectory::Exists(URIUtils::AddFileToFolder(appPath, "userdata")))
      {
        fprintf(stderr, "Unable to find path to %s data files!\n", appName.c_str());
        exit(1);
      }
    }
  }

  /* Set some environment variables */
  setenv(envAppBinHome, appBinPath.c_str(), 0);
  setenv(envAppHome, appPath.c_str(), 0);

  if (bPlatformDirectories)
  {
    // map our special drives
    CSpecialProtocol::SetXBMCBinPath(appBinPath);
    CSpecialProtocol::SetXBMCAltBinAddonPath(binaddonAltDir);
    CSpecialProtocol::SetXBMCPath(appPath);
    CSpecialProtocol::SetHomePath(userHome);
    CSpecialProtocol::SetMasterProfilePath(userHome + "/userdata");
    CSpecialProtocol::SetTempPath(strTempPath);
    CSpecialProtocol::SetLogPath(strTempPath);

    CreateUserDirs();

  }
  else
  {
    URIUtils::AddSlashAtEnd(appPath);

    CSpecialProtocol::SetXBMCBinPath(appBinPath);
    CSpecialProtocol::SetXBMCAltBinAddonPath(binaddonAltDir);
    CSpecialProtocol::SetXBMCPath(appPath);
    CSpecialProtocol::SetHomePath(URIUtils::AddFileToFolder(appPath, "portable_data"));
    CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(appPath, "portable_data/userdata"));

    std::string strTempPath = appPath;
    strTempPath = URIUtils::AddFileToFolder(strTempPath, "portable_data/temp");
    if (getenv(envAppTemp))
      strTempPath = getenv(envAppTemp);
    CSpecialProtocol::SetTempPath(strTempPath);
    CSpecialProtocol::SetLogPath(strTempPath);
    CreateUserDirs();
  }
  CSpecialProtocol::SetXBMCBinAddonPath(appBinPath + "/addons");

  return true;
#else
  return false;
#endif
}

#if defined(TARGET_DARWIN)
bool InitDirectoriesOSX(bool bPlatformDirectories)
#else
bool InitDirectoriesOSX(bool /*bPlatformDirectories*/)
#endif
{
#if defined(TARGET_DARWIN)
  std::string userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";

  std::string binaddonAltDir;
  if (getenv("KODI_BINADDON_PATH"))
    binaddonAltDir = getenv("KODI_BINADDON_PATH");

  std::string appPath = CUtil::GetHomePath();
  setenv("KODI_HOME", appPath.c_str(), 0);

#if defined(TARGET_DARWIN_EMBEDDED)
  std::string fontconfigPath;
  fontconfigPath = appPath + "/system/players/VideoPlayer/etc/fonts/fonts.conf";
  setenv("FONTCONFIG_FILE", fontconfigPath.c_str(), 0);
#endif

  // setup path to our internal dylibs so loader can find them
  std::string frameworksPath = CUtil::GetFrameworksPath();
  CSpecialProtocol::SetXBMCFrameworksPath(frameworksPath);

  if (bPlatformDirectories)
  {
    // map our special drives
    CSpecialProtocol::SetXBMCBinPath(appPath);
    CSpecialProtocol::SetXBMCAltBinAddonPath(binaddonAltDir);
    CSpecialProtocol::SetXBMCPath(appPath);
#if defined(TARGET_DARWIN_EMBEDDED)
    std::string appName = CCompileInfo::GetAppName();
    CSpecialProtocol::SetHomePath(userHome + "/" + CDarwinEmbedUtils::GetAppRootFolder() + "/" +
                                  appName);
    CSpecialProtocol::SetMasterProfilePath(userHome + "/" + CDarwinEmbedUtils::GetAppRootFolder() +
                                           "/" + appName + "/userdata");
#else
    std::string appName = CCompileInfo::GetAppName();
    CSpecialProtocol::SetHomePath(userHome + "/Library/Application Support/" + appName);
    CSpecialProtocol::SetMasterProfilePath(userHome + "/Library/Application Support/" + appName + "/userdata");
#endif

    std::string dotLowerAppName = "." + appName;
    StringUtils::ToLower(dotLowerAppName);
    // location for temp files
#if defined(TARGET_DARWIN_EMBEDDED)
    std::string strTempPath = URIUtils::AddFileToFolder(
        userHome, std::string(CDarwinEmbedUtils::GetAppRootFolder()) + "/" + appName + "/temp");
#else
    std::string strTempPath = URIUtils::AddFileToFolder(userHome, dotLowerAppName + "/");
    XFILE::CDirectory::Create(strTempPath);
    strTempPath = URIUtils::AddFileToFolder(userHome, dotLowerAppName + "/temp");
#endif
    CSpecialProtocol::SetTempPath(strTempPath);

    // xbmc.log file location
#if defined(TARGET_DARWIN_EMBEDDED)
    strTempPath = userHome + "/" + std::string(CDarwinEmbedUtils::GetAppRootFolder());
#else
    strTempPath = userHome + "/Library/Logs";
#endif
    CSpecialProtocol::SetLogPath(strTempPath);
    CreateUserDirs();
  }
  else
  {
    URIUtils::AddSlashAtEnd(appPath);

    CSpecialProtocol::SetXBMCBinPath(appPath);
    CSpecialProtocol::SetXBMCAltBinAddonPath(binaddonAltDir);
    CSpecialProtocol::SetXBMCPath(appPath);
    CSpecialProtocol::SetHomePath(URIUtils::AddFileToFolder(appPath, "portable_data"));
    CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(appPath, "portable_data/userdata"));

    std::string strTempPath = URIUtils::AddFileToFolder(appPath, "portable_data/temp");
    CSpecialProtocol::SetTempPath(strTempPath);
    CSpecialProtocol::SetLogPath(strTempPath);
    CreateUserDirs();
  }
  CSpecialProtocol::SetXBMCBinAddonPath(appPath + "/addons");
  return true;
#else
  return false;
#endif
}

bool InitDirectoriesWin32(bool bPlatformDirectories)
{
#ifdef TARGET_WINDOWS
  std::string xbmcPath = CUtil::GetHomePath();
  CEnvironment::setenv("KODI_HOME", xbmcPath);
  CSpecialProtocol::SetXBMCBinPath(xbmcPath);
  CSpecialProtocol::SetXBMCPath(xbmcPath);
  CSpecialProtocol::SetXBMCBinAddonPath(xbmcPath + "/addons");

  std::string strWin32UserFolder = CWIN32Util::GetProfilePath(bPlatformDirectories);
  CSpecialProtocol::SetLogPath(strWin32UserFolder);
  CSpecialProtocol::SetHomePath(strWin32UserFolder);
  CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(strWin32UserFolder, "userdata"));
  CSpecialProtocol::SetTempPath(URIUtils::AddFileToFolder(strWin32UserFolder,"cache"));

  CEnvironment::setenv("KODI_PROFILE_USERDATA", CSpecialProtocol::TranslatePath("special://masterprofile/"));

  CreateUserDirs();

  return true;
#else
  return false;
#endif
}
} // unnamed namespace

CSettingsComponent::CSettingsComponent()
  : m_settings(std::make_shared<CSettings>()),
    m_advancedSettings(std::make_shared<CAdvancedSettings>()),
    m_subtitlesSettings(std::make_shared<KODI::SUBTITLES::CSubtitlesSettings>(m_settings)),
    m_profileManager(std::make_shared<CProfileManager>())
{
}

CSettingsComponent::~CSettingsComponent() = default;

void CSettingsComponent::Initialize()
{
  if (m_state == State::DEINITED)
  {
    const std::shared_ptr<const CAppParams> params = CServiceBroker::GetAppParams();

    // only the InitDirectories* for the current platform should return true
    InitDirectoriesLinux(params->HasPlatformDirectories()) ||
        InitDirectoriesOSX(params->HasPlatformDirectories()) ||
        InitDirectoriesWin32(params->HasPlatformDirectories());

    m_settings->Initialize();

    m_advancedSettings->Initialize(*m_settings->GetSettingsManager());
    URIUtils::RegisterAdvancedSettings(*m_advancedSettings);

    m_profileManager->Initialize(m_settings);

    m_state = State::INITED;
  }
}

bool CSettingsComponent::Load()
{
  if (m_state == State::INITED)
  {
    if (!m_profileManager->Load())
    {
      CLog::Log(LOGFATAL, "unable to load profile");
      return false;
    }

    CSpecialProtocol::RegisterProfileManager(*m_profileManager);
    XFILE::IDirectory::RegisterProfileManager(*m_profileManager);

    if (!m_settings->Load())
    {
      CLog::Log(LOGFATAL, "unable to load settings");
      return false;
    }

    m_settings->SetLoaded();

    m_state = State::LOADED;
    return true;
  }
  else if (m_state == State::LOADED)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void CSettingsComponent::Deinitialize()
{
  if (m_state >= State::INITED)
  {
    if (m_state == State::LOADED)
    {
      m_subtitlesSettings.reset();

      m_settings->Unload();

      XFILE::IDirectory::UnregisterProfileManager();
      CSpecialProtocol::UnregisterProfileManager();
    }
    m_profileManager->Uninitialize();

    URIUtils::UnregisterAdvancedSettings();
    m_advancedSettings->Uninitialize(*m_settings->GetSettingsManager());

    m_settings->Uninitialize();
  }
  m_state = State::DEINITED;
}

std::shared_ptr<CSettings> CSettingsComponent::GetSettings() const
{
  return m_settings;
}

std::shared_ptr<CAdvancedSettings> CSettingsComponent::GetAdvancedSettings() const
{
  return m_advancedSettings;
}

std::shared_ptr<KODI::SUBTITLES::CSubtitlesSettings> CSettingsComponent::GetSubtitlesSettings()
    const
{
  return m_subtitlesSettings;
}

std::shared_ptr<CProfileManager> CSettingsComponent::GetProfileManager() const
{
  return m_profileManager;
}
