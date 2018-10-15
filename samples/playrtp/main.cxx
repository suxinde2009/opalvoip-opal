/*
 * main.cxx
 *
 * OPAL application source file for playing RTP from a PCAP file
 *
 * Main program entry point.
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include "precompile.h"
#include "main.h"

#include <rtp/pcapfile.h>
#include <ptlib/vconvert.h>
#include <ptlib/pipechan.h>


PCREATE_PROCESS(PlayRTP);


const int g_extraHeight = 35;


///////////////////////////////////////////////////////////////////////

PlayRTP::PlayRTP()
  : PProcess("OPAL Audio/Video Codec Tester", "PlayRTP", 1, 0, ReleaseCode, 0)
  , m_transcoder(NULL)
  , m_player(NULL)
#if OPAL_VIDEO
  , m_display(NULL)
#endif
{
}


PlayRTP::~PlayRTP()
{
  delete m_transcoder;
  delete m_player;
}


void PlayRTP::Main()
{
  PArgList & args = GetArguments();

  if (!args.Parse("[Options:]"
             "m-mapping: Set mapping of payload type to format, eg 101=H.264\n"
             "S-src-ip: Source IP address, default is any\n"
             "D-dst-ip: Destination IP address, default is any\n"
             "s-src-port: Source UDP port, default is any\n"
             "d-dst-port: Destination UDP port, default is any\n"
             "A-audio-driver: Audio player driver.\n"
             "a-audio-device: Audio player device.\n"
#if OPAL_VIDEO
             "V-video-driver: Video display driver to use.\n"
             "v-video-device: Video display device to use.\n"
             "Y-video-file: write decoded video to file\n"
#endif
             "p-singlestep. Single step through input data.\n"
             "P-payload-file: write RTP payload to file\n"
             "i-info. Display per-frame information.\n"
             "f-find. find and display list of RTP sessions.\n"
             "E: write event log to file\n"
             "T: put text in extra video information\n"
             "X. enable extra video information\n"
             "O: Set codec option (may be used multiple times)\r"
                 "   fmt is name of codec, eg \"H.261\"\r"
                 "   opt is name of option, eg \"Target Bit Rate\"\r"
                 "   val is value of option, eg \"48000\"\n"
             "-session: automatically select session num\n"
             "-rotate: Rotate on RTP header extension N\n"
             "-nodelay. do not delay as per timestamps\n"
             "j-jitter-limit: set jitter limit\n"
             PTRACE_ARGLIST
             "h-help. print this help message.\n"
             , false) || args.HasOption('h')) {
    args.Usage(cerr, "[ options ] filename [ filename ... ]") << "\n"
               "e.g. " << GetFile().GetTitle() << " conversation.pcap\n\n";
    return;
  }

  PTRACE_INITIALISE(args);

  m_extendedInfo = args.HasOption('X') || args.HasOption('T');
  m_rotateExtensionId = args.GetOptionString("rotate").AsUnsigned();
  m_noDelay      = args.HasOption("nodelay");
  m_jitterLimit.SetInterval(args.GetOptionString("jitter-limit", "50").AsUnsigned());

  PStringArray options = args.GetOptionString('O').Lines();
  for (PINDEX i = 0; i < options.GetSize(); i++) {
    const PString & optionDescription = options[i];
    PINDEX colon = optionDescription.Find(':');
    PINDEX equal = optionDescription.Find('=', colon+2);
    if (colon == P_MAX_INDEX || equal == P_MAX_INDEX) {
      cerr << "Invalid option description \"" << optionDescription << '"' << endl;
      continue;
    }
    OpalMediaFormat mediaFormat = optionDescription.Left(colon);
    if (mediaFormat.IsEmpty()) {
      cerr << "Invalid media format in option description \"" << optionDescription << '"' << endl;
      continue;
    }
    PString optionName = optionDescription(colon+1, equal-1);
    if (!mediaFormat.HasOption(optionName)) {
      cerr << "Invalid option name in description \"" << optionDescription << '"' << endl;
      continue;
    }
    PString valueStr = optionDescription.Mid(equal+1);
    if (!mediaFormat.SetOptionValue(optionName, valueStr)) {
      cerr << "Invalid option value in description \"" << optionDescription << '"' << endl;
      continue;
    }
    OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
    cout << "Set option \"" << optionName << "\" to \"" << valueStr << "\" in \"" << mediaFormat << '"' << endl;
  }

  OpalPCAPFile pcap;
  if (!pcap.Open(args[0], PFile::ReadOnly)) {
    cerr << "Could not open file \"" << args[0] << "\"\n";
    return;
  }

  if (args.HasOption('f')) {
    OpalPCAPFile::DiscoveredRTP discoveredRTP;
    if (!pcap.DiscoverRTP(discoveredRTP)) {
      cerr << "No RTP sessions found" << endl;
      return;
    }
    cout << "Found " << discoveredRTP.size() << " sessions:\n" << discoveredRTP << endl;
    return;
  }

  PStringArray mappings = args.GetOptionString('m').Lines();
  for (PINDEX i = 0; i < mappings.GetSize(); i++) {
    const PString & mapping = mappings[i];
    PINDEX equal = mapping.Find('=');
    if (equal == P_MAX_INDEX) {
      cout << "Invalid syntax for mapping \"" << mapping << '"' << endl;
      continue;
    }

    RTP_DataFrame::PayloadTypes pt = (RTP_DataFrame::PayloadTypes)mapping.Left(equal).AsUnsigned();
    if (pt > RTP_DataFrame::MaxPayloadType) {
      cout << "Invalid payload type for mapping \"" << mapping << '"' << endl;
      continue;
    }

    OpalMediaFormat mf = mapping.Mid(equal+1);
    if (!mf.IsTransportable()) {
      cout << "Invalid media format for mapping \"" << mapping << '"' << endl;
      continue;
    }

    pcap.SetPayloadMap(pt, mf);
  }

  if (args.HasOption('S') || args.HasOption('D') || args.HasOption('s') || args.HasOption('d')) {
    pcap.SetFilterSrcIP(PIPSocket::Address(args.GetOptionString('S', PIPSocket::GetDefaultIpAny().AsString())));
    pcap.SetFilterDstIP(PIPSocket::Address(args.GetOptionString('D', PIPSocket::GetDefaultIpAny().AsString())));
    pcap.SetFilterSrcPort(PIPSocket::GetPortByService("udp", args.GetOptionString('s')));
    pcap.SetFilterDstPort(PIPSocket::GetPortByService("udp", args.GetOptionString('d', "5000")));
  }
  else {
    cout << "Scanning file for RTP sessions." << endl;
    OpalPCAPFile::DiscoveredRTP discoveredRTP;
    if (!pcap.DiscoverRTP(discoveredRTP)) {
      cerr << "No RTP sessions found - please use -S/-D/-s/-d option to specify session manually" << endl;
      return;
    }

    if (args.HasOption("session")) {
      size_t idx = args.GetOptionString("session").AsUnsigned();
      if (idx == 0 || idx > discoveredRTP.size() || !pcap.SetFilters(discoveredRTP[idx-1])) {
        cout << "Preselected session " << args.GetOptionString("session") << " not valid" << endl;
        return;
      }
    }
    else if (discoveredRTP.size() == 1 && discoveredRTP[0].m_mediaFormat.IsValid())
      pcap.SetFilters(discoveredRTP[0]);
    else {
      cout << "Select one of the following sessions (index [ format ]):\n";
      for (size_t i = 0; i < discoveredRTP.size(); ++i)
        cout << (i + 1) << ' ' << discoveredRTP[i] << endl;
      for (;;) {
        cout << "Select (1-" << discoveredRTP.size()*2 << ") ? " << flush;
        PString line;
        cin >> line;
        size_t idx = line.AsUnsigned();
        if (idx > 0 && idx <= discoveredRTP.size() && pcap.SetFilters(discoveredRTP[idx-1], line.Mid(line.Find(' ')).Trim()))
          break;
        cout << "Session/format " << line << " is not valid" << endl;
      }
    }
  }

  if (args.HasOption('E')) {
    if (!m_eventLog.Open(args.GetOptionString('E'), PFile::WriteOnly)) {
      cerr << "cannot open event log file '" << args.GetOptionString('E') << "'" << endl;
      return;
    }
    m_eventLog << "Event log created " << PTime().AsString() << " from " << args[0] << endl;
  }

  m_singleStep = args.HasOption('p');
  m_info       = args.GetOptionCount('i');

  if (args.HasOption('P')) {
    if (!m_payloadFile.Open(args.GetOptionString('P'), PFile::ReadWrite)) {
      cerr << "Cannot create \"" << m_payloadFile.GetFilePath() << '\"' << endl;
      return;
    }
    cout << "Dumping RTP payload to \"" << m_payloadFile.GetFilePath() << '\"' << endl;
  }

#if OPAL_VIDEO
  if (args.HasOption('Y')) {
    PFilePath yuvFileName = args.GetOptionString('Y');
    m_extraText = yuvFileName.GetFileName();

    if (yuvFileName.GetType() != ".yuv") {
      m_encodedFileName = yuvFileName;
      yuvFileName += ".yuv";
    }

    if (!m_yuvFile.Open(yuvFileName, PFile::ReadWrite)) {
      cerr << "Cannot create '" << yuvFileName << "'" << endl;
      return;
    }
    cout << "Writing video to \"" << yuvFileName << '\"' << endl;
  }

  if (m_extendedInfo) {
    if (args.HasOption('T'))
      m_extraText = args.GetOptionString('T').Trim();
    m_extraHeight = g_extraHeight + ((m_extraText.GetLength() == 0) ? 0 : 17);
  }
#endif

  // Audio player
  {
    PString driverName = args.GetOptionString('A');
    PString deviceName = args.GetOptionString('a');
    m_player = PSoundChannel::CreateOpenedChannel(driverName, deviceName, PSoundChannel::Player);
    if (m_player == NULL) {
      PStringList devices = PSoundChannel::GetDriversDeviceNames("*", PSoundChannel::Player);
      if (devices.IsEmpty()) {
        cerr << "No audio devices in the system!" << endl;
        return;
      }

      if (!driverName.IsEmpty() || !deviceName.IsEmpty()) {
        cerr << "Cannot use ";
        if (driverName.IsEmpty() && deviceName.IsEmpty())
          cerr << "default ";
        cerr << "audio player";
        if (!driverName.IsEmpty())
          cerr << ", driver \"" << driverName << '"';
        if (!deviceName.IsEmpty())
          cerr << ", device \"" << deviceName << '"';
        cerr << ", must be one of:\n";
        for (PINDEX i = 0; i < devices.GetSize(); i++)
          cerr << "   " << devices[i] << '\n';
        cerr << endl;
        return;
      }

      PStringList::iterator it = devices.begin();
      while ((m_player = PSoundChannel::CreateOpenedChannel(driverName, *it, PSoundChannel::Player)) == NULL) {
        cerr << "Cannot use audio device \"" << *it << '"' << endl;
        if (++it == devices.end()) {
          cerr << "Unable to find an available sound device." << endl;
          return;
        }
      }
    }

    cout << "Audio Player ";
    if (!driverName.IsEmpty())
      cout << "driver \"" << driverName << "\" and ";
    cout << "device \"" << m_player->GetName() << "\" opened." << endl;
  }

#if OPAL_VIDEO
  // Video display
  PString driverName = args.GetOptionString('V');
  PString deviceName = args.GetOptionString('v');
  m_display = PVideoOutputDevice::CreateOpenedDevice(driverName, deviceName, FALSE);
  if (m_display == NULL) {
    cerr << "Cannot use ";
    if (driverName.IsEmpty() && deviceName.IsEmpty())
      cerr << "default ";
    cerr << "video display";
    if (!driverName.IsEmpty())
      cerr << ", driver \"" << driverName << '"';
    if (!deviceName.IsEmpty())
      cerr << ", device \"" << deviceName << '"';
    cerr << ", must be one of:\n";
    PStringList devices = PVideoOutputDevice::GetDriversDeviceNames("*");
    for (PINDEX i = 0; i < devices.GetSize(); i++)
      cerr << "   " << devices[i] << '\n';
    cerr << endl;
    return;
  }

  m_display->SetColourFormatConverter(OpalYUV420P);

  cout << "Display ";
  if (!driverName.IsEmpty())
    cout << "driver \"" << driverName << "\" and ";
  cout << "device \"" << m_display->GetDeviceName() << "\" opened." << endl;
#endif

  Play(pcap);

  for (PINDEX i = 1; i < args.GetCount(); i++) {
    if (pcap.Open(args[i], PFile::ReadOnly))
      Play(pcap);
  }
}


#if OPAL_VIDEO
static void DrawText(unsigned x, unsigned y, unsigned frameWidth, unsigned frameHeight, BYTE * frame, const char * text)
{
  BYTE * output  = frame + ((y * frameWidth) + x);
  unsigned uoffs = (frameWidth * frameHeight) / 4;

  while (*text != '\0') {
    const PVideoFont::LetterData * letter = PVideoFont::GetLetterData(*text++);
    if (letter != NULL) {
      for (PINDEX row = 0; row < PVideoFont::MAX_L_HEIGHT; ++row) {
        BYTE * rowPtr = output + row * frameWidth;
        const char * line = letter->line[row];
        {
          int UVwidth = (strlen(line)+2)/2;
          memset(rowPtr + uoffs*2, 0x80, UVwidth);
          memset(rowPtr + uoffs*2, 0x80, UVwidth);
        }
        while (*line != '\0')
          *rowPtr++ = (*line++ == ' ') ? 16 : 240;
      }
      output += 1 + strlen(letter->line[0]);
    }
  }
}
#endif


void PlayRTP::Play(OpalPCAPFile & pcap)
{
  cout << "Playing " << pcap.GetFilePath() << endl;

  RTP_DataFrame::PayloadTypes rtpStreamPayloadType = RTP_DataFrame::IllegalPayloadType;
  RTP_DataFrame::PayloadTypes lastUnsupportedPayloadType = RTP_DataFrame::IllegalPayloadType;
  RTP_Timestamp firstTimeStamp = 0;
  RTP_Timestamp lastTimeStamp = 0;
  PTime firstFrameTime(0);
  PTime lastFrameTime(0);
  bool lastPacketMarker = true;
  PTimeInterval frameDuration;
  PTimeInterval minDuration(PMaxTimeInterval);
  PTimeInterval maxDuration;
  PTimeInterval frameTimeStampSum;
  unsigned frameTimeStampCount = 0;

  PINDEX totalBytes = 0;
  unsigned intraFrames = 0;
  int qualitySum = 0;
  unsigned fragmentationCount = 0;
  RTP_SequenceNumber nextSequenceNumber = 0;
  unsigned missingPackets = 0;
  m_packetCount = 0;

  RTP_DataFrame extendedData;
#if OPAL_VIDEO
  m_videoError = false;
  m_videoFrames = 0;
#endif

  bool isAudio = false;
  bool needInfoHeader = true;

  static PTimeInterval const progressTime(0,5);
  PSimpleTimer progressTimer(progressTime);

  PTime playStartTime;

  unsigned filePacketCount = 0;
  while (!pcap.IsEndOfFile()) {
    ++filePacketCount;

    RTP_DataFrame rtp;
    if (pcap.GetRTP(rtp) < 0)
      continue;

    RTP_SequenceNumber thisSequenceNumber = rtp.GetSequenceNumber();
    if (nextSequenceNumber != 0 && thisSequenceNumber != nextSequenceNumber) {
      cout << "Received SN=" << thisSequenceNumber << ", expected SN=" << nextSequenceNumber << endl;
      if (thisSequenceNumber > nextSequenceNumber)
        missingPackets += (thisSequenceNumber - nextSequenceNumber);
    }
    nextSequenceNumber = thisSequenceNumber+1;

    if (rtpStreamPayloadType != rtp.GetPayloadType()) {
      if (rtpStreamPayloadType != RTP_DataFrame::IllegalPayloadType) {
        cout << "Payload type changed in mid file at sequence number " << thisSequenceNumber
             << " in file\"" << pcap.GetFilePath() << '"' << endl;
        continue;
      }
      rtpStreamPayloadType = rtp.GetPayloadType();
    }

    if (m_transcoder == NULL) {
      OpalMediaFormat srcFmt = pcap.GetMediaFormat(rtp);
      if (!srcFmt.IsValid()) {
        if (lastUnsupportedPayloadType != rtpStreamPayloadType) {
          cout << "Unsupported Payload Type " << rtpStreamPayloadType << " in file \"" << pcap.GetFilePath() << '"' << endl;
          lastUnsupportedPayloadType = rtpStreamPayloadType;
        }
        rtpStreamPayloadType = RTP_DataFrame::IllegalPayloadType;
        continue;
      }

      OpalMediaFormat dstFmt;
      if (srcFmt.GetMediaType() == OpalMediaType::Audio()) {
        dstFmt = GetOpalPCM16(srcFmt.GetClockRate(), srcFmt.GetOptionInteger(OpalAudioFormat::ChannelsOption()));
        m_noDelay = true; // Will be paced by output device.
        isAudio = true;
        unsigned frame = srcFmt.GetFrameTime();
        if (frame < 160)
          frame = 160;
        m_player->SetFormat(dstFmt.GetOptionInteger(OpalAudioFormat::ChannelsOption()), dstFmt.GetClockRate());
        m_player->SetBuffers(frame*2, 2000/frame); // 250ms of buffering or Vista goes funny
      }
#if OPAL_VIDEO
      else if (srcFmt.GetMediaType() == OpalMediaType::Video()) {
        dstFmt = OpalYUV420P;
        m_display->Start();
      }
#endif
      else {
        cout << "Unsupported Media Type " << srcFmt.GetMediaType() << " in file \"" << pcap.GetFilePath() << '"' << endl;
        return;
      }

      m_transcoder = OpalTranscoder::Create(srcFmt, dstFmt);
      if (m_transcoder == NULL) {
        cout << "No transcoder for " << srcFmt << " in file \"" << pcap.GetFilePath() << '"' << endl;
        return;
      }

      cout << "Decoding " << srcFmt << " from file \"" << pcap.GetFilePath() << '"' << endl;
      m_transcoder->SetCommandNotifier(PCREATE_NOTIFIER(OnTranscoderCommand));
      firstTimeStamp = lastTimeStamp = rtp.GetTimestamp();
      firstFrameTime = lastFrameTime = pcap.GetPacketTime();
      frameDuration = srcFmt.GetFrameTime()/srcFmt.GetTimeUnits();
    }

    const OpalMediaFormat & inputFmt = m_transcoder->GetInputFormat();

    if (inputFmt == "H.264") {
      static BYTE const StartCode[4] = { 0, 0, 0, 1 };
      m_payloadFile.Write(StartCode, sizeof(StartCode));
    }
    m_payloadFile.Write(rtp.GetPayloadPtr(), rtp.GetPayloadSize());

    RTP_Timestamp thisTimeStamp = rtp.GetTimestamp();
    if (thisTimeStamp != lastTimeStamp) {
      PTime thisPacketTime = pcap.GetPacketTime();

      PTimeInterval deltaTimeStamp = (thisTimeStamp - lastTimeStamp)/inputFmt.GetTimeUnits();
      PTimeInterval deltaPacketTime = thisPacketTime - lastFrameTime;
      if (deltaTimeStamp > deltaPacketTime+200) {
        cout << "Timestamp jump from " << lastTimeStamp << " to " << thisTimeStamp
             << " (" << (thisTimeStamp - lastTimeStamp) << "), "
             << deltaTimeStamp << " > " << deltaPacketTime
             << " SN=" << thisSequenceNumber << endl;
        firstTimeStamp = thisTimeStamp;
        firstFrameTime = thisPacketTime;
      }
      else {
        frameTimeStampSum += deltaTimeStamp;
        frameTimeStampCount++;
        if (minDuration > deltaTimeStamp)
          minDuration = deltaTimeStamp;
        if (maxDuration < deltaTimeStamp)
          maxDuration = deltaTimeStamp;
        if (!m_noDelay)
          PThread::Sleep(deltaTimeStamp);
      }

      deltaTimeStamp = (thisTimeStamp - firstTimeStamp)/inputFmt.GetTimeUnits();
      deltaPacketTime = thisPacketTime - firstFrameTime;
      if (deltaTimeStamp < deltaPacketTime-m_jitterLimit)
        cout << "Excessive jitter detected:"
             << " SN=" << thisSequenceNumber << " TS=" << thisTimeStamp
             << " norm-TS=" << deltaTimeStamp << " Duration=" << deltaPacketTime
             << " delta=" << (deltaPacketTime - deltaTimeStamp) << endl;
      else if (deltaTimeStamp > deltaPacketTime+5) {
        cout << "Early packet detected:"
             << " SN=" << thisSequenceNumber << " TS=" << thisTimeStamp
             << " norm-TS=" << deltaTimeStamp << " Duration=" << deltaPacketTime
             << " delta=" << (deltaPacketTime - deltaTimeStamp) << endl;
        firstTimeStamp = thisTimeStamp;
        firstFrameTime = thisPacketTime;
      }

      if (!lastPacketMarker && !isAudio)
        cout << "Video frame was not correctly terminated with marker:"
                " SN=" << thisSequenceNumber << " TS=" << thisTimeStamp << endl;

      lastTimeStamp = thisTimeStamp;
      lastFrameTime = thisPacketTime;
    }
    lastPacketMarker = rtp.GetMarker();

    if (pcap.IsFragmentated())
      ++fragmentationCount;

    if (m_info > 0) {
      if (needInfoHeader) {
        needInfoHeader = false;
        if (m_info > 0) {
          if (m_info > 1)
            cout << "Packet,RealTime,CaptureTime,";
          cout << "SSRC,SequenceNumber,TimeStamp";
          if (m_info > 1) {
            cout << ",Marker,PayloadType,payloadSize";
            if (isAudio)
              cout << ",DecodedSize";
            else
              cout << ",Width,Height";
            if (m_info > 2)
              cout << ",Data";
          }
          cout << '\n';
        }
      }

      if (m_info > 1)
        cout << m_packetCount << ','
             << PTimer::Tick().GetMilliSeconds() << ','
             << pcap.GetPacketTime().AsString(PTime::EpochTime) << ',';
      cout << "0x" << hex << rtp.GetSyncSource() << dec << ',' << rtp.GetSequenceNumber() << ',' << rtp.GetTimestamp();
      if (m_info > 1)
        cout << ',' << rtp.GetMarker() << ',' << rtp.GetPayloadType() << ',' << rtp.GetPayloadSize();
    }

    if (m_singleStep) 
      cout << "Input packet of length " << rtp.GetPayloadSize() << (rtp.GetMarker() ? " with MARKER" : "") << " -> ";

    totalBytes += rtp.GetPayloadSize();

#if OPAL_VIDEO
    m_vfu = false;
#endif

    RTP_DataFrameList output;
    if (!m_transcoder->ConvertFrames(rtp, output)) {
      cout << "Transcoder error decoding file \"" << pcap.GetFilePath() << '"' << endl;
    }

    if (output.GetSize() == 0) {
      if (m_singleStep) 
        cout << "no frame" << endl;
    }
    else {
      if (m_singleStep) {
        cout << output.GetSize() << " packets" << endl;
        if (cin.get() == 'c')
          m_singleStep = false;
      }

      for (PINDEX i = 0; i < output.GetSize(); i++) {
        const RTP_DataFrame & data = output[i];
        if (isAudio) {
          m_player->Write(data.GetPayloadPtr(), data.GetPayloadSize());
          if (m_info > 1)
            cout << ',' << data.GetPayloadSize();
        }
#if OPAL_VIDEO
        else {
          ++m_videoFrames;

          OpalMediaStatistics stats;
          m_transcoder->GetStatistics(stats);
          if (stats.m_videoQuality >= 0)
            qualitySum += stats.m_videoQuality;

          OpalVideoTranscoder * video = (OpalVideoTranscoder *)m_transcoder;
          if (video->WasLastFrameIFrame()) {
            ++intraFrames;
            m_eventLog << "Frame " << m_videoFrames << ": I-Frame received";
            if (m_videoError)
              m_eventLog << " - decode error cleared";
            m_eventLog << endl;
            m_videoError = false;
          }

          OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)data.GetPayloadPtr();
          m_yuvFile.SetFrameSize(frame->width, frame->height + (m_extendedInfo ? m_extraHeight : 0));

          unsigned extensionId;
          PINDEX extensionLen;
          BYTE * extensionData = data.GetHeaderExtension(extensionId, extensionLen, 0);
          if (extensionData != NULL && extensionId == m_rotateExtensionId) {
            switch (*extensionData >> 4) {
              case 0 : // Portrait left
                PColourConverter::RotateYUV420P(90, frame->width, frame->height, OpalVideoFrameDataPtr(frame));
                std::swap(frame->width, frame->height);
                break;
              case 1 : // Landscape up
                break;
              case 2 : // Portrait right
                PColourConverter::RotateYUV420P(-90, frame->width, frame->height, OpalVideoFrameDataPtr(frame));
                std::swap(frame->width, frame->height);
                break;
              case 3 : // Lnadscape down
                PColourConverter::RotateYUV420P(180, frame->width, frame->height, OpalVideoFrameDataPtr(frame));
                break;
            }
          }

          if (!m_extendedInfo) {

            m_display->SetFrameSize(frame->width, frame->height);
            m_display->SetFrameData(frame->x, frame->y,
                                    frame->width, frame->height,
                                    OpalVideoFrameDataPtr(frame), data.GetMarker());
            m_yuvFile.WriteFrame(OpalVideoFrameDataPtr(frame));
          }
          else {
            int extendedHeight = frame->height + m_extraHeight;
            extendedData.CopyHeader(data);
            extendedData.SetPayloadSize(sizeof(OpalVideoTranscoder::FrameHeader) + PVideoFrameInfo::CalculateFrameBytes(frame->width, extendedHeight));
            OpalVideoTranscoder::FrameHeader * extendedFrame = (OpalVideoTranscoder::FrameHeader *)extendedData.GetPayloadPtr();
            *extendedFrame = *frame;
            extendedFrame->height = extendedHeight;

            PColourConverter::FillYUV420P(0,  0,                extendedFrame->width, extendedFrame->height, 
                                          extendedFrame->width, extendedFrame->height,  OpalVideoFrameDataPtr(extendedFrame), 
                                          0, 0, 0);

            char text[60];
            sprintf(text, "Seq:%08u  Ts:%08u",
                           rtp.GetSequenceNumber(),
                           rtp.GetTimestamp());
            DrawText(4, 4, extendedFrame->width, extendedFrame->height, OpalVideoFrameDataPtr(extendedFrame), text);

            sprintf(text, "TC:%06u  %c %c %c", 
                           m_videoFrames, 
                           m_vfu ? 'V' : ' ', 
                           video->WasLastFrameIFrame() ? 'I' : ' ', 
                           m_videoError ? 'E' : ' ');
            DrawText(4, 20, extendedFrame->width, extendedFrame->height, OpalVideoFrameDataPtr(extendedFrame), text);

            if (m_extraText.GetLength() > 0) 
              DrawText(4, 37, extendedFrame->width, extendedFrame->height, OpalVideoFrameDataPtr(extendedFrame), m_extraText);

            PColourConverter::CopyYUV420P(0, 0,           frame->width,  frame->height, frame->width,         frame->height,         OpalVideoFrameDataPtr(frame),
                                          0, m_extraHeight, frame->width,  frame->height, extendedFrame->width, extendedFrame->height, OpalVideoFrameDataPtr(extendedFrame), 
                                          PVideoFrameInfo::eCropTopLeft);

            m_display->SetFrameSize(extendedFrame->width, extendedFrame->height);
            m_display->SetFrameData(extendedFrame->x,     extendedFrame->y,
                                    extendedFrame->width, extendedFrame->height,
                                    OpalVideoFrameDataPtr(extendedFrame), data.GetMarker());

            m_yuvFile.WriteFrame(OpalVideoFrameDataPtr(extendedFrame));
          }

          if (m_info > 1)
            cout << ',' << frame->width << ',' << frame->height;
        }
#endif
      }
    }

    if (m_info > 0) {
      if (m_info > 2) {
        PINDEX psz = rtp.GetPayloadSize();
        if (m_info == 2 && psz > 30)
          psz = 30;
        cout << ',' << hex << setfill('0');
        const BYTE * ptr = rtp.GetPayloadPtr();
        for (PINDEX i = 0; i < psz; ++i)
          cout << setw(2) << (unsigned)*ptr++;
        cout << dec << setfill(' ');
      }
      cout << '\n';
    }

    ++m_packetCount;

    if (progressTimer.HasExpired()) {
      progressTimer = progressTime;
      cout << "Processing packet " << m_packetCount << " ..." << endl;
      m_eventLog << "Packet " << m_packetCount << ": still processing" << endl;
    }
  }

  m_eventLog << "Packet " << m_packetCount << ": completed" << endl;

  // Output final stats.
  cout << '\n';

  const unsigned TitleWidth = 18;
  const char Colon[] = ": ";

#if OPAL_VIDEO
  if (m_yuvFile.IsOpen())
    cout << setw(TitleWidth) << "Written file" << Colon << m_yuvFile.GetFilePath() << '\n';
#endif

  cout << setw(TitleWidth) << "Payload Type" << Colon << rtpStreamPayloadType;
  if (m_transcoder != NULL)
    cout << " (" << m_transcoder->GetInputFormat() << ')';
  cout << '\n';

  cout << setw(TitleWidth) << "Source"          << Colon << pcap.GetFilterSrcIP() << ':' << pcap.GetFilterSrcPort() << '\n'
       << setw(TitleWidth) << "Destination"     << Colon << pcap.GetFilterDstIP() << ':' << pcap.GetFilterDstPort() << '\n'
       << setw(TitleWidth) << "Total packets"   << Colon << m_packetCount << '\n'
       << setw(TitleWidth) << "Missing packets" << Colon << missingPackets << '\n'
       << setw(TitleWidth) << "IP fragments"    << Colon << fragmentationCount << '\n'
       << setw(TitleWidth) << "Total bytes"     << Colon << totalBytes << '\n';

  PTimeInterval playTime = PTime() - playStartTime;
  if (
#if OPAL_VIDEO
    !m_yuvFile.IsOpen() &&
#endif
        playTime > 0) {
    cout << setw(TitleWidth) << "Duration (real)" << Colon << playTime << " seconds\n"
         << setw(TitleWidth) << "Duration (file)" << Colon << (lastFrameTime - firstFrameTime) << " seconds\n"
         << setw(TitleWidth) << "Bit rate"        << Colon << fixed << setprecision(1) << (totalBytes*8/playTime.GetMilliSeconds()) << "kbps\n";
  }

#if OPAL_VIDEO
  if (m_videoFrames > 0) {
    cout << setw(TitleWidth) << "Resolution"    << Colon << m_yuvFile.GetFrameWidth() << "x" << m_yuvFile.GetFrameHeight() << '\n'
         << setw(TitleWidth) << "Video frames"  << Colon << m_videoFrames << '\n'
         << setw(TitleWidth) << "Intra Frames"  << Colon << intraFrames << '\n';
    if (m_videoFrames > 0 && qualitySum > 0)
      cout << setw(TitleWidth) << "Avg quality" << Colon << (qualitySum/m_videoFrames) << '\n';
    if (!m_yuvFile.IsOpen() && playTime > 0)
      cout << setw(TitleWidth) << "Frame rate"  << Colon << fixed << setprecision(1) << (m_videoFrames*1000.0/playTime.GetMilliSeconds()) << "fps\n";
  }
  else
#endif
  {
    if (playTime > 0)
      cout << setw(TitleWidth) << "Frame time (real)" << Colon << setprecision(3) << (playTime/m_packetCount) << " seconds\n";
  }

  if (frameTimeStampCount > 0)
    cout << setw(TitleWidth) << "Frame TS (avg)" << Colon << setprecision(3) << (frameTimeStampSum/frameTimeStampCount) << " seconds\n"
         << setw(TitleWidth) << "Frame TS (min)" << Colon << minDuration << " seconds\n"
         << setw(TitleWidth) << "Frame TS (max)" << Colon << maxDuration << " seconds\n";

  cout << endl;

#if OPAL_VIDEO
  if (!m_encodedFileName.IsEmpty()) {
    PStringStream args; 
    args << "ffmpeg -r 10 -y -s " << m_yuvFile.GetFrameWidth() << 'x' << m_yuvFile.GetFrameHeight()
         << " -i '" << m_yuvFile.GetFilePath() << "' '" << m_encodedFileName << '\'';
    cout << "Executing command \"" << args << '\"' << endl;
    PPipeChannel cmd;
    if (!cmd.Open(args, PPipeChannel::ReadWriteStd, true)) 
      cout << "failed";
    cmd.Execute();
    cmd.WaitForTermination();
    cout << "done" << endl;
  }
#endif

  // Clean up
  delete m_transcoder;
  m_transcoder = NULL;
}


void PlayRTP::OnTranscoderCommand(OpalMediaCommand & command, P_INT_PTR /*extra*/)
{
#if OPAL_VIDEO
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    PStringStream msg;
    msg << "Packet " << m_packetCount << ", frame " << m_videoFrames << ": decoding error";
      
    OpalVideoPictureLoss * loss = dynamic_cast<OpalVideoPictureLoss *>(&command);
    if (loss != NULL)
      msg << " for sn=" << loss->GetSequenceNumber() << ", ts=" << loss->GetTimestamp();
    m_eventLog << msg << endl;
    cout << msg << endl;
    m_videoError = m_vfu = true;
  }
#endif
}


// End of File ///////////////////////////////////////////////////////////////
