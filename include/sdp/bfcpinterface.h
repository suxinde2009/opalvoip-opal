/*!
 * bfcpinterface.h
 *
 * Binary Floor Control Protocol interface
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2017-2018 Fuze, Inc. All rights reserved
 * Copyright (c) 2017-218 Great Software Laboratory. All rights reserved.
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
 * The Initial Developer of the Original Code is Sandeepkumar Sharma
 *
 * Contributor(s): Fuze Pvt Ltd, www.fuze.com
 *  		       GSLab Pvt Ltd, www.gslab.com
 *                 Pradeep Kumar, Sagar Joshi, Ammar Nasikwala 
 *                 Robert Jongbloed, Vox Lucida Pty. Ltd. 
 */

#ifndef OPAL_BFCP_INTERFACE_H
#define OPAL_BFCP_INTERFACE_H

#include <opal_config.h>

#if OPAL_BFCP

#include <opal/mediafmt.h>
#include <sdp/sdp.h>


class BFCPDescription
{
  public:
    P_DECLARE_BITWISE_ENUM(FloorCtrlMode, 3, (
      FloorCtrlUnknown,
	  FloorCtrlClientOnly,
      FloorCtrlServerOnly,
	  FloorCtrlClientServer
    ));

    BFCPDescription();

	unsigned GetConfID() const { return m_confID; }
	void SetConfID(unsigned p_confID) { m_confID = p_confID; }

	unsigned GetUserID() const { return m_userID; }
	void SetUserID(unsigned p_userID) {	m_userID = p_userID; }

    typedef std::map<unsigned, unsigned> FloorStreamMap;
    unsigned SetFloorStreamMapping(unsigned floorId, unsigned streamId);
	FloorStreamMap GetFloorStreamMap() const { return m_floorStreamMap; }
    bool HasFloorID(unsigned floorID) const { return m_floorStreamMap.find(floorID) != m_floorStreamMap.end(); }

    const PCaselessString & GetTransportProto() const;
    bool SetTransportProto(const PString & transport);

  protected:
    FloorCtrlMode  m_floorCtrl;
    unsigned       m_confID;
    unsigned       m_userID;
    FloorStreamMap m_floorStreamMap;
	int            m_transportProto;
};


class BFCPMediaDefinition : public OpalMediaTypeDefinition
{
  public:
    static const char * Name();

    BFCPMediaDefinition();

    virtual PString GetSDPMediaType() const;
    virtual bool MatchesSDP(const PCaselessString & sdpMediaType, const PCaselessString & sdpTransport, const PStringArray & sdpLines, PINDEX index);
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const;
};


class BFCPMediaFormat : public OpalMediaFormat
{
    PCLASSINFO(BFCPMediaFormat, OpalMediaFormat);
  public:
    class Internal : public OpalMediaFormatInternal
    {
        PCLASSINFO(Internal, OpalMediaFormatInternal);
      public:
        Internal();
    };

    BFCPMediaFormat(Internal * info, bool dynamic) : OpalMediaFormat(info, dynamic) { }
    BFCPMediaFormat(const PString & name) : OpalMediaFormat(name) { }
    static const OpalMediaFormat & Get();
};


class BFCPSDPMediaDescription : public SDPApplicationMediaDescription, BFCPDescription
{
  public:
    BFCPSDPMediaDescription(const OpalTransportAddress & localAddress);
    virtual PCaselessString GetSDPTransportType() const;
    virtual void SetSDPTransportType(const PString & type);
    virtual bool FromSession(OpalMediaSession * session, const SDPMediaDescription * offer, RTP_SyncSourceId ssrc);
    virtual bool ToSession(OpalMediaSession * session, RTP_SyncSourceArray & ssrcs) const;
    virtual void OutputAttributes(ostream & str) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
};


class BFCPCallback
{
public :
	virtual ~BFCPCallback(){}

	virtual bool OnBfcpConnected() = 0;
	virtual bool OnBfcpDisconnected() = 0;

    struct BFCPEvent {
      unsigned m_state;           //< Current state of FSM
      unsigned m_transactionID;   //< transaction ID of request  \ref BFCP-PROTOCOL-TRANSACTION
      unsigned m_userID;          //< user ID of request \ref BFCP-COMMON-HEADER
      unsigned m_conferenceID;    //< conference ID of request \ref BFCP-COMMON-HEADER
      unsigned m_floorID;         //< floor id of request \ref FLOOR-ID
      unsigned m_floorRequestID;  //< floor request id \ref FLOOR-REQUEST-ID
      unsigned m_queuePosition;   //< queue position \ref  REQUEST-STATUS
      unsigned m_beneficiaryID;   //< benficiary ID of request  \ref BENEFICIARY-ID
    };

	virtual bool OnFloorRequestStatusAccepted(const BFCPEvent & evt) = 0;
	virtual bool OnFloorRequestStatusGranted(const BFCPEvent & evt) = 0;
	virtual bool OnFloorRequestStatusAborted(const BFCPEvent & evt) = 0;
	virtual bool OnFloorStatusAccepted(const BFCPEvent & evt) = 0;
	virtual bool OnFloorStatusGranted(const BFCPEvent & evt) = 0;
	virtual bool OnFloorStatusAborted(const BFCPEvent & evt) = 0;
};

//! to create and get instances
class BFCPSession : public OpalMediaSession, public BFCPDescription
{
  public:
    static const PCaselessString & TCP_BFCP();
    static const PCaselessString & UDP_BFCP();
#if OPAL_PTLIB_SSL
    static const PCaselessString & TCP_TLS_BFCP();
    static const PCaselessString & UDP_TLS_BFCP();
#endif

	BFCPSession(const Init & init, BFCPCallback * callback);
	virtual ~BFCPSession();

    virtual const PCaselessString & GetSessionType() const;
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);
    virtual SDPMediaDescription * CreateSDPMediaDescription();
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource);
    virtual SetUpMode GetSetUpMode() const;
    virtual void SetSetUpMode(SetUpMode mode);

  protected:
    class LibServerWrapper;
    friend class LibServerWrapper;
    LibServerWrapper  * m_server;

    class LibParticipantWrapper;
    friend class LibParticipantWrapper;
    LibParticipantWrapper * m_participant;

	bool m_IsPassive;

	PIPAddress m_localIP;
    uint16_t   m_localPort;
	PIPAddress m_remoteIP;
    uint16_t   m_remotePort;

	bool m_clientServerStarted;
	uint16_t m_floorOwnerID;
};

#endif // OPAL_BFCP

#endif // OPAL_BFCP_INTERFACE_H
