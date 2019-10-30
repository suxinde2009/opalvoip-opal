/*
 * silencedetect.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "silencedetect.h"
#endif
#include <opal_config.h>

#include <codec/silencedetect.h>
#include <opal/patch.h>

#include <cmath>


#define new PNEW
#define PTraceModule() "Silence"


///////////////////////////////////////////////////////////////////////////////

class OpalSilenceDetector::MyData : public PObject
{
  friend class OpalSilenceDetector;

  OpalSilenceDetector & m_owner;

  Result   m_lastResult;
  int      m_levelThreshold;
  bool     m_wasActive;
  bool     m_initialiseTimestamps;
  bool     m_initialiseThreshold;
  unsigned m_wrapCheckTimestamp;
  unsigned m_nextDeadbandTimestamp;
  unsigned m_nextAdaptionTimestamp;
  unsigned m_signalDeadband;
  unsigned m_silenceDeadband;
  unsigned m_adaptivePeriod;

  struct Sample
  {
    unsigned m_timestamp;
    int      m_level;
    Sample(unsigned timestamp, int level) : m_timestamp(timestamp), m_level(level) { }
  };

  struct History : std::deque<Sample>
  {
    unsigned m_period;
    int64_t  m_sum;
    int      m_average;

    History()
      : m_period(0)
      , m_sum(0)
      , m_average(OpalSilenceDetector::MinAudioLevel)
    {
    }

    void Reset()
    {
      clear();
      m_sum = 0;
    }

    void Process(unsigned timestamp, int level)
    {
      if (size() >= 2) {
        unsigned thresholdTimestamp = timestamp - m_period;

        // had a big pause and whole history has aged
        if (back().m_timestamp < thresholdTimestamp)
          Reset();
        else {
          // REmove the old entries
          iterator next = begin();
          iterator earliest = next++;
          while (next != end() && next->m_timestamp <= thresholdTimestamp) {
            m_sum -= (int64_t)(next->m_timestamp - earliest->m_timestamp) * earliest->m_level;
            erase(earliest++);
            ++next;
          }
          // In case we have split a period, keep old level and just shorten the period
          if (thresholdTimestamp > earliest->m_timestamp) {
            m_sum -= (int64_t)(thresholdTimestamp - earliest->m_timestamp) * earliest->m_level;
            earliest->m_timestamp = thresholdTimestamp;
          }
        }
      }

      if (empty()) {
        Reset();
        m_average = level;
      }
      else if (timestamp > front().m_timestamp) {
        m_sum += (int64_t)(timestamp - back().m_timestamp) * back().m_level;
        m_average = (int)(m_sum / (timestamp - front().m_timestamp));
      }

      push_back(Sample(timestamp, level));
    }
  };

  History m_shortTerm, m_longTerm;

public:
  MyData(OpalSilenceDetector & owner)
    : m_owner(owner)
    , m_lastResult(VoiceInactive)
    , m_levelThreshold(MinAudioLevel)
    , m_wasActive(false)
    , m_initialiseTimestamps(true)
    , m_initialiseThreshold(true)
    , m_wrapCheckTimestamp(0)
    , m_nextDeadbandTimestamp(0)
    , m_nextAdaptionTimestamp(0)
    , m_signalDeadband(0)
    , m_silenceDeadband(0)
    , m_adaptivePeriod(0)
  {
  }


  void ChangedParameters()
  {
    m_signalDeadband = m_owner.m_params.m_signalDeadband*m_owner.m_params.m_sampleRate/1000;
    m_silenceDeadband = m_owner.m_params.m_silenceDeadband*m_owner.m_params.m_sampleRate/1000;
    m_adaptivePeriod = m_owner.m_params.m_adaptivePeriod*m_owner.m_params.m_sampleRate/1000;
    m_shortTerm.m_period = m_owner.m_params.m_shortTermPeriod*m_owner.m_params.m_sampleRate/1000;
    m_longTerm.m_period = m_owner.m_params.m_longTermPeriod*m_owner.m_params.m_sampleRate/1000;

    switch (m_owner.m_params.m_mode) {
      case NoSilenceDetection :
        m_lastResult = VoiceActive;
        m_levelThreshold = INT_MAX;
        break;

      case FixedSilenceDetection :
        m_levelThreshold =  m_owner.m_params.m_threshold <= 0
                         ?  m_owner.m_params.m_threshold            // This value compared to dBov encoded signal level
                         : (m_owner.m_params.m_threshold/2 - 127); // Backward compatibility with old uLaw value (not very accurately)
        break;

      default :
        // Initialise threshold level to half way - pretty quiet, actually
        m_levelThreshold = (MaxAudioLevel + MinAudioLevel)/2;
    }

    Reset();

    PTRACE(3, "Parameters set: "
              "mode=" << m_owner.m_params.m_mode << ", "
              "threshold=" << m_levelThreshold << "dBov, "
              "silencedb=" << m_owner.m_params.m_silenceDeadband << "ms=" << m_silenceDeadband << " samples, "
              "signaldb=" << m_owner.m_params.m_signalDeadband << "ms=" << m_signalDeadband << " samples, "
              "period=" << m_owner.m_params.m_adaptivePeriod << "ms=" << m_adaptivePeriod << " samples");
  }


  void Reset()
  {
    m_lastResult = VoiceInactive;
    m_initialiseThreshold = m_initialiseTimestamps = true;
    m_shortTerm.Reset();
    m_longTerm.Reset();
    PTRACE(3, "Reset of adaptive data");
  }


  void Detect(unsigned timestamp, int audioLevel)
  {
    // Make sure the transitional result values are moved to steady state
    switch (m_lastResult) {
      case VoiceActivated :
        m_lastResult = VoiceActive;
        break;
      case VoiceDeactivated :
        m_lastResult = VoiceInactive;
        break;
      default :
        break;
    }

    if (m_initialiseTimestamps) {
      m_initialiseTimestamps = false;
      m_wasActive = m_lastResult > VoiceInactive;
      m_wrapCheckTimestamp = timestamp;
      m_nextDeadbandTimestamp = timestamp + (m_wasActive ? m_silenceDeadband : m_signalDeadband);
      m_nextAdaptionTimestamp = timestamp + m_adaptivePeriod;
    }

    // We wrapped around the timestamp?
    if (timestamp < m_wrapCheckTimestamp) {
      Reset();
      return;
    }

    m_shortTerm.Process(timestamp, audioLevel);

    /* While we are in the initialisation phase, we set the threshold to the
       long term average we have so far. */
    if (m_initialiseThreshold && m_owner.m_params.m_mode == AdaptiveSilenceDetection)
      m_levelThreshold = m_longTerm.m_average;

    // Have we changed?
    bool lastResultActive = m_lastResult > VoiceInactive;
    bool nowActive = m_shortTerm.m_average > m_levelThreshold;
    if (nowActive != m_wasActive) {
      m_nextDeadbandTimestamp = timestamp + (lastResultActive ? m_silenceDeadband : m_signalDeadband);
      m_wasActive = nowActive;
    }
    else if (nowActive != lastResultActive && timestamp > m_nextDeadbandTimestamp) {
      m_lastResult = nowActive ? VoiceActivated : VoiceDeactivated;
      PTRACE(4, "Detector transition:"
                " " << m_lastResult << ","
                " level=" << m_shortTerm.m_average << "dBov,"
                " threshold=" << m_levelThreshold << "dBov");
    }

    if (m_owner.m_params.m_mode == FixedSilenceDetection)
      return;

    // Use long term average for adapting threshold
    m_longTerm.Process(timestamp, audioLevel);

    if (timestamp < m_nextAdaptionTimestamp)
      return;

    m_nextAdaptionTimestamp = timestamp + m_adaptivePeriod;

    if (m_initialiseThreshold) {
      m_initialiseThreshold = false;

      /* We initialise the threshold to the long term average. Unless the
         speaker never takes a breath, or we are detecteing music, then the
         average should be below the peaks of active speech. We then fine
         tune the threshold adjustment over time. */
      m_levelThreshold = m_longTerm.m_average;
      PTRACE(4, "Threshold initialised to " << m_levelThreshold);
    }
    else if (m_longTerm.m_average > m_levelThreshold) {
      /* If every frame was noisy, move threshold up. Don't want to move too
         fast so only go a quarter of the way to minimum signal value over the
         period. This avoids oscillations, and time will continue to make the
         level go up if there really is a lot of background noise.
       */
      int newThreshold = m_levelThreshold - (m_levelThreshold - m_longTerm.m_average - 3)/4;
      if (m_levelThreshold < newThreshold) {
        PTRACE(4, "Threshold increased:"
                  " old=" << m_levelThreshold << ","
                  " new=" << newThreshold << ","
                  " average=" << m_longTerm.m_average);
        m_levelThreshold = newThreshold;
      }
    }
    else {
      /* If every frame was quiet, move threshold down. Again do not want to
         move too quickly, but we do want it to move faster down than up, so
         move to halfway to maximum value of the quiet period. As a rule the
         lower the threshold the better as it would improve response time to
         the start of a talk burst. We also do not get to close to a lower
         bound, increasing the threshold by 10% on the long term average. This
         is becuase when very quiet, (-100 and below) it is very susceptable
         to noise and large changes can occur even when really silent.
       */
      int newThreshold = (m_levelThreshold + m_longTerm.m_average)/2 - m_longTerm.m_average/10;
      if (m_levelThreshold > newThreshold) {
        PTRACE(4, "Threshold decreased:"
                  " old=" << m_levelThreshold << ","
                  " new=" << newThreshold << ","
                  " average=" << m_longTerm.m_average);
        m_levelThreshold = newThreshold;
      }
    }
  }
};


///////////////////////////////////////////////////////////////////////////////

OpalSilenceDetector::OpalSilenceDetector(const Params & params)
  : m_receiveHandler(PCREATE_NOTIFIER(ReceivedPacket))
  , m_params(params)
  , m_data(new MyData(*this))
{
  m_data->ChangedParameters();

  PTRACE(4, "Handler created");
}


OpalSilenceDetector::~OpalSilenceDetector()
{
  delete m_data;
}


void OpalSilenceDetector::AdaptiveReset()
{
  PWaitAndSignal mutex(m_inUse);
  PTRACE_CONTEXT_ID_TO(*m_data);
  if (m_params.m_mode == AdaptiveSilenceDetection)
    m_data->Reset();
}


void OpalSilenceDetector::SetParameters(const Params & params, const int sampleRate /*= 0*/)
{
  PWaitAndSignal mutex(m_inUse);
  PTRACE_CONTEXT_ID_TO(*m_data);

  m_params = params;
  if (sampleRate > 0)
    m_params.m_sampleRate = sampleRate;

  m_data->ChangedParameters();
}


void OpalSilenceDetector::SetClockRate(unsigned rate)
{
  PWaitAndSignal mutex(m_inUse);
  PTRACE_CONTEXT_ID_TO(*m_data);
  m_params.m_sampleRate = rate;
  m_data->ChangedParameters();
}


void OpalSilenceDetector::GetParameters(Params & params) const
{
  PWaitAndSignal mutex(m_inUse);
  params = m_params;
  params.m_threshold = m_data->m_levelThreshold;
}


PString OpalSilenceDetector::Params::AsString() const
{
  return PSTRSTRM(m_mode << ','
               << m_threshold << ','
               << m_signalDeadband << ','
               << m_silenceDeadband << ','
               << m_adaptivePeriod << ','
               << m_shortTermPeriod << ','
               << m_longTermPeriod);
}


void OpalSilenceDetector::Params::FromString(const PString & str)
{
  PStringArray params = str.Tokenise(',');
  switch (params.GetSize()) {
    default :
    case 7 :
      m_longTermPeriod = params[4].AsUnsigned();
    case 6 :
      m_shortTermPeriod = params[4].AsUnsigned();
    case 5 :
      m_adaptivePeriod = params[4].AsUnsigned();
    case 4 :
      m_silenceDeadband = params[3].AsUnsigned();
    case 3 :
      m_signalDeadband = params[2].AsUnsigned();
    case 2 :
      m_threshold = params[1].AsInteger();
    case 1 :
      m_mode = params[0].As<OpalSilenceDetector::Modes>(m_mode);
    case 0 :
      break;
  }
}


OpalSilenceDetector::Result OpalSilenceDetector::GetResult(ResultInfo * info) const
{
  PWaitAndSignal mutex(m_inUse);

  if (info != NULL) {
    info->m_result = m_data->m_lastResult;
    info->m_currentThreshold = m_data->m_levelThreshold;
    info->m_shortTermAverage = m_data->m_shortTerm.m_average;
    info->m_longTermAverage = m_data->m_longTerm.m_average;
  }

  return m_data->m_lastResult;
}


void OpalSilenceDetector::ReceivedPacket(RTP_DataFrame & frame, P_INT_PTR)
{
  switch (Detect(frame)) {
    case VoiceDeactivated :
    case VoiceInactive :
      frame.SetPayloadSize(0); // Not in talk burst so silence the frame
      break;

    case VoiceActivated :
      frame.SetMarker(true); // If we just have moved to sending a talk burst, set the RTP marker
    default :
      break;
  }
}


OpalSilenceDetector::Result OpalSilenceDetector::Detect(const RTP_DataFrame & rtp, ResultInfo * info)
{
  return Detect(rtp.GetPayloadPtr(),
                rtp.GetPayloadSize(),
                rtp.GetTimestamp(),
                rtp.GetMetaData().m_audioLevel,
                info);
}


OpalSilenceDetector::Result OpalSilenceDetector::Detect(const BYTE * audioPtr,
                                                        PINDEX audioLen,
                                                        unsigned timestamp,
                                                        int audioLevel,
                                                        ResultInfo * info)
{
  PWaitAndSignal mutex(m_inUse);

  // Can never have silence if NoSilenceDetection
  if (m_params.m_mode == NoSilenceDetection)
    return VoiceActive;

  if (audioLevel == INT_MAX) {
    audioLevel = audioLen == 0 ? MinAudioLevel : GetAudioLevelDB(audioPtr, audioLen);
    if (audioLevel < MinAudioLevel || audioLevel > MaxAudioLevel)
      return VoiceActive; // Something wrong
  }

  m_data->Detect(timestamp, audioLevel);
  return GetResult(info);
}


void OpalSilenceDetector::CalculateDB::Reset()
{
  m_rmsSum = 0;
  m_rmsSamples = 0;
}


OpalSilenceDetector::CalculateDB & OpalSilenceDetector::CalculateDB::Accumulate(const void * pcm, PINDEX size)
{
  const short * samplePtr = (const short *)pcm;
  PINDEX sampleCount = size/sizeof(short);

  m_rmsSamples += sampleCount;

  while (sampleCount-- > 0) {
    double sample = (double)(*samplePtr++) / std::numeric_limits<short>::max();
    m_rmsSum += sample * sample;
  }

  return *this;
}


int OpalSilenceDetector::CalculateDB::Finalise()
{
  // Calculate the root mean square (RMS) of the signal.
  double rms = std::sqrt(m_rmsSum / m_rmsSamples);

  Reset(); // Ready for next block

  /* The audio level is a logarithmic measure of the rms level of an audio
     sample relative to a reference value and is measured in decibels. */
  if (rms <= 0)
    return MinAudioLevel; // zero RMS is digital silence

  /* The "zero" reference level is the overload level, which corresponds to
     1.0 in this calculation, because the samples are normalized in
     calculating the RMS. */
  double db = 20 * std::log10(rms);

  /* Ensure that the calculated level is within the minimum and maximum range
     permitted. */
  if (db < MinAudioLevel)
    return MinAudioLevel;
  if (db > MaxAudioLevel)
    return MaxAudioLevel;
  return (int)rint(db);
}


/////////////////////////////////////////////////////////////////////////////

int OpalPCM16SilenceDetector::GetAudioLevelDB(const BYTE * buffer, PINDEX size)
{
  return m_calculator.Accumulate(buffer, size).Finalise();
}


/////////////////////////////////////////////////////////////////////////////
