/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <sip/sipcon.h>

/**OpalConnection::StringOption key to a boolean indicating the SDP ptime
   parameter should be included in audio session streams. Default false.
  */
#define OPAL_OPT_OFFER_SDP_PTIME "Offer-SDP-PTime"

/**OpalConnection::StringOption key to a boolean indicating the the state
   of the "Refer-Sub" header in the REFER request. Default true.
  */
#define OPAL_OPT_REFER_SUB       "Refer-Sub"

/**OpalConnection::StringOption key to an integer indicating the the mode
   for the reliable provisional response system. See PRACKMode for more
   information. Default is from SIPEndPoint::GetDefaultPRACKMode() which
   in turn defaults to e_prackSupported.
  */
#define OPAL_OPT_PRACK_MODE      "PRACK-Mode"

/**OpalConnection::StringOption key to a boolean indicating that we should
   make initial SDP offer. Default true.
  */
#define OPAL_OPT_INITIAL_OFFER "Initial-Offer"

/**OpalConnection::StringOption key to a string for a regular expression to
   match the product information, which if matching the remote system, will
   indicate the remote does not support asymmetric hold as required by the
   standard.
   
   This fault is when SDP sendonly is sent (us putting them on hold), and
   they reply inactive, which implies them putting us on hold. When we
   subsequently send recvonly to release our hold to them, they continue to
   send inactive, and hold is never released.

   Note the OpalProductInfo vendor, name & version strings are concatenated
   before comparison with the regular expression.

   Defaults to empty string.
  */
#define OPAL_OPT_SYMMETRIC_HOLD_PRODUCT "Symmetric-Hold-Product"

/**OpalConnection::StringOption key to a string representing the precise SDP
   to be included in the INVITE offer. No media streams are opened or any
   checks whstsoever made on the string. It is simply included as the body of
   the INVITE.
   
   Note this options presence also prevents handling of the response SDP
   
   Defaults to empty string which generates SDP from available
   media formats and media streams.
  */
#define OPAL_OPT_EXTERNAL_SDP "External-SDP"

#define SIP_HEADER_PREFIX      "SIP-Header:"
#define SIP_HEADER_REPLACES    SIP_HEADER_PREFIX"Replaces"
#define SIP_HEADER_REFERRED_BY SIP_HEADER_PREFIX"Referred-By"
#define SIP_HEADER_CONTACT     SIP_HEADER_PREFIX"Contact"

#define OPAL_SIP_REFERRED_CONNECTION "Referred-Connection"
%End

/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol connection.
 */

class SIPConnection : OpalRTPConnection
{
  public:

  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    SIPConnection(
      OpalCall & call,                          ///<  Owner call for connection
      SIPEndPoint & endpoint,                   ///<  Owner endpoint for connection
      const PString & token,                    ///<  token to identify the connection
      const SIPURL & address,                   ///<  Destination address for outgoing call
      OpalTransport * transport,                ///<  Transport INVITE came in on
      unsigned int options = 0,                 ///<  Connection options
      OpalConnection::StringOptions * stringOptions = NULL  ///<  complex string options
    );

    /**Destroy connection.
     */
    ~SIPConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const;

    /**Get this connections protocol prefix for URLs.
      */
    virtual PString GetPrefixName() const;

    /**Get the protocol-specific unique identifier for this connection.
     */
    virtual PString GetIdentifier() const;

	/// Call back for connection to act on changed string options
	virtual void OnApplyStringOptions();
	
    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is .
      */
    virtual PBoolean SetUpConnection();

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.

       The default behaviour for sip returns the request URI in the INVITE.
      */
    virtual PString GetDestinationAddress();

    /**Get the fulll URL being indicated by the remote for incoming calls. This may
       not have any relation to the local name of the endpoint.

       The default behaviour returns GetDestinationAddress() normalised to a URL.
       The remote may provide a full URL, if it does not then the prefix for the
       endpoint is prepended to the destination address.
      */
    virtual PString GetCalledPartyURL();

    /**Get alerting type information of an incoming call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns an empty string.
      */
    virtual PString GetAlertingType() const;

    /**Set alerting type information for outgoing call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns false.
      */
    virtual bool SetAlertingType(const PString & info);

    /**Get call information of an incoming call.
       This is protocol dependent information provided about the call. The
       details are outside the scope of this help.

       For SIP this corresponds to the string contained in the "Call-Info"
       header field of the INVITE.
      */
    virtual PString GetCallInfo() const;

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );

    /**Put the current connection on hold, suspending all media streams.
       The \p fromRemote parameter indicates if we a putting the remote on
       hold (false) or it is a request for the remote to put us on hold (true).

       The /p placeOnHold parameter indicates of teh command/request is for
       going on hold or retrieving from hold.
     */
    virtual bool Hold(
      bool fromRemote,  ///< Flag for if remote has us on hold, or we have them
      bool placeOnHold  ///< Flag for setting on or off hold
    );

    /**Return true if the current connection is on hold.
       The \p fromRemote parameter indicates if we are testing if the remote
       system has us on hold, or we have them on hold.
     */
    virtual bool IsOnHold(
      bool fromRemote  ///< Flag for if remote has us on hold, or we have them
    );

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia            ///<  Flag to alert with/without media
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual PBoolean SetConnected();

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
    
    /**Open source or sink media stream for session.
      */
/*     virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    ); */

    /**Request close of a specific media stream.
       Note that this is usually asymchronous, the OnClosedMediaStream() function is
       called when the stream is really closed.
      */
    virtual bool CloseMediaStream(
      OpalMediaStream & stream  ///< Stream to close
    );

    /**Pause media streams for connection.
      */
    virtual void OnPauseMediaStream(
      OpalMediaStream & strm,     ///< Media stream paused/un-paused
      bool paused                 ///< Flag for pausing/un-pausing
    );

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnReleased();

    /**Forward incoming connection to the specified address.
       This would typically be called from within the OnIncomingConnection()
       function when an application wishes to redirect an unwanted incoming
       call.

       The return value is true if the call is to be forwarded, false 
       otherwise. Note that if the call is forwarded, the current connection
       is cleared with the ended call code set to EndedByCallForwarded.
      */
    virtual PBoolean ForwardCall(
      const PString & forwardParty   ///<  Party to forward call to.
    );

    /**Get the real user input indication transmission mode.
       This will return the user input mode that will actually be used for
       transmissions. It will be the value of GetSendUserInputMode() provided
       the remote endpoint is capable of that mode.
      */
    virtual SendUserInputModes GetRealSendUserInputMode() const;
	
	/**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       The default behaviour is to call SendUserInputTone() for each character
       in the string.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something more sophisticated
       than the simple tones that can be sent using the SendUserInput()
       function.

       A duration of zero indicates that no duration is to be indicated.
       A non-zero logical channel indicates that the tone is to be syncronised
       with the logical channel at the rtpTimestamp value specified.

       The tone parameter must be one of "0123456789#*ABCD!" where '!'
       indicates a hook flash. If tone is a ' ' character then a
       signalUpdate PDU is sent that updates the last tone indication
       sent. See the H.245 specifcation for more details on this.

       The default behaviour sends the tone using RFC2833.
      */
    PBoolean SendUserInputTone(char tone, unsigned duration);

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const RTP_Session & session         ///<  Session with statistics
    ) const;
  //@}

  /**@name Protocol handling functions */
  //@{
    /**Handle the fail of a transaction we initiated.
      */
    virtual void OnTransactionFailed(
      SIPTransaction & transaction
    );

    /**Handle an incoming SIP PDU that has been full decoded
      */
    virtual void OnReceivedPDU(SIP_PDU & pdu);

    /**Handle an incoming INVITE request
      */
    virtual void OnReceivedINVITE(SIP_PDU & pdu);

    /**Handle an incoming Re-INVITE request
      */
    virtual void OnReceivedReINVITE(SIP_PDU & pdu);

    /**Handle an incoming ACK PDU
      */
    virtual void OnReceivedACK(SIP_PDU & pdu);
  
    /**Handle an incoming OPTIONS PDU
      */
    virtual void OnReceivedOPTIONS(SIP_PDU & pdu);

    /**Handle an incoming NOTIFY PDU
      */
    virtual void OnReceivedNOTIFY(SIP_PDU & pdu);

    /**Callback function on receipt of an allowed NOTIFY message.
       Allowed events are determined by the m_allowedEvents member variable.
      */
    virtual void OnAllowedEventNotify(
      const PString & eventName  ///< Name of event
    );

    /**Handle an incoming REFER PDU
      */
    virtual void OnReceivedREFER(SIP_PDU & pdu);
  
    /**Handle an incoming INFO PDU
      */
    virtual void OnReceivedINFO(SIP_PDU & pdu);

    /**Handle an incoming PING PDU
      */
    virtual void OnReceivedPING(SIP_PDU & pdu);

    /**Handle an incoming PRACK PDU
      */
    virtual void OnReceivedPRACK(SIP_PDU & pdu);

    /**Handle an incoming BYE PDU
      */
    virtual void OnReceivedBYE(SIP_PDU & pdu);
  
    /**Handle an incoming CANCEL PDU
      */
    virtual void OnReceivedCANCEL(SIP_PDU & pdu);
  
    /**Handle an incoming response PDU to our INVITE.
       Note this is called before th ACK is sent and thus should do as little as possible.
       All the hard work (SDP processing etc) should be in the usual OnReceivedResponse().
      */
    virtual void OnReceivedResponseToINVITE(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming response PDU.
      */
    virtual void OnReceivedResponse(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming Trying response PDU
      */
    virtual void OnReceivedTrying(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
  
    /**Handle an incoming Ringing response PDU
      */
    virtual void OnReceivedRinging(SIP_PDU & pdu);
  
    /**Handle an incoming Session Progress response PDU
      */
    virtual void OnReceivedSessionProgress(SIP_PDU & pdu);
  
    /**Handle an incoming Proxy Authentication Required response PDU
       Returns: true if handled, if false is returned connection is released.
      */
    virtual PBoolean OnReceivedAuthenticationRequired(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
  
    /**Handle an incoming redirect response PDU
      */
    virtual void OnReceivedRedirection(SIP_PDU & pdu);

    /**Handle an incoming OK response PDU.
       This actually gets any PDU of the class 2xx not just 200.
      */
    virtual void OnReceivedOK(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
  
    /**Handle a sending INVITE request
      */
    virtual void OnCreatingINVITE(SIPInvite & pdu);

    enum TypeOfINVITE {
      IsNewINVITE,
      IsDuplicateINVITE,
      IsReINVITE,
      IsLoopedINVITE
    };

    /// Indicate if this is a duplicate or multi-path INVITE
    SIPConnection::TypeOfINVITE CheckINVITE(
      const SIP_PDU & pdu
    ) const;

	
    /*enum TypeOfALERTING {
		Is180Ringing,
		Is183SessionProgress,
		NotSet
    };*/

    //TypeOfALERTING GetAlertingMethod();
	
    /**Send an OPTIONS command within this calls dialog.
       Note if \p reply is non-NULL, this function will block until the
       transaction completes. Care must be executed in this case that
       no deadlocks occur.
      */
    bool SendOPTIONS(
      const SIPOptions::Params & params,  ///< Parameters for OPTIONS command
      SIP_PDU * reply /Out/ = NULL              ///< Reply to message
    );

    /**Send an INFO command within this calls dialog.
       Note if \p reply is non-NULL, this function will block until the
       transaction completes. Care must be executed in this case that
       no deadlocks occur.
      */
    bool SendINFO(
      const SIPInfo::Params & params,  ///< Parameters for OPTIONS command
      SIP_PDU * reply /Out/ = NULL            ///< Reply to message
    );
  //@}

    OpalTransportAddress GetDefaultSDPConnectAddress(WORD port = 0) const;

    OpalTransport & GetTransport() const;
    bool SetTransport(const SIPURL & destination);

    SIPEndPoint & GetEndPoint() const;
    SIPDialogContext & GetDialog();
    //const SIPDialogContext & GetDialog() const;
    SIPAuthentication * GetAuthenticator() const;

    /// Mode for reliable provisional responses.
    enum PRACKMode {
      e_prackDisabled,  /**< Do not use PRACK if remote asks for 100rel in Supported
                             field, refuse call with 420 Bad Extension if 100rel is
                             in Require header field. */
                             
      e_prackSupported, /** Add 100rel to Supported header in outgoing INVITE. For
                            incoming INVITE enable PRACK is either Supported or
                            Require headers include 100rel. */
      e_prackRequired   /** Add 100rel to Require header in outgoing INVITE. For
                            incoming INVITE enable PRACK is either Supported or
                            Require headers include 100rel, fail the call with
                            a 421 Extension Required if missing. */
    };
    /**Get active PRACK mode. See PRACKMode enum for details.
      */
    SIPConnection::PRACKMode GetPRACKMode() const;
	
	/** Return a bit mask of the allowed SIP methods.
      */
    virtual unsigned GetAllowedMethods() const;

    /**Call when SIP INFO of type application/media_control+xml is received.

       Return false if default reponse of Failure_UnsupportedMediaType is to be returned

      */
    virtual PBoolean OnMediaControlXML(SIP_PDU & pdu);

    virtual void OnMediaCommand(OpalMediaCommand & note, INT extra);

    virtual void OnStartTransaction(SIPTransaction & transaction);

    virtual void OnReceivedMESSAGE(SIP_PDU & pdu);

    PString GetLocalPartyURL() const;

  protected:
    virtual bool GarbageCollection();

    virtual bool OnSendOfferSDP(
      OpalRTPSessionManager & rtpSessions,
      SDPSessionDescription & sdpOut,
      bool offerCurrentOnly
    );
    virtual bool OnSendOfferSDPSession(
      const OpalMediaType & mediaType,
      unsigned sessionID,
      OpalRTPSessionManager & rtpSessions,
      SDPSessionDescription & sdpOut,
      bool offerOpenMediaStreamOnly
    );

    virtual bool OnSendAnswerSDP(
      OpalRTPSessionManager & rtpSessions,
      SDPSessionDescription & sdpOut
    );
    virtual bool OnSendAnswerSDPSession(
      const SDPSessionDescription & sdpIn,
      unsigned sessionIndex,
      SDPSessionDescription & sdpOut
    );

    virtual void OnReceivedAnswerSDP(
      SIP_PDU & pdu
    );
    virtual bool OnReceivedAnswerSDPSession(
      SDPSessionDescription & sdp,
      unsigned sessionId,
      bool & multipleFormats
    );

    virtual OpalMediaSession * SetUpMediaSession(
      const unsigned rtpSessionId,
      const OpalMediaType & mediaType,
      const SDPMediaDescription & mediaDescription,
      OpalTransportAddress & localAddress,
      bool & remoteChanged
    );

    bool SendReINVITE(const char * msg);
    void StartPendingReINVITE();

    static PBoolean WriteINVITE(OpalTransport & transport, void * param);
    bool WriteINVITE();

    virtual bool SendInviteOK();
    virtual PBoolean SendInviteResponse(
      SIP_PDU::StatusCodes code,
      const SDPSessionDescription * sdp = NULL
    );
    virtual void AdjustInviteResponse(
      SIP_PDU & response
    );

    void UpdateRemoteAddresses();

    void NotifyDialogState(
      SIPDialogNotification::States state,
      SIPDialogNotification::Events eventType = SIPDialogNotification::NoEvent,
      unsigned eventCode = 0
    );

    enum HoldState {
      eHoldOff,
      eRetrieveInProgress,

      // Order is important!
      eHoldOn,
      eHoldInProgress
    };
};


/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class SIP_RTP_Session : RTP_UserData
{

  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    SIP_RTP_Session(
      SIPConnection & connection  ///<  Owner of the RTP session
    );
  //@}

  /**@name Overrides from RTP_UserData */
  //@{
    /**Callback from the RTP session for transmit statistics monitoring.
       This is called every RTP_Session::senderReportInterval packets on the
       transmitter indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnTxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session after an IntraFrameRequest is receieved.
       The default behaviour executes an OpalVideoUpdatePicture command on the
       connection's source video stream if it exists.
      */
    virtual void OnRxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session after an IntraFrameRequest is sent.
       The default behaviour does nothing.
      */
    virtual void OnTxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;
  //@}

    virtual void SessionFailing(RTP_Session & /*session*/);
};



// End of File ///////////////////////////////////////////////////////////////
