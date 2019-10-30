/*
 * silencedetect.h
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2004 Post Increment
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
 */

#ifndef OPAL_CODEC_SILENCEDETECT_H
#define OPAL_CODEC_SILENCEDETECT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>
#include <rtp/rtp.h>


///////////////////////////////////////////////////////////////////////////////

/**Implement silence detection of audio.
   This is the complement of Voice Activity Detection (VAD) and can be used for
   that purpose.

   The signal level is returned as an integer from 0 to -127, representing dBov
   as per RFC6464.

   For backward compatibility the \p threshold parameter will linearly map
   0..255 uLaw to -127..0 dBov, as an approximation of the same value.
  */
class OpalSilenceDetector : public PObject
{
    PCLASSINFO(OpalSilenceDetector, PObject);
  public:
    P_DECLARE_STREAMABLE_ENUM(Modes,
      NoSilenceDetection,
      FixedSilenceDetection,
      AdaptiveSilenceDetection
    );
    typedef Modes Mode; // Backward compatibility

    enum LevelLimits {
      MinAudioLevel = -127,  /// Digital silence
      MaxAudioLevel = 0      /// Overload
    };

    struct Params {
      Params(
        Modes mode = AdaptiveSilenceDetection, ///<  New silence detection mode
        int threshold = 0,                     ///<  Threshold value if FixedSilenceDetection
        unsigned signalDeadband = 40,          ///<  milliseconds of signal needed
        unsigned silenceDeadband = 400,        ///<  milliseconds of silence needed
        unsigned adaptivePeriod = 1000,        ///<  millisecond window for adaptive threshold
        unsigned shortTermPeriod = 200,        ///<  millisecond period to average levels over
        unsigned longTermPeriod = 4000,        ///<  millisecond period to average levels over
        unsigned sampleRate = 8000             ///<  sample rate for audio
      )
        : m_mode(mode)
        , m_threshold(threshold)
        , m_signalDeadband(signalDeadband)
        , m_silenceDeadband(silenceDeadband)
        , m_adaptivePeriod(adaptivePeriod)
        , m_shortTermPeriod(shortTermPeriod)
        , m_longTermPeriod(longTermPeriod)
        , m_sampleRate(sampleRate)
      { }

      PString AsString() const;
      void FromString(const PString & str);

      Modes    m_mode;             /// Silence detection mode
      int      m_threshold;        /// Threshold value if FixedSilenceDetection
      unsigned m_signalDeadband;   /// milliseconds of signal needed
      unsigned m_silenceDeadband;  /// milliseconds of silence needed
      unsigned m_adaptivePeriod;   /// millisecond window for adaptive threshold
      unsigned m_shortTermPeriod;  /// millisecond period to average levels over
      unsigned m_longTermPeriod;   /// millisecond period to average levels over
      unsigned m_sampleRate;       /// sample rate for audio
    };

  /**@name Construction */
  //@{
    /**Create a new detector. Default clock rate is 8000.
     */
    OpalSilenceDetector(
      const Params & newParam = Params() ///<  New parameters for silence detector
    );

    /** Destructor
     */
    ~OpalSilenceDetector();
  //@}

  /**@name Basic operations */
  //@{
    const PNotifier & GetReceiveHandler() const { return m_receiveHandler; }

    /**Set the silence detector parameters.
       This adjusts the silence detector "agression". The deadband and 
       adaptive periods are in ms units to work for any clock rate.
	   The clock rate value is optional: 0 leaves value unchanged.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
      */
    void SetParameters(
      const Params & params,  ///< New parameters for silence detector
      const int sampleRate = 0 ///< Sampling clock rate for the preprocessor
    );

    /**Get the current parameters in use.
      */
    Params GetParameters() const { return m_params; }
    void GetParameters(
      Params & params
    ) const;

    /**Set the sampling clock rate for the preprocessor.
       Adusts the interpretation of time values.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
     */
    void SetClockRate(
      unsigned clockRate     ///< Sampling clock rate for the preprocessor
    );

    /**Get the current sampling clock rate.
      */
    unsigned GetClockRate() const { return m_params.m_sampleRate; }

    /**Reset the adaptive filter
     */
    void AdaptiveReset();

    /// Codes for detection result.
    P_DECLARE_STREAMABLE_ENUM(Result,
      VoiceDeactivated, // Note, order is important
      VoiceInactive,
      VoiceActivated,
      VoiceActive
    );
    enum { IsSilent = VoiceInactive }; // For backward compatibility (mostly)

    struct ResultInfo {
        /// The current result of the detector.
        Result m_result;

        /// The current threshold value being used.
        int m_currentThreshold;

        /** The short term energy value as dBov (-127 to 0) which is used in
            comparison to the threshold value for audio active/inactive. */
        int m_shortTermAverage;

        /** The long term energy value as dBov (-127 to 0) which is used to
            adaptively adjust the threshold value. */
        int m_longTermAverage;
    };

    /**Get silence detection status
      */
    Result GetResult(
      ResultInfo * info = NULL ///< Result information
    ) const;

    /**Detemine (in context) if audio stream is currently silent.
      */
    Result Detect(
      const BYTE * audioPtr,   ///< Pointer to PCM-16 audio data
      PINDEX audioLen,         ///< length in bytes of PCM-16 audio data
      unsigned timestamp,      ///< Timestamp in RTP units
      int audioLevel,          ///< Level (dBov) of audio data, INT_MAX is unknown
      ResultInfo * info = NULL ///< Result information
    );
    Result Detect(
      const RTP_DataFrame & rtp,  ///< RTP packet to detect
      ResultInfo * info = NULL    ///< Result information
    );

    /**Get the average signal level in the stream.
       This is called from within the silence detection algorithm to
       calculate the average signal level of the last data frame read from
       the stream.

       The default behaviour returns INT_MAX which indicates that the
       average signal has no meaning for the stream.

       @return 0 to -127 dBov as per RFC6464
      */
    virtual int GetAudioLevelDB(
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size           ///<  Size of payload buffer
    ) = 0;

    /**Calculate the average signal level for PCM-16 data.
       This code is adapted from the reference in RFC6465.
      */
    class CalculateDB
    {
      public:
        CalculateDB() { Reset(); }
        void Reset();
        CalculateDB & Accumulate(
          const void * pcm, /// Pointer to PCM-16 data
          PINDEX size       /// Size, in bytes, of PCM-16 data
        );
        int Finalise();
      protected:
        double   m_rmsSum;
        unsigned m_rmsSamples;
    };

  private:
    OpalSilenceDetector(const OpalSilenceDetector &) { }
    void operator=(const OpalSilenceDetector &) { }

  protected:
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalSilenceDetector, ReceivedPacket);

    PDECLARE_MUTEX(m_inUse);
    PNotifier      m_receiveHandler;
    Params         m_params;

    // Some implementation hiding.
    class MyData;
    MyData * m_data;
    friend class MyData;
};


class OpalPCM16SilenceDetector : public OpalSilenceDetector
{
    PCLASSINFO(OpalPCM16SilenceDetector, OpalSilenceDetector);
  public:
    /** Construct new silence detector for PCM-16/8000
      */
    OpalPCM16SilenceDetector(
      const Params & newParam = Params() ///<  New parameters for silence detector
    ) : OpalSilenceDetector(newParam) { }

  /**@name Overrides from OpalSilenceDetector */
  //@{
    virtual int GetAudioLevelDB(
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size           ///<  Size of payload buffer
    );
    //@}

  protected:
    CalculateDB m_calculator;
};


extern ostream & operator<<(ostream & strm, OpalSilenceDetector::Mode mode);


#endif // OPAL_CODEC_SILENCEDETECT_H


/////////////////////////////////////////////////////////////////////////////
