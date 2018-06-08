/*!
 * bfcpinterface.cxx
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

#include <ptlib.h>

#include <sdp/bfcpinterface.h>

#if OPAL_BFCP

#include <opal/endpoint.h>
#include <sdp/sdp.h>

#include "./BFCP/libbfcp/BFCP_fsm.h"
#include "./BFCP/libbfcp/BFCPconnection.h"
#include "./BFCP/libbfcp/bfcpsrvctl/bfcpcli/bfcp_participant.h"
#include "./BFCP/libbfcp/bfcpsrvctl/bfcpsrv/bfcp_server.h"


#define CLIENT_CONF_ID 1
#define CLIENT_FLOOR_ID 1
#define STREAM_ID 3

#define REMOTE_CLIENT_USER_ID 2
#define LOCAL_CLIENT_USER_ID  3

#define BFCP_OVER_UDP_SOCKET_OFFSET 10
#define BFCPAPI_SUCCESS 1
#define BFCPAPI_ERROR 0

#define PTraceModule() "BFCP"


/////////////////////////////////////////////////////////////////////////

BFCPDescription::BFCPDescription()
  : m_floorCtrl(FloorCtrlUnknown)
  , m_confID(CLIENT_CONF_ID)
  , m_userID(REMOTE_CLIENT_USER_ID)
  , m_transportProto(BFCP_OVER_TCP)
{
  m_floorStreamMap[CLIENT_FLOOR_ID] = 0;
}


unsigned BFCPDescription::SetFloorStreamMapping(unsigned floorId, unsigned streamId)
{
  if (floorId > 0) {
    m_floorStreamMap[floorId] = streamId;
    return floorId;
  }

  for (FloorStreamMap::iterator it = m_floorStreamMap.begin(); it != m_floorStreamMap.end(); ++it) {
    if (it->second == 0) {
      it->second = streamId;
      return it->first;
    }
  }

  floorId = 1;
  while (m_floorStreamMap.find(floorId) != m_floorStreamMap.end())
    ++floorId;

  m_floorStreamMap[floorId] = streamId;
  return floorId;
}


const PCaselessString & BFCPDescription::GetTransportProto() const
{
  switch (m_transportProto) {
    default :
    case BFCP_OVER_TCP :
      return BFCPSession::TCP_BFCP();
    case BFCP_OVER_UDP:
      return BFCPSession::UDP_BFCP();
#if OPAL_PTLIB_SSL
    case BFCP_OVER_TLS :
      return BFCPSession::TCP_TLS_BFCP();
    case BFCP_OVER_DTLS :
      return BFCPSession::UDP_TLS_BFCP();
#endif
  }
}


bool BFCPDescription::SetTransportProto(const PString & transport)
{
  if (BFCPSession::TCP_BFCP() == transport)
    m_transportProto = BFCP_OVER_TCP;
  else if (BFCPSession::UDP_BFCP() == transport)
    m_transportProto = BFCP_OVER_UDP;
#if OPAL_PTLIB_SSL
  else if (BFCPSession::TCP_TLS_BFCP() == transport)
    m_transportProto = BFCP_OVER_TLS;
  else if (BFCPSession::UDP_TLS_BFCP() == transport)
    m_transportProto = BFCP_OVER_DTLS;
#endif
  else
    return false;
  return true;
}


/////////////////////////////////////////////////////////////////////////

const char * BFCPMediaDefinition::Name()
{
  return "BFCP";
}


BFCPMediaDefinition::BFCPMediaDefinition()
  : OpalMediaTypeDefinition(Name(), Name(), 0, OpalMediaType::ReceiveTransmit)
{
}


PString BFCPMediaDefinition::GetSDPMediaType() const
{
  return SDPApplicationMediaDescription::TypeName();
}


bool BFCPMediaDefinition::MatchesSDP(const PCaselessString & sdpMediaType,
                                     const PCaselessString & sdpTransport,
                                     const PStringArray & sdpLines,
                                     PINDEX index)
{
  if (!OpalMediaTypeDefinition::MatchesSDP(sdpMediaType, sdpTransport, sdpLines, index))
    return false;

  return sdpTransport.Find("/BFCP") != P_MAX_INDEX;
}


SDPMediaDescription * BFCPMediaDefinition::CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
{
  return new BFCPSDPMediaDescription(localAddress);
}

OPAL_MEDIATYPE(BFCPMedia);


///////////////////////////////////////////////////////////////////////

BFCPMediaFormat::Internal::Internal()
  : OpalMediaFormatInternal(BFCPMediaDefinition::Name(),
                            BFCPMediaDefinition::Name(),
                            RTP_DataFrame::MaxPayloadType,
                            NULL, false, 0, 0, 0, 0, 0, false)
{
}

const OpalMediaFormat & BFCPMediaFormat::Get()
{
  static OpalMediaFormatStatic<BFCPMediaFormat> bfcp(new BFCPMediaFormat::Internal());
  return bfcp;
};


///////////////////////////////////////////////////////////////////////

BFCPSDPMediaDescription::BFCPSDPMediaDescription(const OpalTransportAddress & localAddress)
  : SDPApplicationMediaDescription(localAddress)
{
}


PCaselessString BFCPSDPMediaDescription::GetSDPTransportType() const
{
  return GetTransportProto();
}


void BFCPSDPMediaDescription::SetSDPTransportType(const PString & type)
{
  PAssert(SetTransportProto(type), PInvalidParameter);
}


bool BFCPSDPMediaDescription::FromSession(OpalMediaSession * session, const SDPMediaDescription * offer, RTP_SyncSourceId ssrc)
{
  if (!SDPApplicationMediaDescription::FromSession(session, offer, ssrc))
    return false;

  BFCPSession * bfcpSession = dynamic_cast<BFCPSession *>(session);
  if (bfcpSession == NULL)
    return false;

  *static_cast<BFCPDescription *>(this) = *bfcpSession;
  return true;
}


bool BFCPSDPMediaDescription::ToSession(OpalMediaSession * session, RTP_SyncSourceArray & ssrcs) const
{
  if (!SDPApplicationMediaDescription::ToSession(session, ssrcs))
    return false;

  BFCPSession * bfcpSession = dynamic_cast<BFCPSession *>(session);
  if (bfcpSession == NULL)
    return false;

  // Set various options via common ancestor class
  *static_cast<BFCPDescription *>(bfcpSession) = *this;
  return true;
}


void BFCPSDPMediaDescription::OutputAttributes(ostream & strm) const
{
  SDPApplicationMediaDescription::OutputAttributes(strm);

  strm << "a=floorctrl:";
  for (FloorCtrlMode mode = FloorCtrlMode::Begin(); mode < FloorCtrlMode::End(); ++mode) {
    switch ((m_floorCtrl*mode).AsBits()) {
      case FloorCtrlClientOnly:
        strm << "c-only ";
        break;
      case FloorCtrlServerOnly:
        strm << "s-only ";
        break;
      case FloorCtrlClientServer:
        strm << "c-s ";
        break;
      default:
        break;
    }
  }
  strm << "\r\n"
          "a=confid:" << m_confID << "\r\n"
          "a=userid:" << m_userID << "\r\n";
  for (FloorStreamMap::const_iterator it = m_floorStreamMap.begin(); it != m_floorStreamMap.end(); ++it) {
    strm << "a=floorid:" << it->first;
    if (it->second > 0)
      strm << " mstrm:" << it->second;
    strm << "\r\n";
  }
}


void BFCPSDPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  if (attr *= "floorctrl") {
    m_floorCtrl = FloorCtrlUnknown;
    if (value.Find("c-only") != P_MAX_INDEX)
      m_floorCtrl |= FloorCtrlClientOnly;
    if (value.Find("s-only") != P_MAX_INDEX)
      m_floorCtrl |= FloorCtrlServerOnly;
    if (value.Find("c-s") != P_MAX_INDEX)
      m_floorCtrl |= FloorCtrlClientServer;
    return;
  }

  if (attr *= "confid") {
    m_confID = value.AsUnsigned();
    return;
  }

  if (attr *= "userid") {
    m_userID = value.AsUnsigned();
    return;
  }

  if (attr *= "floorid") {
    PStringArray tokens = value.Tokenise(" ", false);
    if (tokens.IsEmpty())
      return;

    unsigned floorID = tokens[0].AsUnsigned();
    unsigned streamID = 0;
    if (tokens.size() > 1) {
      PCaselessString token = tokens[1];
      if (token.NumCompare("mstrm:") != EqualTo && token.NumCompare("m-stream:") != EqualTo)
        return;
      streamID = token.Mid(token.Find(':')+1).AsUnsigned();
    }

    m_floorStreamMap[floorID] = streamID;
    return;
  }

  SDPApplicationMediaDescription::SetAttribute(attr, value);
}


/////////////////////////////////////////////////////////////////////////

static BFCPCallback::BFCPEvent FromInternalEvent(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  BFCPCallback::BFCPEvent evt;
  evt.m_state = p_evt->State;
  evt.m_transactionID = p_evt->TransactionID;
  evt.m_userID = p_evt->userID;
  evt.m_conferenceID = p_evt->conferenceID;
  evt.m_floorID = p_evt->FloorID;
  evt.m_floorRequestID = p_evt->FloorRequestID;
  evt.m_queuePosition = p_evt->QueuePosition;
  evt.m_beneficiaryID = p_evt->BeneficiaryID;
  return evt;
}


/////////////////////////////////////////////////////////////////////////

class BFCPSession::LibServerWrapper : public PObject, public BFCP_Server::ServerEvent
{
public:
  BFCPSession & m_session;
  BFCP_Server * m_BFCPServer;

  LibServerWrapper(BFCPSession & session);
  virtual ~LibServerWrapper();

  bool StopServer();

  /**
    *  \brief Floor control server callback for BFCP events and respons
    *
    * This callback is called when server received an event or connection.
    * p_evt is the current event and p_FsmEvent is the context and formations of event
    * @param p_evt \ref BFCP_fsm::e_BFCP_ACT current event of BFCP server
    * @param p_FsmEvent \ref  BFCP_fsm::st_BFCP_fsm_event formations struct of events
    * @return  true success , false failed
    */
  virtual bool OnBfcpServerEvent(BFCP_fsm::e_BFCP_ACT p_evt, BFCP_fsm::st_BFCP_fsm_event* p_FsmEvent);

#if PTRACING
  virtual void Log(const  char* pcFile, int iLine, int iErrorLevel, const  char* pcFormat, va_list args)
  {
    if (PTrace::CanTrace(iErrorLevel))
      PTrace::Begin(iErrorLevel, pcFile, iLine, this, PTraceModule()) << pvsprintf(pcFormat, args) << PTrace::End;
  }
#endif // PTRACING
};


BFCPSession::LibServerWrapper::LibServerWrapper(BFCPSession & session)
  : m_session(session)
  , m_BFCPServer(NULL)
{
}


BFCPSession::LibServerWrapper::~LibServerWrapper()
{
  StopServer();
}


bool BFCPSession::LibServerWrapper::StopServer()
{
  if (m_BFCPServer != NULL) {
    m_BFCPServer->disconnect();
    delete m_BFCPServer;
    m_BFCPServer = NULL;
  }

  return true;
}

bool BFCPSession::LibServerWrapper::OnBfcpServerEvent(BFCP_fsm::e_BFCP_ACT p_evt, BFCP_fsm::st_BFCP_fsm_event* p_FsmEvent)
{
  bool Status = false;
  if (!p_FsmEvent)
    return Status;

  PTRACE(4, " BFCP evt[" << getBfcpFsmAct(p_evt) << "] state[" << getBfcpFsmAct(p_FsmEvent->State) << "]");

  switch (p_evt) {
    case BFCP_fsm::BFCP_ACT_UNDEFINED:
      break;
    case BFCP_fsm::BFCP_ACT_INITIALIZED:
      break;
    case BFCP_fsm::BFCP_ACT_DISCONNECTED:
      /* participant have closed network connection */
      break;
    case BFCP_fsm::BFCP_ACT_CONNECTED:
      /*  participant are connected */
      break;
    case BFCP_fsm::BFCP_ACT_FloorRequest:
      /*  participant have send BFCP floor request  */
      /*  In this school sample with automatic accept and granted this request */
      /*  if another participant have the floor , we send revoked status */
      /*  remarks , bfcp library auto send pending request */
      if (m_BFCPServer) {
        e_bfcp_status bfcp_status = BFCP_PENDING;
        uint32_t userID = 0;
        uint32_t beneficiaryID = 0;
        uint16_t floorRequestID = 0;
        /* check FloorState */
        if (m_BFCPServer->GetFloorState(&bfcp_status, &userID, &beneficiaryID, &floorRequestID)) {
          if (bfcp_status == BFCP_GRANTED && userID) {
            /* revoke current GRANTED bfcp request */
            m_BFCPServer->FloorRequestRespons(userID, beneficiaryID,
                                              p_FsmEvent->TransactionID, floorRequestID,
                                              BFCP_REVOKED, 0, BFCP_NORMAL_PRIORITY, false);
          }
        }

        /* accept new request */
        Status = m_BFCPServer->FloorRequestRespons(p_FsmEvent->userID,
                                                    p_FsmEvent->userID, p_FsmEvent->TransactionID,
                                                    p_FsmEvent->FloorRequestID, BFCP_ACCEPTED,
                                                    0, BFCP_NORMAL_PRIORITY, false);
        /* and granted new request */
        if (Status) {
          Status = m_BFCPServer->FloorRequestRespons(p_FsmEvent->userID, p_FsmEvent->userID,
                                                      p_FsmEvent->TransactionID, p_FsmEvent->FloorRequestID, BFCP_GRANTED, 0, BFCP_NORMAL_PRIORITY, true);
        }
      }
      else {
        PTRACE(4, "BFCP Server is not created so cannot respond of Floor request.");
      }
      break;
    case BFCP_fsm::BFCP_ACT_FloorRelease:
      /* participant have send floor release request */
      if (m_BFCPServer) {
        /* alert all particpant of the new state */
        Status = m_BFCPServer->SendFloorStatus(0, 0, 0, NULL, true);
      }
      break;
    case BFCP_fsm::BFCP_ACT_FloorRequestQuery:
      /* actually this request are not supported , you shoould never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_UserQuery:
      /* actually this request are not supported , you shoould never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_FloorQuery:
      /* actually this request are not supported , you shoould never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_ChairAction:
      /* actually this request are not supported , you shoould never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_Hello:
      /*  FCS have received an hello message , libbfcp have automatically send helloAck   */
      break;
    case BFCP_fsm::BFCP_ACT_Error:
      /*  FCS have received an Error message */
      break;
    case BFCP_fsm::BFCP_ACT_FloorRequestStatusAccepted:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_FloorRequestStatusAborted:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_FloorRequestStatusGranted:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_UserStatus:
      /* actually this request are not supported , you shoould never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_FloorStatusAccepted:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_FloorStatusAborted:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_FloorStatusGranted:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_ChairActionAck:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_HelloAck:
      /* Here we are not sending FlooeStatus as some endpoints are not able to treat it properly and stops screen share.*/
#if 0
    /*  FCS have send an helloAck  message , send floor status to form status of conference */
      if (m_BFCPServer) {
        Status = m_BFCPServer->SendFloorStatus(p_FsmEvent->userID, p_FsmEvent->BeneficiaryID, 0, NULL, false);
      }
#endif
    case BFCP_fsm::BFCP_ACT_SHARE:
      /* not valid on server  , you should never receive this event */
      break;
    case BFCP_fsm::BFCP_ACT_SERVER_SHARE:
      /* not valid on server  , you should never receive this event */
      break;
    default:
      break;
  }
  return Status;
}


///////////////////////////////////////////////////////////////////////

class BFCPSession::LibParticipantWrapper : public PObject, public BFCP_Participant::ParticipantEvent
{
public:
  BFCPSession & m_session;
  BFCPCallback * m_callback;
  BFCP_Participant * m_BFCPParticipant;

  PDECLARE_NOTIFIER(PTimer, LibParticipantWrapper, UDPKeepAlive);
  PTimer m_keepAliveTimer;

  LibParticipantWrapper(BFCPSession & session, BFCPCallback * callback);

  virtual bool OnBfcpParticipantEvent(BFCP_fsm::e_BFCP_ACT p_evt, BFCP_fsm::st_BFCP_fsm_event* p_FsmEvent);

  bool connected();
  bool disconnected();
  bool floorRequestStatusAccepted(BFCP_fsm::st_BFCP_fsm_event* p_evt);
  bool floorRequestStatusGranted(BFCP_fsm::st_BFCP_fsm_event* p_evt);
  bool floorRequestStatusAborted(BFCP_fsm::st_BFCP_fsm_event* p_evt);
  bool floorStatusAccepted(BFCP_fsm::st_BFCP_fsm_event* p_evt);
  bool floorStatusGranted(BFCP_fsm::st_BFCP_fsm_event* p_evt);
  bool floorStatusAborted(BFCP_fsm::st_BFCP_fsm_event* p_evt);
  bool floorReleaseStatusAborted(BFCP_fsm::st_BFCP_fsm_event* p_evt);

#if PTRACING
  virtual void Log(const  char* pcFile, int iLine, int iErrorLevel, const  char* pcFormat, va_list args)
  {
    if (PTrace::CanTrace(iErrorLevel))
      PTrace::Begin(iErrorLevel, pcFile, iLine, this, PTraceModule()) << pvsprintf(pcFormat, args) << PTrace::End;
  }
#endif // PTRACING
};


BFCPSession::LibParticipantWrapper::LibParticipantWrapper(BFCPSession & session, BFCPCallback * callback)
  : m_session(session)
  , m_callback(callback)
  , m_BFCPParticipant(NULL)
{
  m_keepAliveTimer.SetNotifier(PCREATE_NOTIFIER(UDPKeepAlive));
  m_keepAliveTimer.RunContinuous(5000);
}


void BFCPSession::LibParticipantWrapper::UDPKeepAlive(PTimer &, P_INT_PTR)
{
  if (m_BFCPParticipant != NULL)
    m_BFCPParticipant->bfcp_hello_participant(NULL);
}


// FSM server callback exit function ( if transaction success )
bool BFCPSession::LibParticipantWrapper::OnBfcpParticipantEvent(BFCP_fsm::e_BFCP_ACT p_evt ,BFCP_fsm::st_BFCP_fsm_event* p_FsmEvent )
{
  bool Status = false ;
  if ( !p_FsmEvent )
    return Status ;

  PTRACE(4, "BFCP evt[" << getBfcpFsmAct(p_evt)
      << "] state[" << getBfcpFsmAct(p_FsmEvent->State) << "]" );

  switch ( p_evt ) {
    case BFCP_fsm::BFCP_ACT_UNDEFINED :
      break ;
    case BFCP_fsm::BFCP_ACT_INITIALIZED  :
      break ;
    case BFCP_fsm::BFCP_ACT_DISCONNECTED  :
      Status = disconnected() ;
      break ;
    case BFCP_fsm::BFCP_ACT_CONNECTED :
      Status = connected() ;
      break ;
    case BFCP_fsm::BFCP_ACT_FloorRequest :
      break ;
    case BFCP_fsm::BFCP_ACT_FloorRelease  :
      break ;
    case BFCP_fsm::BFCP_ACT_FloorRequestQuery :
      break ;
    case BFCP_fsm::BFCP_ACT_UserQuery  :
      break ;
    case BFCP_fsm::BFCP_ACT_FloorQuery  :
      break ;
    case BFCP_fsm::BFCP_ACT_ChairAction  :
      break ;
    case BFCP_fsm::BFCP_ACT_Hello :
      break ;
    case BFCP_fsm::BFCP_ACT_Error :
      break ;
    case BFCP_fsm::BFCP_ACT_FloorRequestStatusAccepted :
      Status = floorRequestStatusAccepted(p_FsmEvent) ;
      break ;
    case BFCP_fsm::BFCP_ACT_FloorRequestStatusAborted :
      Status = floorRequestStatusAborted(p_FsmEvent);
      break ;
    case BFCP_fsm::BFCP_ACT_FloorRequestStatusGranted  :
      Status = floorRequestStatusGranted(p_FsmEvent) ;
      break ;
    case BFCP_fsm::BFCP_ACT_UserStatus :
      break ;
    case BFCP_fsm::BFCP_ACT_FloorStatusAccepted :
      Status = floorStatusAccepted(p_FsmEvent) ;
      break ;
    case BFCP_fsm::BFCP_ACT_FloorStatusAborted :
      Status = floorStatusAborted(p_FsmEvent)  ;
      break ;
    case BFCP_fsm::BFCP_ACT_FloorStatusGranted  :
      Status = floorStatusGranted(p_FsmEvent) ;
      break ;
    case BFCP_fsm::BFCP_ACT_ChairActionAck :
      break ;
    case BFCP_fsm::BFCP_ACT_HelloAck  :
      break ;
    case BFCP_fsm::BFCP_ACT_SHARE :
      Status = true ;
      break ;
    case BFCP_fsm::BFCP_ACT_SERVER_SHARE :
      Status = true ;
      break ;
    default :
      break ;
  }

  return Status ;
}

//@private methods
bool BFCPSession::LibParticipantWrapper::connected()
{
  m_callback->OnBfcpConnected();
  return true ;
}

bool BFCPSession::LibParticipantWrapper::disconnected()
{
  m_callback->OnBfcpDisconnected();
  return true ;
}

/*
   +-------+-----------+
   | Value | Status    |
   +-------+-----------+
   |   1   | Pending   |  floorRequestStatusAccepted
   |   2   | Accepted  |  floorRequestStatusAccepted
   |   3   | Granted   |  floorRequestStatusGranted
   |   4   | Denied    |  floorRequestStatusAborted
   |   5   | Cancelled |  floorRequestStatusAborted
   |   6   | Released  |  floorRequestStatusAborted
   |   7   | Revoked   |  floorRequestStatusAborted
   +-------+-----------+
 */

bool BFCPSession::LibParticipantWrapper::floorRequestStatusGranted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false ;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP floorRequestStatusGranted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    m_session.m_floorOwnerID = p_evt->BeneficiaryID;
    m_callback->OnFloorRequestStatusGranted(FromInternalEvent(p_evt));
    return true;
  }

  PTRACE(1, "BFCP floorRequestStatusGranted: eventFloorId=" << p_evt->FloorID << ", UserID="<< p_evt->userID );
  return false;
}

bool BFCPSession::LibParticipantWrapper::floorRequestStatusAborted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP floorRequestStatusAborted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    m_callback->OnFloorRequestStatusAborted(FromInternalEvent(p_evt));
    return true;
  }

  PTRACE(1, "BFCP floorRequestStatusAborted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
  return false;
}

bool BFCPSession::LibParticipantWrapper::floorRequestStatusAccepted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP floorRequestStatusAccepted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    m_callback->OnFloorRequestStatusAccepted(FromInternalEvent(p_evt));
    return true;
  }

  PTRACE(1, "BFCP floorRequestStatusAccepted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
  return false;
}

bool BFCPSession::LibParticipantWrapper::floorReleaseStatusAborted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP floorReleaseStatusAborted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    return true;
  }

  PTRACE(1, "BFCP floorReleaseStatusAborted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
  return false;
}

bool BFCPSession::LibParticipantWrapper::floorStatusAccepted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP FloorStatusAccepted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    m_callback->OnFloorStatusAccepted(FromInternalEvent(p_evt));
    return true;
  }

  PTRACE(1, "BFCP FloorStatusAccepted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
  return false;
}

bool BFCPSession::LibParticipantWrapper::floorStatusGranted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP FloorStatusGranted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    m_session.m_floorOwnerID = p_evt->BeneficiaryID;
    m_callback->OnFloorStatusGranted(FromInternalEvent(p_evt));
    return true;
  }

  PTRACE(1, "BFCP FloorStatusGranted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
  return false;
}

bool BFCPSession::LibParticipantWrapper::floorStatusAborted(BFCP_fsm::st_BFCP_fsm_event* p_evt)
{
  if (!p_evt)
    return false;

  if (m_session.HasFloorID(p_evt->FloorID)) {
    PTRACE(4, "BFCP FloorRequestAborted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
    m_callback->OnFloorStatusAborted(FromInternalEvent(p_evt));
    return true;
  }

  PTRACE(1, "BFCP FloorRequestAborted: eventFloorId=" << p_evt->FloorID << ", UserID=" << p_evt->userID);
  return false;
}


///////////////////////////////////////////////////////////////////////

const PCaselessString & BFCPSession::TCP_BFCP() { static const PConstCaselessString s("TCP/BFCP" ); return s; }
const PCaselessString & BFCPSession::UDP_BFCP() { static const PConstCaselessString s("UDP/BFCP" ); return s; }
#if OPAL_PTLIB_SSL
const PCaselessString & BFCPSession::TCP_TLS_BFCP() { static const PConstCaselessString s("TCP/TLS/BFCP" ); return s; }
const PCaselessString & BFCPSession::UDP_TLS_BFCP() { static const PConstCaselessString s("UDP/TLS/BFCP" ); return s; }
#endif

BFCPSession::BFCPSession(const Init & init, BFCPCallback * callback)
  : OpalMediaSession(init)
  , m_server(new LibServerWrapper(*this))
  , m_participant(new LibParticipantWrapper(*this, callback))
  , m_IsPassive(false)
  , m_localIP(PIPSocket::GetInvalidAddress())
  , m_localPort(0)
  , m_remoteIP(PIPSocket::GetInvalidAddress())
  , m_remotePort(0)
  , m_clientServerStarted(false)
  , m_floorOwnerID(0)
{
}


BFCPSession::~BFCPSession()
{
  PTRACE(4, "BFCPSession instance destroy.");

  delete m_participant;

  if (m_server != NULL && m_server->m_BFCPServer != NULL) {
    m_server->m_BFCPServer->RemoveUserInConf(LOCAL_CLIENT_USER_ID);
    m_server->m_BFCPServer->RemoveUserInConf(REMOTE_CLIENT_USER_ID);
    delete m_server;
  }
}


const PCaselessString & BFCPSession::GetSessionType() const
{
  return GetTransportProto();
}


bool BFCPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress)
{
  // Before we start client, we must have server. So, let's create one.
  if (m_clientServerStarted && GetRemoteAddress() == remoteAddress)
    return true;

  if (m_floorStreamMap.empty()) {
    PTRACE(4, "Cannot open without floor ID");
    return false;
  }

  m_localIP = localInterface;

  if (!remoteAddress.GetIpAndPort(m_remoteIP, m_remotePort))
    return false;

  m_server->StopServer();

  m_server->m_BFCPServer = new BFCP_Server(BFCP_MAX_CONF,
                                           m_confID,
                                           m_userID,
                                           m_floorStreamMap.begin()->first,
                                           m_floorStreamMap.begin()->second,
                                           m_server,
                                           m_transportProto);
  m_participant->m_BFCPParticipant = new BFCP_Participant(m_confID,
                                                          (UINT16)m_userID,
                                                          (UINT16)m_floorStreamMap.begin()->first,
                                                          (UINT16)m_floorStreamMap.begin()->second,
                                                          m_participant,
                                                          m_transportProto);

  m_IsPassive = m_remotePort == 0;

  if (m_IsPassive) {
    m_floorCtrl = FloorCtrlServerOnly;
    const PIPSocket::PortRange & portRange = m_connection.GetEndPoint().GetManager().GetTCPPortRange();
    m_localPort = portRange.GetBase();
    if (m_localPort == 0)
      m_localPort = 6000;
    while (!m_server->m_BFCPServer->OpenTcpConnection(m_localIP.AsString(), m_localPort,
                                                      m_remoteIP.AsString(), m_remotePort,
                                                      BFCPConnectionRole::PASSIVE)) {
      ++m_localPort;
      if (m_localPort == 0 || (portRange.GetMax() != 0 && m_localPort > portRange.GetMax())) {
        PTRACE(2, "Could not open BFCP passive listener on port range " << portRange);
        return false;
      }
    }
  }
  else {
    m_floorCtrl = FloorCtrlServerOnly; // Shouldn't this be FloorCtrlClientOnly ?

    if (!m_server->m_BFCPServer->OpenTcpConnection(m_localIP.AsString(), m_localPort,
                                                   m_remoteIP.AsString(), m_remotePort,
                                                   BFCPConnectionRole::ACTIVE)) {
      PTRACE(2, "Could not open BFCP active connection to " << remoteAddress);
      return false;
    }
  }

  m_server->m_BFCPServer->AddUser(LOCAL_CLIENT_USER_ID);

  switch (m_transportProto) {
    default:
      PTRACE(2, "Unsupported transport");
      return false;

    case BFCP_OVER_TCP:
      m_connectionMode = ConnectionNew;
      if (!m_participant->m_BFCPParticipant->OpenTcpConnection(m_localIP.AsString(), m_localPort,
                                                               m_remoteIP.AsString(), m_remotePort,
                                                               m_IsPassive ? BFCPConnectionRole::PASSIVE : BFCPConnectionRole::ACTIVE)) {
        PTRACE(2, "Participant connection failed ");
        return false;
      }
      break;

    case BFCP_OVER_UDP:
      char udpInterface[100];
      if (m_localIP.IsValid() && !m_localIP.IsAny())
        strcpy(udpInterface, m_localIP.AsString());
      else
        udpInterface[0] = '\0';
      m_server->m_BFCPServer->OpenUdpConnection(REMOTE_CLIENT_USER_ID, udpInterface, m_localPort);
      m_server->m_BFCPServer->OpenUdpConnection(LOCAL_CLIENT_USER_ID, udpInterface, m_localPort + BFCP_OVER_UDP_SOCKET_OFFSET);

      m_localIP.FromString(udpInterface);
      if (m_participant->m_BFCPParticipant->OpenTcpConnection(m_localIP.AsString(),
                                                              m_localPort + 2/* Virtual Client Port which is 2 more than that of servers*/,
                                                              m_remoteIP.AsString(),
                                                              m_localPort + BFCP_OVER_UDP_SOCKET_OFFSET,/* BFCP Server port*/
                                                              m_IsPassive ? BFCPConnectionRole::PASSIVE : BFCPConnectionRole::ACTIVE)) {
        PTRACE(2, "Participant virtual connection failed ");
        return false;
      }
  }

  PTRACE(4, "Participant started successfully");
  m_clientServerStarted = true;
  return true;
}


OpalTransportAddress BFCPSession::GetLocalAddress(bool /*isMediaAddress*/) const
{
  switch (m_transportProto) {
    case BFCP_OVER_TCP:
      return OpalTransportAddress(m_localIP, m_localPort, OpalTransportAddress::TcpPrefix());
    case BFCP_OVER_UDP:
      return OpalTransportAddress(m_localIP, m_localPort, OpalTransportAddress::UdpPrefix());
#if OPAL_PTLIB_SSL
    case BFCP_OVER_TLS:
      return OpalTransportAddress(m_localIP, m_localPort, OpalTransportAddress::TlsPrefix());
#endif
    default :
      return OpalTransportAddress();
  }
}


OpalTransportAddress BFCPSession::GetRemoteAddress(bool /*isMediaAddress*/) const
{
  switch (m_transportProto) {
    case BFCP_OVER_TCP:
      return OpalTransportAddress(m_remoteIP, m_remotePort, OpalTransportAddress::TcpPrefix());
    case BFCP_OVER_UDP:
      return OpalTransportAddress(m_remoteIP, m_remotePort, OpalTransportAddress::UdpPrefix());
#if OPAL_PTLIB_SSL
    case BFCP_OVER_TLS:
      return OpalTransportAddress(m_remoteIP, m_remotePort, OpalTransportAddress::TlsPrefix());
#endif
    default :
      return OpalTransportAddress();
  }
}


bool BFCPSession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool /*isMediaAddress*/)
{
  return remoteAddress.GetIpAndPort(m_remoteIP, m_remotePort);
}


SDPMediaDescription * BFCPSession::CreateSDPMediaDescription()
{
  return new BFCPSDPMediaDescription(m_localIP);
}


OpalMediaStream * BFCPSession::CreateMediaStream(const OpalMediaFormat & /*mediaFormat*/, unsigned /*sessionID*/, bool /*isSource*/)
{
  return NULL;
}


OpalMediaSession::SetUpMode BFCPSession::GetSetUpMode() const
{
  return m_IsPassive ? SetUpModePassive : SetUpModeActive;
}


void BFCPSession::SetSetUpMode(SetUpMode mode)
{
  m_IsPassive = mode != SetUpModeActive;
}


#endif // OPAL_BFCP
