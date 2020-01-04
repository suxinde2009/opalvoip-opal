/*
 * main.cpp
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

#include "main.h"

#include "version.h"

#include <ptlib/sound.h>
#include <ptlib/vconvert.h>
#include <codec/vidcodec.h>
#include <ptclib/wxWinVidDev.h>

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/accel.h>
#include <wx/valgen.h>
#include <wx/progdlg.h>
#include <wx/cmdline.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/rawbmp.h>
#include <wx/tokenzr.h>


#if defined(__WXGTK__)   || \
    defined(__WXMOTIF__) || \
    defined(__WXX11__)   || \
    defined(__WXMAC__)   || \
    defined(__WXMGL__)   || \
    defined(__WXCOCOA__)
  #include <wx/gdicmn.h>
  #include "opalshark.xpm"
#endif


extern void InitXmlResource(); // From resource.cpp whichis compiled openphone.xrc


static const wxChar OpalSharkString[] = wxT("OPAL Shark");
static const wxChar OpalSharkErrorString[] = wxT("OPAL Shark Error");
static const wxChar GridTrueString[] = wxT("Yes");
static const wxChar GridFalseString[] = wxT("No");

// Definitions of the configuration file section and key names

#define DEF_FIELD(name) static const wxChar name##Key[] = wxT(#name)

static const wxChar AppearanceGroup[] = wxT("/Appearance");
DEF_FIELD(MainFrameX);
DEF_FIELD(MainFrameY);
DEF_FIELD(MainFrameWidth);
DEF_FIELD(MainFrameHeight);

static const wxChar OptionsGroup[] = wxT("/Options");
DEF_FIELD(AudioDevice);
DEF_FIELD(VideoTiming);
DEF_FIELD(LastExportFile);
static const wxChar MappingsGroup[] = wxT("/PayloadMappings");

// Menu and command identifiers
#define DEF_XRCID(name) static int ID_##name = XRCID(#name)
DEF_XRCID(MenuExport);
DEF_XRCID(Play);
DEF_XRCID(Stop);
DEF_XRCID(Pause);
DEF_XRCID(Resume);
DEF_XRCID(Step);
DEF_XRCID(Analyse);


#define PTraceModule() "OpalShark"


///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

template <class cls, typename id_type>
cls * FindWindowByNameAs(cls * & derivedChild, wxWindow * window, id_type name)
{
  wxWindow * baseChild = window->FindWindow(name);
  if (PAssert(baseChild != NULL, "Windows control not found")) {
    derivedChild = dynamic_cast<cls *>(baseChild);
    if (PAssert(derivedChild != NULL, "Cannot cast window object to selected class"))
      return derivedChild;
  }

  return NULL;
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif


static wxArrayString GetAllMediaFormatNames()
{
  wxArrayString formatNames;
  OpalMediaFormatList mediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
  for (OpalMediaFormatList::iterator it = mediaFormats.begin(); it != mediaFormats.end(); ++it) {
    if (it->IsTransportable() && (it->GetMediaType() == OpalMediaType::Audio() || it->GetMediaType() == OpalMediaType::Video()))
      formatNames.push_back(PwxString(it->GetName()));
  }
  return formatNames;
}


///////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_APP(OpalSharkApp);

OpalSharkApp::OpalSharkApp()
  : PProcess(MANUFACTURER_TEXT, PRODUCT_NAME_TEXT,
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, PATCH_VERSION)
{
}


void OpalSharkApp::Main()
{
  // Dummy function
}

//////////////////////////////////

bool OpalSharkApp::OnInit()
{
  // Check command line arguments
  static const wxCmdLineEntryDesc cmdLineDesc[] =
  {
#if wxCHECK_VERSION(2,9,0)
  #define wxCMD_LINE_DESC(kind, shortName, longName, description, ...) \
      { kind, shortName, longName, description, __VA_ARGS__ }
#else
  #define wxCMD_LINE_DESC(kind, shortName, longName, description, ...) \
      { kind, wxT(shortName), wxT(longName), wxT(description), __VA_ARGS__ }
  #define wxCMD_LINE_DESC_END { wxCMD_LINE_NONE, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, 0 }
#endif
      wxCMD_LINE_DESC(wxCMD_LINE_SWITCH, "h", "help", "", wxCMD_LINE_VAL_NONE,  wxCMD_LINE_OPTION_HELP),
      wxCMD_LINE_DESC(wxCMD_LINE_OPTION, "n", "config-name", "Set name to use for configuration", wxCMD_LINE_VAL_STRING),
      wxCMD_LINE_DESC(wxCMD_LINE_OPTION, "f", "config-file", "Use specified file for configuration", wxCMD_LINE_VAL_STRING),
      wxCMD_LINE_DESC(wxCMD_LINE_SWITCH, "m", "minimised", "Start application minimised"),
#if PTRACING
      wxCMD_LINE_DESC(wxCMD_LINE_OPTION, "t", "trace-level",  "Trace log level", wxCMD_LINE_VAL_NUMBER),
      wxCMD_LINE_DESC(wxCMD_LINE_OPTION, "o", "trace-output", "Trace output file", wxCMD_LINE_VAL_STRING),
#endif
      wxCMD_LINE_DESC(wxCMD_LINE_PARAM,  "", "", "PCAP file to play", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_PARAM_MULTIPLE),
      wxCMD_LINE_DESC_END
  };

  wxCmdLineParser cmdLine(cmdLineDesc, wxApp::argc,  wxApp::argv);
  if (cmdLine.Parse() != 0)
    return false;

  {
    PwxString name(GetName());
    PwxString manufacture(GetManufacturer());
    PwxString filename;
    cmdLine.Found(wxT("config-name"), &name);
    cmdLine.Found(wxT("config-file"), &filename);
    wxConfig::Set(new wxConfig(name, manufacture, filename));
  }

#if PTRACING
  long level;
  if (cmdLine.Found("trace-level", &level)) {
    wxString file;
    if (cmdLine.Found("trace-output", &file))
      PTrace::Initialise(level, file.c_str());
    else
      PTrace::Initialise(level);
  }
#endif

  // Create the main frame window
  MyManager * main = new MyManager();
  SetTopWindow(main);

  wxBeginBusyCursor();

  bool ok = main->Initialise(cmdLine.Found(wxT("minimised")));
  if (ok) {
    for (size_t i = 0; i < cmdLine.GetParamCount(); ++i)
      main->CallAfter<MyManager, wxString>(&MyManager::Load, cmdLine.GetParam(i));
  }

  wxEndBusyCursor();
  return ok;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyManager, wxMDIParentFrame)
  EVT_CLOSE(MyManager::OnClose)

  EVT_MENU_OPEN(MyManager::OnMenuOpen)
  EVT_MENU_CLOSE(MyManager::OnMenuClose)

  EVT_MENU(wxID_EXIT,        MyManager::OnMenuQuit)
  EVT_MENU(wxID_ABOUT,       MyManager::OnMenuAbout)
  EVT_MENU(wxID_PREFERENCES, MyManager::OnMenuOptions)
  EVT_MENU(wxID_OPEN,        MyManager::OnMenuOpenPCAP)
  EVT_MENU(wxID_CLOSE_ALL,   MyManager::OnMenuCloseAll)
  EVT_MENU(ID_MenuExport,    MyManager::OnMenuExport)
END_EVENT_TABLE()


MyManager::MyManager()
  : wxMDIParentFrame(NULL, wxID_ANY, OpalSharkString, wxDefaultPosition, wxSize(640, 480))
{
  // Give it an icon
  SetIcon(wxICON(AppIcon));
}


MyManager::~MyManager()
{
  // and disconnect it to prevent accessing already deleted m_textWindow in
  // the size event handler if it's called during destruction
  Disconnect(wxEVT_SIZE, wxSizeEventHandler(MyManager::OnSize));

  delete wxXmlResource::Set(NULL);
}


bool MyManager::Initialise(bool startMinimised)
{
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

  wxGridCellBoolEditor::UseStringValues(GridTrueString, GridFalseString);

  // Make a menubar
  wxMenuBar * menubar;
  {
    PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;
    if ((menubar = wxXmlResource::Get()->LoadMenuBar(wxT("MenuBar"))) == NULL)
      return false;
    SetMenuBar(menubar);
  }

  wxAcceleratorEntry accelEntries[] = {
      wxAcceleratorEntry(wxACCEL_CTRL,  'O', wxID_OPEN),
      wxAcceleratorEntry(wxACCEL_CTRL,  'A', wxID_ABOUT),
      wxAcceleratorEntry(wxACCEL_CTRL,  'X', wxID_EXIT)
  };
  SetAcceleratorTable(wxAcceleratorTable(PARRAYSIZE(accelEntries), accelEntries));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PwxString(AppearanceGroup));

  wxPoint initalPosition = wxDefaultPosition;
  if (config->Read(MainFrameXKey, &initalPosition.x) && config->Read(MainFrameYKey, &initalPosition.y))
    Move(initalPosition);

  wxSize initialSize(1024, 768);
  if (config->Read(MainFrameWidthKey, &initialSize.x) && config->Read(MainFrameHeightKey, &initialSize.y))
    SetSize(initialSize);

  // connect it only now, after creating m_textWindow
  Connect(wxEVT_SIZE, wxSizeEventHandler(MyManager::OnSize));

  config->SetPath(OptionsGroup);
  config->Read(AudioDeviceKey, &m_options.m_AudioDevice);
  config->Read(VideoTimingKey, &m_options.m_VideoTiming);
  m_lastExportFile = config->Read(LastExportFileKey, wxT("*.avi"));

  config->SetPath(MappingsGroup);
  PwxString entryName;
  long entryIndex;
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      unsigned long pt;
      if (entryName.ToULong(&pt) && pt < RTP_DataFrame::IllegalPayloadType) {
        PwxString str;
        if (config->Read(entryName, &str)) {
          OpalMediaFormat mf = str.p_str();
          if (mf.IsTransportable())
            m_options.m_mappings[(RTP_DataFrame::PayloadTypes)pt] = mf;
        }
      }
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  // Show the frame window
  if (startMinimised)
    Iconize();
  Show(true);

  return true;
}


void MyManager::OnMenuOpen(wxMenuEvent & menuEvent)
{
  MyPlayer * player = dynamic_cast<MyPlayer *>(GetActiveChild());
  GetMenuBar()->Enable(ID_MenuExport, player != NULL && player->CanExport());
  wxMDIParentFrame::OnMenuOpen(menuEvent);
}


void MyManager::OnClose(wxCloseEvent & closeEvent)
{
  for (wxWindowList::const_iterator it = GetChildren().begin(); it != GetChildren().end(); ++it) {
    if (wxDynamicCast(*it, wxMDIChildFrame)) {
      if (!(*it)->Close()) {
        closeEvent.Veto();
        return;
      }
    }
  }

  ::wxBeginBusyCursor();

  wxProgressDialog progress(OpalSharkString, wxT("Exiting ..."));
  progress.Pulse();

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  if (!IsIconized()) {
    int x, y;
    GetPosition(&x, &y);
    config->Write(MainFrameXKey, x);
    config->Write(MainFrameYKey, y);

    int w, h;
    GetSize(&w, &h);
    config->Write(MainFrameWidthKey, w);
    config->Write(MainFrameHeightKey, h);
  }

  ShutDownEndpoints();

  Destroy();
}


void MyManager::OnMenuQuit(wxCommandEvent &)
{
  Close(true);
}


void MyManager::OnMenuAbout(wxCommandEvent &)
{
  PwxString text;
  PTime compiled(__DATE__);
  text << PRODUCT_NAME_TEXT " Version " << PProcess::Current().GetVersion() << "\n"
           "\n"
           "Copyright (c) " << COPYRIGHT_YEAR;
  if (compiled.GetYear() != COPYRIGHT_YEAR)
    text << '-' << compiled.GetYear();
  text << " " COPYRIGHT_HOLDER ", All rights reserved.\n"
          "\n"
          "This application may be used for any purpose so long as it is not sold "
          "or distributed for profit on it's own, or it's ownership by " COPYRIGHT_HOLDER
          " disguised or hidden in any way.\n"
          "\n"
          "Part of the Open Phone Abstraction Library, http://www.opalvoip.org\n"
          "  OPAL Version:  " << OpalGetVersion() << "\n"
          "  PTLib Version: " << PProcess::GetLibVersion() << '\n';
  wxMessageDialog dialog(this, text, wxT("About ..."), wxOK);
  dialog.ShowModal();
}


void MyManager::OnMenuOptions(wxCommandEvent &)
{
  PTRACE(4, "Opening options dialog");
  OptionsDialog dlg(this, m_options);
  if (dlg.ShowModal())
    m_options = dlg.GetOptions();
}


BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
END_EVENT_TABLE()

OptionsDialog::OptionsDialog(MyManager *manager, const MyOptions & options)
: m_manager(*manager)
, m_options(options)
{
  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("OptionsDialog"));

  m_screenAudioDevice = m_options.m_AudioDevice;
  m_screenAudioDevice.Replace(wxT("\t"), wxT(": "));

  wxComboBox * combo = NULL;
  FindWindowByNameAs(combo, this, AudioDeviceKey)->SetValidator(wxGenericValidator(&m_screenAudioDevice));

  PStringArray devices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  for (PINDEX i = 0; i < devices.GetSize(); i++) {
    PwxString str = devices[i];
    str.Replace(wxT("\t"), wxT(": "));
    combo->Append(str);
  }

  FindWindowByName(VideoTimingKey)->SetValidator(wxGenericValidator(&m_options.m_VideoTiming));

  FindWindowByNameAs(m_mappings, this, wxT("Mappings"));
  m_mappings->CreateGrid(m_options.m_mappings.size()+1, 2);
  m_mappings->SetColLabelValue(0, wxT("Payload Type"));
  m_mappings->SetColLabelValue(1, wxT("Media Format"));
  m_mappings->SetColLabelSize(wxGRID_AUTOSIZE);
  m_mappings->AutoSizeColLabelSize(0);
  m_mappings->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_TOP);
  m_mappings->HideRowLabels();
  RefreshMappings();
}

void OptionsDialog::RefreshMappings()
{
  wxArrayString formatNames = GetAllMediaFormatNames();

  int row = 0;
  for (OpalPCAPFile::PayloadMap::const_iterator it = m_options.m_mappings.begin(); it != m_options.m_mappings.end(); ++it,++row) {
    m_mappings->SetCellValue (row, 0, wxString() << it->first);
    m_mappings->SetCellEditor(row, 0, new wxGridCellNumberEditor(0, RTP_DataFrame::MaxPayloadType));
    m_mappings->SetCellValue (row, 1, wxString() << it->second);
    m_mappings->SetCellEditor(row, 1, new wxGridCellChoiceEditor(formatNames));
  }
  if (row >= m_mappings->GetNumberRows())
    m_mappings->AppendRows();
  m_mappings->SetCellEditor(row, 0, new wxGridCellNumberEditor(0, RTP_DataFrame::MaxPayloadType));
  m_mappings->SetCellEditor(row, 1, new wxGridCellChoiceEditor(formatNames));

  m_mappings->AutoSizeColumns();
  m_mappings->SetColSize(1, m_mappings->GetColSize(1)+40);
}


bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  m_options.m_AudioDevice = m_screenAudioDevice;
  m_options.m_AudioDevice.Replace(wxT(": "), wxT("\t"));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(OptionsGroup);
  config->Write(AudioDeviceKey, m_options.m_AudioDevice);
  config->Write(VideoTimingKey, m_options.m_VideoTiming);

  m_options.m_mappings.clear();
  config->DeleteGroup(MappingsGroup);
  config->SetPath(MappingsGroup);
  for (int row = 0; row < m_mappings->GetNumberRows(); ++row) {
    PwxString ptStr = m_mappings->GetCellValue(row, 0);
    PwxString mfStr = m_mappings->GetCellValue(row, 1);
    unsigned long pt;
    OpalMediaFormat mf;
    if (ptStr.ToULong(&pt) && pt < RTP_DataFrame::IllegalPayloadType && (mf = mfStr.p_str()).IsTransportable()) {
      m_options.m_mappings[(RTP_DataFrame::PayloadTypes)pt] = mf;
      config->Write(ptStr, mfStr);
    }
  }

  RefreshMappings();
  return true;
}


void MyManager::OnMenuOpenPCAP(wxCommandEvent &)
{
  wxFileDialog dlg(this,
                   wxT("Capture file to play"),
                   wxEmptyString,
                   wxEmptyString,
                   "Capture Files (*.pcap)|*.pcap|All Files (*.*)|*.*");
  if (dlg.ShowModal() == wxID_OK)
    Load(dlg.GetPath());
}


void MyManager::OnMenuCloseAll(wxCommandEvent &)
{
  for (wxWindowList::const_iterator it = GetChildren().begin(); it != GetChildren().end(); ++it) {
    if (wxDynamicCast(*it, wxMDIChildFrame))
      (*it)->Close();
  }
}


void MyManager::Load(wxString fname)
{
  new MyPlayer(this, PFilePath(PwxString(fname)));
}


///////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyPlayer, wxMDIChildFrame)
  EVT_CLOSE(MyPlayer::OnCloseWindow)

  EVT_MENU_OPEN(MyPlayer::OnMenuOpen)
  EVT_MENU_CLOSE(MyPlayer::OnMenuClose)
  EVT_MENU(wxID_CLOSE, MyPlayer::OnClose)

  EVT_GRID_CELL_CHANGED(MyPlayer::OnListChanged)

  EVT_BUTTON(ID_Play,    MyPlayer::OnPlay)
  EVT_BUTTON(ID_Stop,    MyPlayer::OnStop)
  EVT_BUTTON(ID_Pause,   MyPlayer::OnPause)
  EVT_BUTTON(ID_Resume,  MyPlayer::OnResume)
  EVT_BUTTON(ID_Step,    MyPlayer::OnStep)
  EVT_BUTTON(ID_Analyse, MyPlayer::OnAnalyse)
END_EVENT_TABLE()


MyPlayer::MyPlayer(MyManager * manager, const PFilePath & filepath)
  : wxMDIChildFrame(manager, wxID_ANY, PwxString(filepath.GetTitle()))
  , m_manager(*manager)
  , m_pcapFilePath(filepath)
  , m_backgroundThread(NULL)
  , m_packetCount(0)
  , m_playThreadCtrl(CtlIdle)
  , m_pausePacket(UINT_MAX)
{
  wxXmlResource::Get()->LoadPanel(this, wxT("PlayerPanel"));

  FindWindowByNameAs(m_rtpList, this, wxT("RTPList"));
  m_rtpList->CreateGrid(1, NumCols);
  for (int col = ColSrcIP; col < NumCols; ++col) {
    static wxChar const * const headings[] = {
      wxT("Src IP"),
      wxT("Src Port"),
      wxT("Dst IP"),
      wxT("Dst Port"),
      wxT("SSRC"),
      wxT("Type"),
      wxT("Format"),
      wxT("Play")
    };
    m_rtpList->SetColLabelValue(col, headings[col]);
  }
  m_rtpList->SetColLabelSize(wxGRID_AUTOSIZE);
  m_rtpList->AutoSizeColLabelSize(0);
  m_rtpList->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_TOP);
  m_rtpList->SetColFormatBool(ColPlay);
  m_rtpList->HideRowLabels();
  m_rtpList->AutoSizeColumns();

  FindWindowByNameAs(m_streamTabs,   this, wxT("StreamAnalysis"));
  FindWindowByNameAs(m_play,         this, wxT("Play"));
  FindWindowByNameAs(m_stop,         this, wxT("Stop"));
  FindWindowByNameAs(m_pause,        this, wxT("Pause"));
  FindWindowByNameAs(m_resume,       this, wxT("Resume"));
  FindWindowByNameAs(m_step,         this, wxT("Step"));
  FindWindowByNameAs(m_analyse,      this, wxT("Analyse"));
  FindWindowByNameAs(m_playToPacket, this, wxT("PlayToPacket"));

  m_play->Disable();
  m_stop->Disable();
  m_pause->Disable();
  m_resume->Disable();
  m_analyse->Disable();

  OpalPCAPFile * pcapFile = new OpalPCAPFile();
  if (pcapFile->Open(m_pcapFilePath, PFile::ReadOnly)) {
    pcapFile->SetPayloadMap(m_manager.GetOptions().m_mappings);
    m_progressDialog.reset(new wxProgressDialog(OpalSharkString,
                                                PwxString(PSTRSTRM("Loading \"" << m_pcapFilePath << '"')),
                                                1000,
                                                this,
                                                wxPD_CAN_ABORT|wxPD_AUTO_HIDE));
    m_backgroundThread = new PThreadObj1Arg<MyPlayer, OpalPCAPFile *>(*this, pcapFile, &MyPlayer::Discover, false, "Discover");
    Show(true);
  }
  else {
    delete pcapFile;
    wxMessageBox("Could not open PCAP file", OpalSharkErrorString, wxICON_EXCLAMATION | wxOK);
    Close();
  }
}


MyPlayer::~MyPlayer()
{
  PThread::WaitAndDelete(m_backgroundThread);

  m_playThreadCtrl = CtlStop;
  m_streamTabs->DeleteAllPages();
}


void MyPlayer::OnCloseWindow(wxCloseEvent& closeEvent)
{
  if (closeEvent.CanVeto() && wxMessageBox("Really close?", OpalSharkString, wxICON_QUESTION | wxYES_NO) != wxYES)
    closeEvent.Veto();
  else
    closeEvent.Skip();
}


void MyPlayer::OnClose(wxCommandEvent &)
{
  Close(true);
}


void MyPlayer::Discover(OpalPCAPFile * pcapFile)
{
  pcapFile->DiscoverRTP(m_discoveredRTP, PCREATE_NOTIFIER(DiscoverProgress));
  CallAfter(&MyPlayer::OnDiscoverComplete);
  delete pcapFile;
}


void MyPlayer::OnDiscoverComplete()
{
  if (m_progressDialog->WasCancelled()) {
    m_progressDialog.reset();
    return;
  }

  {
    OpalPCAPFile::DiscoveredRTPInfo * info = new OpalPCAPFile::DiscoveredRTPInfo;
    info->m_src.SetAddress(PIPSocket::GetDefaultIpAny(), 5000);
    info->m_dst.SetAddress(PIPSocket::GetDefaultIpAny(), 5000);
    info->m_payloadType = RTP_DataFrame::PCMU;
    info->m_mediaFormat = OpalG711uLaw;
    m_discoveredRTP.Append(info);
  }

  m_rtpList->AppendRows(m_discoveredRTP.size()-1, false);

  wxArrayString formatNames = GetAllMediaFormatNames();

  size_t row;
  for (row = 0; row < m_discoveredRTP.size(); ++row) {
    for (int col = ColSrcIP; col < NumCols; ++col) {
      m_rtpList->SetCellAlignment(row, col, wxALIGN_CENTRE, wxALIGN_TOP);
      if (row < m_discoveredRTP.size()-1 && col < ColFormat)
        m_rtpList->SetReadOnly(row, col);
    }

    m_rtpList->SetCellEditor(row, ColFormat, new wxGridCellChoiceEditor(formatNames));
    m_rtpList->SetCellEditor(row, ColPlay, new wxGridCellBoolEditor);

    const OpalPCAPFile::DiscoveredRTPInfo & info = m_discoveredRTP[row];
    m_rtpList->SetCellValue(row, ColSrcIP,       PwxString(PSTRSTRM(info.m_src.GetAddress())));
    m_rtpList->SetCellValue(row, ColSrcPort,     wxString() << info.m_src.GetPort());
    m_rtpList->SetCellValue(row, ColDstIP,       PwxString(PSTRSTRM(info.m_dst.GetAddress())));
    m_rtpList->SetCellValue(row, ColDstPort,     wxString() << info.m_dst.GetPort());
    m_rtpList->SetCellValue(row, ColSSRC,        wxString() << info.m_ssrc);
    m_rtpList->SetCellValue(row, ColPayloadType, PwxString(PSTRSTRM(info.m_payloadType)));
    m_rtpList->SetCellValue(row, ColFormat,      wxString() << info.m_mediaFormat);
    m_rtpList->SetCellValue(row, ColPlay,        row == 0 && m_discoveredRTP.size() == 2 ? GridTrueString : GridFalseString);
  }

  m_rtpList->AutoSizeColumns();
  m_rtpList->SetColSize(ColFormat, m_rtpList->GetColSize(ColFormat)+40);

  m_playToPacket->SetRange(1, m_packetCount+1);
  m_playToPacket->SetValue(m_packetCount+1);

  m_progressDialog.reset();
}


void  MyPlayer::DiscoverProgress(OpalPCAPFile &, OpalPCAPFile::Progress & progress)
{
  progress.m_abort = m_progressDialog->WasCancelled();
  m_progressDialog->Update(progress.m_filePosition*1000LL/progress.m_fileLength);
  m_packetCount = progress.m_packets;
}


void MyPlayer::OnListChanged(wxGridEvent & evt)
{
  PString value = PwxString(m_rtpList->GetCellValue(evt.GetRow(), evt.GetCol()));
  OpalPCAPFile::DiscoveredRTPInfo & info = m_discoveredRTP[evt.GetRow()];

  switch (evt.GetCol()) {
    case ColSrcIP :
      info.m_src.SetAddress(PIPAddress(value));
      break;
    case ColSrcPort :
      info.m_src.SetPort((WORD)value.AsUnsigned());
      break;
    case ColDstIP :
      info.m_dst.SetAddress(PIPAddress(value));
      break;
    case ColDstPort :
      info.m_dst.SetPort((WORD)value.AsUnsigned());
      break;
    case ColSSRC :
      info.m_ssrc = value.AsUnsigned();
      break;
    case ColPayloadType :
      info.m_payloadType = (RTP_DataFrame::PayloadTypes)value.AsUnsigned();
      break;
    case ColFormat :
      info.m_mediaFormat = value;
      break;
    case ColPlay:
      if (wxGridCellBoolEditor::IsTrueValue(PwxString(value)))
        m_selectedRows.insert(evt.GetRow());
      else
        m_selectedRows.erase(evt.GetRow());
  }

  bool enab = !m_selectedRows.empty();
  for (std::set<unsigned>::iterator it = m_selectedRows.begin(); it != m_selectedRows.end(); ++it) {
    OpalPCAPFile::DiscoveredRTPInfo & selected = m_discoveredRTP[*it];
    if (selected.m_payloadType >= RTP_DataFrame::IllegalPayloadType || !selected.m_mediaFormat.IsTransportable()) {
      enab = false;
      break;
    }
  }
  m_play->Enable(enab);
  m_step->Enable(enab);
  m_analyse->Enable(enab);
}


void MyPlayer::OnPlay(wxCommandEvent &)
{
  m_rtpList->Disable();
  m_play->Disable();
  m_stop->Enable();
  m_pause->Enable();
  m_resume->Disable();
  m_analyse->Disable();

  m_pausePacket = m_playToPacket->GetValue();

  StartPlaying(CtlRunning);
}


void MyPlayer::OnStop(wxCommandEvent &)
{
  m_playThreadCtrl = CtlStop;
}


void MyPlayer::OnPlayEnded()
{
  for (StreamBySSRC::iterator stream = m_streamsBySSRC.begin(); stream != m_streamsBySSRC.end(); ++stream) {
    if (stream->second->IsRunning())
      return;
  }

  m_playThreadCtrl = CtlIdle;

  m_rtpList->Enable();
  m_play->Enable();
  m_stop->Disable();
  m_pause->Disable();
  m_resume->Disable();
  m_analyse->Enable();

  for (StreamBySSRC::iterator stream = m_streamsBySSRC.begin(); stream != m_streamsBySSRC.end(); ++stream) {
    for (int i = 0; i < stream->second->GetColumnCount(); ++i)
      stream->second->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
  }
}


void MyPlayer::OnPause(wxCommandEvent &)
{
  m_playThreadCtrl = CtlPause;
  OnPaused();
}


void MyPlayer::OnPaused()
{
  m_pause->Disable();
  m_resume->Enable();
}


void MyPlayer::OnResume(wxCommandEvent &)
{
  m_playThreadCtrl = CtlRunning;
  m_pause->Enable();
  m_resume->Disable();
}


void MyPlayer::OnStep(wxCommandEvent &)
{
  StartPlaying(CtlStep);
}


void MyPlayer::OnAnalyse(wxCommandEvent &)
{
  m_streamsBySSRC.clear();
  m_streamTabs->DeleteAllPages();

  OpalPCAPFile pcapFile;
  if (!pcapFile.Open(m_pcapFilePath, PFile::ReadOnly))
    return;

  for (std::set<unsigned>::iterator selectedRow = m_selectedRows.begin(); selectedRow != m_selectedRows.end(); ++selectedRow) {
    OpalPCAPFile::DiscoveredRTPInfo & selectedInfo = m_discoveredRTP[*selectedRow];

    MediaStream * stream = new MediaStream(*this, selectedInfo);
    m_streamTabs->AddPage(stream, wxString() << selectedInfo.m_ssrc);
    m_streamsBySSRC[selectedInfo.m_ssrc] = stream;
  }

  off_t fileLength = pcapFile.GetLength();
  wxProgressDialog progress(OpalSharkString,
                            PwxString(PSTRSTRM("Analysing " << m_pcapFilePath)),
                            1000,
                            this,
                            wxPD_CAN_ABORT|wxPD_AUTO_HIDE);

  unsigned packetNumber = 0;
  RTP_DataFrame dummyDecoded;
  while (!pcapFile.IsEndOfFile() && progress.Update(pcapFile.GetPosition()*1000LL/fileLength)) {
    ++packetNumber;

    RTP_DataFrame rtp;
    if (pcapFile.GetRTP(rtp) < 0)
      continue;

    StreamBySSRC::iterator stream = m_streamsBySSRC.find(rtp.GetSyncSource());
    if (stream != m_streamsBySSRC.end())
      stream->second->Analyse(packetNumber, rtp, dummyDecoded, pcapFile.GetPacketTime());
  }

  for (StreamBySSRC::iterator stream = m_streamsBySSRC.begin(); stream != m_streamsBySSRC.end(); ++stream) {
    for (int i = 0; i < stream->second->GetColumnCount(); ++i)
      stream->second->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
  }
}


void MyPlayer::StartPlaying(Controls ctrl)
{
  if (m_playThreadCtrl == CtlIdle) {
    m_streamsBySSRC.clear();
    m_streamTabs->DeleteAllPages();
  }

  m_playThreadCtrl = ctrl;
  for (std::set<unsigned>::iterator selectedRow = m_selectedRows.begin(); selectedRow != m_selectedRows.end(); ++selectedRow) {
    OpalPCAPFile::DiscoveredRTPInfo & selectedInfo = m_discoveredRTP[*selectedRow];
    if (m_streamsBySSRC.find(selectedInfo.m_ssrc) == m_streamsBySSRC.end()) {
      MediaStream * stream = new MediaStream(*this, selectedInfo);
      m_streamTabs->AddPage(stream, wxString() << selectedInfo.m_ssrc);
      m_streamsBySSRC[selectedInfo.m_ssrc] = stream;
    }
  }

  for (StreamBySSRC::iterator stream = m_streamsBySSRC.begin(); stream != m_streamsBySSRC.end(); ++stream)
    stream->second->StartThread();
}


///////////////////////////////////////////////////////////////////////////////

MediaStream::MediaStream(MyPlayer & player, const OpalPCAPFile::DiscoveredRTPInfo & filterInfo)
  : wxListCtrl(player.m_streamTabs, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT)
  , m_player(player)
  , m_clockRate(filterInfo.m_mediaFormat.GetClockRate())
  , m_firstPacket(true)
  , m_firstTime(0)
  , m_lastTime(0)
  , m_firstTimestamp(0)
  , m_lastSequenceNumber(0)
  , m_lastPacketTimestamp(0)
  , m_lastFrameEnd(false)
  , m_lastFrameTimestamp(0)
  , m_audioFormat(filterInfo.m_mediaFormat)
  , m_videoFormat(filterInfo.m_mediaFormat)
  , m_thread(NULL)
  , m_running(true)
  , m_filterInfo(filterInfo)
{
  AppendColumn("#");
  AppendColumn("Time");
  AppendColumn("Delta (ms)",  wxLIST_FORMAT_RIGHT);
  AppendColumn("Sequence",    wxLIST_FORMAT_RIGHT);
  AppendColumn("Timestamp",   wxLIST_FORMAT_RIGHT);
  AppendColumn("Delta",       wxLIST_FORMAT_RIGHT);
  AppendColumn("Jitter (ms)", wxLIST_FORMAT_RIGHT);
  AppendColumn("Notes");
}


MediaStream::~MediaStream()
{
  PThread::WaitAndDelete(m_thread);
}


void MediaStream::StartThread()
{
  if (m_thread != NULL)
    return;

  if (m_audioFormat.IsValid())
    m_thread = new PThreadObj<MediaStream>(*this, &MediaStream::PlayAudio, false, "AudioPlayer");
  else
    m_thread = new PThreadObj<MediaStream>(*this, &MediaStream::PlayVideo, false, "VideoPlayer");
}


void MediaStream::Analyse(unsigned packetNumber,
                          const RTP_DataFrame & encoded,
                          const RTP_DataFrame & decoded,
                          const PTime & thisTime,
                          OpalVideoFormat::FrameType videoFrameType)
{
  RTP_SequenceNumber thisSequenceNumber = encoded.GetSequenceNumber();
  RTP_Timestamp thisTimestamp = encoded.GetTimestamp();
  wxString deltaMS, deltaTS, jitter, notes;

  bool frameEnd = encoded.GetMarker() || m_audioFormat.IsValid();

  if (m_firstPacket) {
    m_firstPacket = false;
    m_firstTime = thisTime;
    m_firstTimestamp = m_lastPacketTimestamp = m_lastFrameTimestamp = thisTimestamp;
    notes << "Clock: " << m_clockRate << "Hz";
  }
  else {
    if (frameEnd) {
      deltaMS << (thisTime - m_lastTime).GetMilliSeconds();
      deltaTS << ((int64_t)thisTimestamp - (int64_t)m_lastFrameTimestamp);
      m_lastFrameTimestamp = thisTimestamp;
      int64_t usJit = (thisTime - (m_firstTime + (thisTimestamp - m_firstTimestamp) * 1000LL / m_clockRate)).GetMicroSeconds();
      if (usJit < -100) {
        usJit = -usJit;
        jitter << '-';
      }
      jitter << usJit / 1000 << '.' << (usJit % 1000) / 100;
    }

    if (m_lastSequenceNumber != UINT_MAX && thisSequenceNumber != (m_lastSequenceNumber + 1))
      notes << "Out of sequence";

    if (!m_lastFrameEnd && m_lastPacketTimestamp != thisTimestamp) {
      if (!notes.empty())
        notes << ", ";
      notes << "Unexpected timestamp change";
      if (!deltaTS.empty())
        deltaTS << ',';
      deltaTS << ((int64_t)thisTimestamp - (int64_t)m_lastPacketTimestamp);
    }
  }

  if (m_audioFormat.IsValid()) {
    if (!decoded.IsEmpty()) {
      if (!notes.empty())
        notes << ", ";
      notes << "dBov="
        << m_dbCalculator.Accumulate(decoded.GetPayloadPtr(), decoded.GetPayloadSize()).Finalise();
    }

    if (m_audioFormat.GetFrameType(encoded.GetPayloadPtr(),
                                   encoded.GetPayloadSize(),
                                   m_audioFrameDetector) & OpalAudioFormat::e_SilenceFrame) {
      if (!notes.empty())
        notes << ", ";
      notes << "Silent ";
    }
  }

  if (m_videoFormat.IsValid()) {
    switch (m_videoFormat.GetFrameType(encoded.GetPayloadPtr(), encoded.GetPayloadSize(), m_videoFrameDetector)) {
      case OpalVideoFormat::e_IntraFrame:
        if (!notes.empty())
          notes << ", ";
        notes << "I-Frame";
        break;

      case OpalVideoFormat::e_InterFrame:
        if (!notes.empty())
          notes << ", ";
        notes << "P-Frame";
        break;

      default:
        break;
    }

    switch (videoFrameType) {
      case OpalVideoFormat::e_IntraFrame:
        if (!notes.empty())
          notes << ", ";
        notes << "Decoded Key Frame";
        break;

      case OpalVideoFormat::e_InterFrame:
        if (!notes.empty())
          notes << ", ";
        notes << "Decoded Frame";
        break;

      default:
        break;
    }
  }

  m_lastTime = thisTime;
  m_lastSequenceNumber = thisSequenceNumber;
  m_lastPacketTimestamp = thisTimestamp;
  m_lastFrameEnd = frameEnd;

  wxString info;
  info << packetNumber << '\n'
    << PwxString(thisTime.AsString("hh:mm:ss.uuu")) << '\n'
    << deltaMS << '\n'
    << thisSequenceNumber << '\n'
    << thisTimestamp << '\n'
    << deltaTS << '\n'
    << jitter << '\n'
    << notes;

  CallAfter<MediaStream, wxString, bool, wxString, bool>(&MediaStream::OnAnalysisUpdate, info, true);
}


void MediaStream::OnAnalysisUpdate(wxString info, bool async)
{
  wxStringTokenizer parser(info, wxT("\n"), wxTOKEN_RET_EMPTY_ALL);

  long pos = InsertItem(INT_MAX, parser.GetNextToken());
  for (int i = 1; i < GetColumnCount(); ++i)
    SetItem(pos, i, parser.GetNextToken());

  if (async) {
    if (pos % 1000 == 0) {
      for (int i = 0; i < GetColumnCount(); ++i)
        SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
    }
    if (pos % 10 == 0)
      EnsureVisible(pos);
  }
}


void MediaStream::PlayAudio()
{
  PTRACE(3, "Started audio player thread.");

  OpalPCAPFile pcapFile;
  if (pcapFile.Open(m_player.m_pcapFilePath, PFile::ReadOnly)) {
    pcapFile.SetFilters(m_filterInfo);

    unsigned packetNumber = 0;
    PSoundChannel * soundChannel = NULL;
    OpalPCAPFile::DecodeContext decodeContext;
    while (m_player.m_playThreadCtrl != MyPlayer::CtlStop && !pcapFile.IsEndOfFile()) {
      while (m_player.m_playThreadCtrl == MyPlayer::CtlPause) {
        PThread::Sleep(200);
      }

      ++packetNumber;

      RTP_DataFrame encodedRTP;
      if (pcapFile.GetRTP(encodedRTP) < 0)
        continue;

      RTP_DataFrame decodedAudio;
      if (pcapFile.DecodeRTP(encodedRTP, decodedAudio, decodeContext) <= 0)
        continue;

      Analyse(packetNumber, encodedRTP, decodedAudio, pcapFile.GetPacketTime());

      if (soundChannel == NULL) {
        OpalMediaFormat format = decodeContext.m_transcoder->GetOutputFormat();
        soundChannel = new PSoundChannel(m_player.m_manager.GetOptions().m_AudioDevice,
                                         PSoundChannel::Player,
                                         format.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1),
                                         format.GetClockRate());
        soundChannel->SetBuffers(decodedAudio.GetPayloadSize(), 8);
      }

      if (!soundChannel->Write(decodedAudio.GetPayloadPtr(), decodedAudio.GetPayloadSize()))
        break;
    }

    delete soundChannel;
  }

  m_running = false;
  m_player.CallAfter(&MyPlayer::OnPlayEnded);
  PTRACE(3, "Ended audio player thread.");
}


void MediaStream::PlayVideo()
{
  PTRACE(3, "Started video player thread.");

  OpalPCAPFile pcapFile;
  if (pcapFile.Open(m_player.m_pcapFilePath, PFile::ReadOnly)) {
    pcapFile.SetFilters(m_filterInfo);

    PVideoOutputDevice::OpenArgs args;
    args.driverName = P_WXWINDOWS_DRIVER_NAME;
    args.deviceName = PSTRSTRM(P_WXWINDOWS_DEVICE_NAME " TITLE=\"" << m_filterInfo << '"');
    args.colourFormat = PVideoFrameInfo::YUV420P();
    PVideoOutputDevice * videoOutput = PVideoOutputDevice::CreateOpenedDevice(args);

    unsigned packetNumber = 0;
    PTime realStartTime;
    PTime fileStartTime(0);
    RTP_Timestamp startTimestamp = 0;

    OpalPCAPFile::DecodeContext decodeContext;
    while (m_player.m_playThreadCtrl != MyPlayer::CtlStop && !pcapFile.IsEndOfFile()) {
      while (m_player.m_playThreadCtrl == MyPlayer::CtlPause) {
        PThread::Sleep(200);
        realStartTime.SetCurrentTime();
        fileStartTime.SetTimestamp(0);
        startTimestamp = 0;
      }

      ++packetNumber;

      RTP_DataFrame encodedRTP;
      if (pcapFile.GetRTP(encodedRTP) < 0)
        continue;

      RTP_DataFrame decodedVideoFrame;
      switch (pcapFile.DecodeRTP(encodedRTP, decodedVideoFrame, decodeContext)) {
        case 1: // Decoded video frame
          break;
        case 0:
          Analyse(packetNumber, encodedRTP, decodedVideoFrame, pcapFile.GetPacketTime());
        default:
          continue;
      }

      Analyse(packetNumber, encodedRTP, decodedVideoFrame, pcapFile.GetPacketTime(),
              dynamic_cast<OpalVideoTranscoder *>(decodeContext.m_transcoder)->WasLastFrameIFrame()
                       ? OpalVideoFormat::e_IntraFrame : OpalVideoFormat::e_InterFrame);
      PTRACE(4, "Decoded " << setw(1) << decodedVideoFrame);

      PTimeInterval delay;
      if (m_player.m_manager.GetOptions().m_VideoTiming == 0) {
        if (fileStartTime.IsValid())
          delay = pcapFile.GetPacketTime() - fileStartTime - realStartTime.GetElapsed();
        else
          fileStartTime = pcapFile.GetPacketTime();
      }
      else {
        if (startTimestamp != 0)
          delay = (decodedVideoFrame.GetTimestamp() - startTimestamp)/90 - realStartTime.GetElapsed();
        else
          startTimestamp = decodedVideoFrame.GetTimestamp();
      }
      if (delay > 0)
        PThread::Sleep(delay);

      const OpalVideoTranscoder::FrameHeader* header = (const OpalVideoTranscoder::FrameHeader*)decodedVideoFrame.GetPayloadPtr();

      videoOutput->SetFrameSize(header->width, header->height);

      PVideoOutputDevice::FrameData frameData;
      frameData.x = header->x;
      frameData.y = header->y;
      frameData.width = header->width;
      frameData.height = header->height;
      frameData.pixels = OPAL_VIDEO_FRAME_DATA_PTR(header);
      videoOutput->SetFrameData(frameData);

      if (packetNumber >= m_player.m_pausePacket || m_player.m_playThreadCtrl == MyPlayer::CtlStep) {
        m_player.m_playThreadCtrl = MyPlayer::CtlPause;
        m_player.CallAfter(&MyPlayer::OnPaused);
      }
    }

    videoOutput->Close();
    // No need to delete, it is done via wx
  }

  m_running = false;
  m_player.CallAfter(&MyPlayer::OnPlayEnded);
  PTRACE(3, "Ended video player thread.");
}


////////////////////////////////////////////////////////////////////////////////

void MyManager::OnMenuExport(wxCommandEvent&)
{
  MyPlayer * child = dynamic_cast<MyPlayer *>(GetActiveChild());
  if (child == NULL)
    return;

  bool addBar = false;
  wxString wildcard;
  PString lastExt = PFilePath(m_lastExportFile).GetType();
  OpalRecordManager::Factory::KeyList_T extList = OpalRecordManager::Factory::GetKeyList();
  OpalRecordManager::Factory::KeyList_T::iterator it = std::find(extList.begin(), extList.end(), lastExt);
  if (it != extList.end()) {
    PwxString ext = it->Mid(1);
    wildcard << ext.Upper() << " Files (*." << ext << ")|*." << ext;
    addBar = true;
  }
  for (it = extList.begin(); it != extList.end(); ++it) {
    if (*it == lastExt)
      continue;

    if (addBar)
      wildcard << '|';
    else
      addBar = true;

    PwxString ext = it->Mid(1);
    wildcard << ext.Upper() << " Files (*." << ext << ")|*." << ext;
  }
  wxFileDialog dlg(this,
                   wxT("Media file for export"),
                   wxEmptyString,
                   m_lastExportFile,
                   wildcard,
                   wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK)
    return;

  m_lastExportFile = dlg.GetPath();
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(OptionsGroup);
  config->Write(LastExportFileKey, m_lastExportFile);
  child->StartExport(m_lastExportFile);
}


bool MyPlayer::CanExport() const
{
  return !m_selectedRows.empty();
}


void MyPlayer::StartExport(const PFilePath & mediaFile)
{
  OpalRecordManager * recorder = OpalRecordManager::Factory::CreateInstance(mediaFile.GetType());
  if (recorder == NULL) {
    wxMessageBox("Cannot record to file type", OpalSharkErrorString, wxICON_EXCLAMATION | wxOK);
    return;
  }

  unsigned audioCount = 0;
  for (std::set<unsigned>::iterator selectedRow = m_selectedRows.begin(); selectedRow != m_selectedRows.end(); ++selectedRow) {
    if (m_discoveredRTP[*selectedRow].m_mediaFormat.GetMediaType() == OpalMediaType::Audio())
      ++audioCount;
  }

  OpalRecordManager::Options options;
  options.m_stereo = audioCount == 2;
  options.m_videoMixing = OpalRecordManager::eSideBySideLetterbox;
  options.m_pushThreads = false;
  PVideoFrameInfo::ParseSize("HD720", options.m_videoWidth, options.m_videoHeight);
  if (!recorder->Open(mediaFile, options)) {
    delete recorder;
    wxMessageBox("Could not open media file", OpalSharkErrorString, wxICON_EXCLAMATION | wxOK);
    return;
  }

  OpalPCAPFile * pcapFile = new OpalPCAPFile();
  if (pcapFile->Open(m_pcapFilePath, PFile::ReadOnly)) {
    m_progressDialog.reset(new wxProgressDialog(OpalSharkString,
                                                PwxString(PSTRSTRM("Exporting \"" << m_pcapFilePath << "\" to \"" << mediaFile << '"')),
                                                1000,
                                                this,
                                                wxPD_CAN_ABORT|wxPD_AUTO_HIDE));
    m_backgroundThread = new PThreadObj2Arg<MyPlayer, OpalPCAPFile *, OpalRecordManager *>(*this, pcapFile, recorder, &MyPlayer::Export, false, "Export");
  }
  else {
    delete pcapFile;
    wxMessageBox("Could not open PCAP file", OpalSharkErrorString, wxICON_EXCLAMATION | wxOK);
  }
}


void MyPlayer::OnExportComplete()
{
  PTRACE(4, "Export completed");
  m_progressDialog.reset();
}


void MyPlayer::Export(OpalPCAPFile * pcapFile, OpalRecordManager * recorder)
{
  PTRACE(3, "Starting export to " << pcapFile->GetName());

  std::map<OpalPCAPFile::DiscoveredRTPKey, OpalPCAPFile::DecodeContext> decodeContext;
  OpalPCAPFile::PayloadMap payloadMap;

  for (std::set<unsigned>::iterator selectedRow = m_selectedRows.begin(); selectedRow != m_selectedRows.end(); ++selectedRow) {
    OpalPCAPFile::DiscoveredRTPInfo & selectedInfo = m_discoveredRTP[*selectedRow];

    OpalMediaFormat raw;
    if (selectedInfo.m_mediaFormat.GetMediaType() == OpalMediaType::Audio())
      raw = GetOpalPCM16(selectedInfo.m_mediaFormat.GetClockRate(), 1);
    else if (selectedInfo.m_mediaFormat.GetMediaType() == OpalMediaType::Video())
      raw = OpalYUV420P;
    else
      continue;

    OpalPCAPFile::DiscoveredRTPKey key = selectedInfo;
    if (recorder->OpenStream(PSTRSTRM(key), raw)) {
      decodeContext.insert(make_pair(selectedInfo, OpalPCAPFile::DecodeContext()));
      payloadMap[selectedInfo.m_payloadType] = selectedInfo.m_mediaFormat;
      PTRACE(4, "Added stream " << key << " with " << selectedInfo << " to " << pcapFile->GetName());
    }
  }

  pcapFile->SetPayloadMap(payloadMap);

  off_t fileLength = pcapFile->GetLength();
  while (!pcapFile->IsEndOfFile() && !m_progressDialog->WasCancelled()) {
    m_progressDialog->Update(pcapFile->GetPosition()*1000LL/fileLength);

    RTP_DataFrame rtp;
    if (pcapFile->GetRTP(rtp) < 0)
      continue;

    OpalPCAPFile::DiscoveredRTPKey rtpKey;
    rtpKey.m_src.SetAddress(pcapFile->GetSrcIP(), pcapFile->GetSrcPort());
    rtpKey.m_dst.SetAddress(pcapFile->GetDstIP(), pcapFile->GetDstPort());
    rtpKey.m_ssrc = rtp.GetSyncSource();
    std::map<OpalPCAPFile::DiscoveredRTPKey, OpalPCAPFile::DecodeContext>::iterator itContext = decodeContext.find(rtpKey);
    if (itContext == decodeContext.end())
      continue;

    RTP_DataFrame decoded;
    if (pcapFile->DecodeRTP(rtp, decoded, itContext->second) > 0) {
      recorder->WriteStream(PSTRSTRM(itContext->first), decoded);
      recorder->OnPushMedia(pcapFile->GetPacketTime());
    }
  }

  PTRACE(3, "Ending export to " << pcapFile->GetName());
  delete pcapFile;
  delete recorder;
  CallAfter(&MyPlayer::OnExportComplete);
}


// End of File ///////////////////////////////////////////////////////////////
