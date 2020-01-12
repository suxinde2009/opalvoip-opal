/*
 * main.h
 *
 * An OPAL analyser/player for RTP.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2015 Vox Lucida
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Opal Shark.
 *
 * The Initial Developer of the Original Code is Robert Jongbloed
 *
 * Contributor(s): ______________________________________.
 *
 */

#ifndef _OpalShark_MAIN_H
#define _OpalShark_MAIN_H

#if WXVER>29
  #define WXUSINGDLL 1
#endif

#include <ptlib_wx.h>
#include <opal/manager.h>
#include <rtp/pcapfile.h>
#include <wx/listctrl.h>

#ifndef OPAL_PTLIB_AUDIO
  #error Cannot compile without PTLib sound channel support!
#endif

// Note, include this after everything else so gets all the conversions
#include <ptlib/wxstring.h>


class MyManager;
class wxProgressDialog;
class wxGrid;
class wxGridEvent;
class wxNotebook;
class wxSpinCtrl;


struct MyOptions
{
  PwxString m_AudioDevice;
  int       m_VideoTiming;
  OpalPCAPFile::PayloadMap m_mappings;

  MyOptions()
    : m_AudioDevice("Default")
    , m_VideoTiming(0)
  { }
};


class OptionsDialog : public wxDialog
{
  public:
    OptionsDialog(MyManager *manager, const MyOptions & options);

    const MyOptions & GetOptions() const { return m_options; }

  private:
    virtual bool TransferDataFromWindow();
    void RefreshMappings();

    MyManager & m_manager;
    MyOptions   m_options;
    PwxString   m_screenAudioDevice;
    wxGrid    * m_mappings;

  wxDECLARE_EVENT_TABLE();
};


class MyPlayer;

class MediaStream : public wxListCtrl
{
  MyPlayer      & m_player;
  unsigned        m_clockRate;
  bool            m_isDecoded;
  bool            m_firstPacket;
  PTime           m_firstTime;
  PTime           m_lastTime;
  unsigned        m_firstTimestamp;
  unsigned        m_lastSequenceNumber;
  unsigned        m_lastPacketTimestamp;
  bool            m_lastFrameEnd;
  unsigned        m_lastFrameTimestamp;
  OpalAudioFormat m_audioFormat;
  OpalVideoFormat m_videoFormat;
  PThread       * m_thread;
  bool            m_running;
  OpalAudioFormat::FrameDetectorPtr m_audioFrameDetector;
  OpalVideoFormat::FrameDetectorPtr m_videoFrameDetector;
  OpalSilenceDetector::CalculateDB  m_dbCalculator;
  OpalPCAPFile::DiscoveredRTPInfo   m_filterInfo;
  OpalPCAPFile::DecodeContext       m_decodeContext;

  void OnAnalysisUpdate(wxString, bool);

  void PlayAudio();
  void PlayVideo();


public:
  MediaStream(MyPlayer & player, const OpalPCAPFile::DiscoveredRTPInfo & filterInfo);
  ~MediaStream();

  void StartThread();
  bool IsRunning() const { return m_running; }

  void Analyse(
    unsigned packetNumber,
    const RTP_DataFrame & encoded,
    const RTP_DataFrame & decoded,
    const PTime & thisTime,
    OpalVideoFormat::FrameType videoFrameType = OpalVideoFormat::e_UnknownFrameType
  );
};


class MyPlayer : public wxMDIChildFrame
{
  public:
    MyPlayer(MyManager * manager, const PFilePath & filename);
    ~MyPlayer();

    void OnCloseWindow(wxCloseEvent &);
    void OnClose(wxCommandEvent &);
    void OnListChanged(wxGridEvent &);
    void OnPlay(wxCommandEvent &);
    void OnStop(wxCommandEvent &);
    void OnPause(wxCommandEvent &);
    void OnResume(wxCommandEvent &);
    void OnStep(wxCommandEvent &);
    void OnAnalyse(wxCommandEvent &);

    void Discover(OpalPCAPFile * pcapFile);
    void OnDiscoverComplete();
    PDECLARE_NOTIFIER2(OpalPCAPFile, MyPlayer, DiscoverProgress, OpalPCAPFile::Progress &);

    enum Controls
    {
      CtlIdle,
      CtlRunning,
      CtlPause,
      CtlStep,
      CtlStop
    };
    void StartPlaying(Controls ctrl);
    void OnPaused();
    void OnPlayEnded();

    bool CanExport() const;
    void StartExport(const PFilePath & mediaFile);
    void Export(OpalPCAPFile * pcapFile, OpalRecordManager * recorder);
    void OnExportComplete();

  private:
    MyManager        & m_manager;
    PFilePath          m_pcapFilePath;
    PThread          * m_backgroundThread;
    auto_ptr<wxProgressDialog> m_progressDialog;

    OpalPCAPFile::DiscoveredRTP m_discoveredRTP;
    unsigned                    m_packetCount;

    enum
    {
      ColSrcIP,
      ColSrcPort,
      ColDstIP,
      ColDstPort,
      ColSSRC,
      ColPayloadType,
      ColFormat,
      ColPlay,
      NumCols
    };
    wxGrid * m_rtpList;
    std::set<unsigned> m_selectedRows;

    wxNotebook * m_streamTabs;
    typedef std::map<RTP_SyncSourceId, MediaStream *> StreamBySSRC;
    StreamBySSRC m_streamsBySSRC;

    Controls  m_playThreadCtrl;
    unsigned  m_pausePacket;

    wxButton   * m_play;
    wxButton   * m_stop;
    wxButton   * m_pause;
    wxButton   * m_resume;
    wxButton   * m_step;
    wxButton   * m_analyse;
    wxSpinCtrl * m_playToPacket;

  friend class MediaStream;

  wxDECLARE_EVENT_TABLE();
};


class MyManager : public wxMDIParentFrame, public OpalManager
{
  public:
    MyManager();
    ~MyManager();

    bool Initialise(bool startMinimised);
    void Load(wxString fname);

    const MyOptions GetOptions() const { return m_options; }

  private:
    void OnMenuOpen(wxMenuEvent &);

    void OnClose(wxCloseEvent &);
    void OnMenuQuit(wxCommandEvent &);
    void OnMenuAbout(wxCommandEvent &);
    void OnMenuOptions(wxCommandEvent &);
    void OnMenuOpenPCAP(wxCommandEvent &);
    void OnMenuCloseAll(wxCommandEvent &);
    void OnMenuExport(wxCommandEvent &);

    MyOptions m_options;
    PwxString m_lastExportFile;

  wxDECLARE_EVENT_TABLE();
};


class OpalSharkApp : public wxApp, public PProcess
{
    PCLASSINFO(OpalSharkApp, PProcess);

  public:
    OpalSharkApp();

    void Main(); // Dummy function

    // Function from wxWindows
    virtual bool OnInit();
};


#endif


// End of File ///////////////////////////////////////////////////////////////
