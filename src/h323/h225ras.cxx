/*
 * H225_RAS.cxx
 *
 * H.225 RAS protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * iFace In, http://www.iface.com
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h225ras.cxx,v $
 * Revision 1.2007  2002/09/04 06:01:48  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.5  2002/07/01 04:56:32  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.4  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.3  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 * Revision 1.29  2002/08/29 06:58:37  robertj
 * Fixed (again) cached response age timeout adjusted to RIP time.
 *
 * Revision 1.28  2002/08/12 06:29:42  robertj
 * Fixed problem with cached responses being aged before the RIP time which
 *   made retries by client appear as "new" requests when they were not.
 *
 * Revision 1.27  2002/08/12 05:35:48  robertj
 * Changes to the RAS subsystem to support ability to make requests to client
 *   from gkserver without causing bottlenecks and race conditions.
 *
 * Revision 1.26  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.25  2002/08/05 05:17:41  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.24  2002/07/29 11:36:08  robertj
 * Fixed race condition if RIP is followed very quickly by actual response.
 *
 * Revision 1.23  2002/07/16 04:18:38  robertj
 * Fixed incorrect check for GRQ in reject processing, thanks Thien Nguyen
 *
 * Revision 1.22  2002/06/28 03:34:28  robertj
 * Fixed issues with address translation on gatekeeper RAS channel.
 *
 * Revision 1.21  2002/06/24 00:11:21  robertj
 * Clarified error message during GRQ authentication.
 *
 * Revision 1.20  2002/06/12 03:50:25  robertj
 * Added PrintOn function for trace output of RAS channel.
 *
 * Revision 1.19  2002/05/29 00:03:19  robertj
 * Fixed unsolicited IRR support in gk client and server,
 *   including support for IACK and INAK.
 *
 * Revision 1.18  2002/05/17 03:41:00  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.17  2002/05/03 09:18:49  robertj
 * Added automatic retransmission of RAS responses to retried requests.
 *
 * Revision 1.16  2002/03/10 19:34:13  robertj
 * Added random starting point for sequence numbers, thanks Chris Purvis
 *
 * Revision 1.15  2002/01/29 02:38:31  robertj
 * Fixed nasty race condition when getting RIP, end up with wrong timeout.
 * Improved tracing (included sequence numbers)
 *
 * Revision 1.14  2002/01/24 01:02:04  robertj
 * Removed trace when authenticator not used, implied error when wasn't one.
 *
 * Revision 1.13  2001/10/09 12:03:30  robertj
 * Fixed uninitialised variable for H.235 authentication checking.
 *
 * Revision 1.12  2001/10/09 08:04:59  robertj
 * Fixed unregistration so still unregisters if gk goes offline, thanks Chris Purvis
 *
 * Revision 1.11  2001/09/18 10:36:57  robertj
 * Allowed multiple overlapping requests in RAS channel.
 *
 * Revision 1.10  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.9  2001/09/12 03:12:38  robertj
 * Added ability to disable the checking of RAS responses against
 *   security authenticators.
 * Fixed bug in having multiple authentications if have a retry.
 *
 * Revision 1.8  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.7  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.6  2001/08/06 03:18:38  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.5  2001/08/03 05:56:04  robertj
 * Fixed RAS read of UDP when get ICMP error for host unreachabe.
 *
 * Revision 1.4  2001/06/25 01:06:40  robertj
 * Fixed resolution of RAS timeout so not rounded down to second.
 *
 * Revision 1.3  2001/06/22 00:21:10  robertj
 * Fixed bug in H.225 RAS protocol with 16 versus 32 bit sequence numbers.
 *
 * Revision 1.2  2001/06/18 07:44:21  craigs
 * Made to compile with h225ras.cxx under Linux
 *
 * Revision 1.1  2001/06/18 06:23:50  robertj
 * Split raw H.225 RAS protocol out of gatekeeper client class.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h225ras.h"
#endif

#include <h323/h225ras.h>

#include <h323/h323ep.h>
#include <h323/transaddr.h>
#include <h323/h323pdu.h>
#include <h323/h235auth.h>

#include <ptclib/random.h>


#define new PNEW


static PTimeInterval ResponseRetirementAge(0, 30); // Seconds


/////////////////////////////////////////////////////////////////////////////

H225_RAS::H225_RAS(H323EndPoint & ep, OpalTransport * trans)
  : endpoint(ep)
{
  if (trans != NULL)
    transport = trans;
  else
    transport = new OpalTransportUDP(ep, INADDR_ANY, DefaultRasUdpPort);

  checkResponseCryptoTokens = TRUE;

  lastRequest = NULL;
  nextSequenceNumber = PRandom::Number()%65536;
  requests.DisallowDeleteObjects();
}


H225_RAS::~H225_RAS()
{
  if (transport != NULL) {
    transport->CloseWait();
    delete transport;
  }
}


void H225_RAS::PrintOn(ostream & strm) const
{
  strm << "RAS<";
  if (transport != NULL)
    strm << transport->GetLocalAddress();
  else
    strm << "no-transport";
  strm << '>';
}


BOOL H225_RAS::StartRasChannel()
{
  transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(HandleRasChannel), 0,
                                          PThread::NoAutoDeleteThread,
                                          PThread::NormalPriority,
                                          "H225 RAS:%x"));
  return TRUE;
}


void H225_RAS::HandleRasChannel(PThread &, INT)
{
  transport->SetReadTimeout(PMaxTimeInterval);

  H323RasPDU response;

  for (;;) {
    PTRACE(5, "RAS\tReading PDU");
    if (response.Read(*transport)) {
      lastRequest = NULL;
      if (HandleRasPDU(response))
        lastRequest->responseHandled.Signal();
      if (lastRequest != NULL)
        lastRequest->responseMutex.Signal();
    }
    else {
      switch (transport->GetErrorNumber()) {
        case ECONNRESET :
        case ECONNREFUSED :
          PTRACE(2, "RAS\tCannot access remote " << transport->GetRemoteAddress());
          break;

        default:
          PTRACE(1, "RAS\tRead error: " << transport->GetErrorText(PChannel::LastReadError));
          return;
      }
    }
  }
}


BOOL H225_RAS::HandleRasPDU(const H323RasPDU & pdu)
{
  lastReceivedFrom = transport->GetRemoteAddress();

  AgeResponses();

  switch (pdu.GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveGatekeeperRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_gatekeeperConfirm :
      return OnReceiveGatekeeperConfirm(pdu, pdu);

    case H225_RasMessage::e_gatekeeperReject :
      return OnReceiveGatekeeperReject(pdu, pdu);

    case H225_RasMessage::e_registrationRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveRegistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationConfirm :
      return OnReceiveRegistrationConfirm(pdu, pdu);

    case H225_RasMessage::e_registrationReject :
      return OnReceiveRegistrationReject(pdu, pdu);

    case H225_RasMessage::e_unregistrationRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveUnregistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationConfirm :
      return OnReceiveUnregistrationConfirm(pdu, pdu);

    case H225_RasMessage::e_unregistrationReject :
      return OnReceiveUnregistrationReject(pdu, pdu);

    case H225_RasMessage::e_admissionRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveAdmissionRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionConfirm :
      return OnReceiveAdmissionConfirm(pdu, pdu);

    case H225_RasMessage::e_admissionReject :
      return OnReceiveAdmissionReject(pdu, pdu);

    case H225_RasMessage::e_bandwidthRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveBandwidthRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthConfirm :
      return OnReceiveBandwidthConfirm(pdu, pdu);

    case H225_RasMessage::e_bandwidthReject :
      return OnReceiveBandwidthReject(pdu, pdu);

    case H225_RasMessage::e_disengageRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveDisengageRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageConfirm :
      return OnReceiveDisengageConfirm(pdu, pdu);

    case H225_RasMessage::e_disengageReject :
      return OnReceiveDisengageReject(pdu, pdu);

    case H225_RasMessage::e_locationRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveLocationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_locationConfirm :
      return OnReceiveLocationConfirm(pdu, pdu);

    case H225_RasMessage::e_locationReject :
      return OnReceiveLocationReject(pdu, pdu);

    case H225_RasMessage::e_infoRequest :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveInfoRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestResponse :
      return OnReceiveInfoRequestResponse(pdu, pdu);

    case H225_RasMessage::e_nonStandardMessage :
      OnReceiveNonStandardMessage(pdu, pdu);
      break;

    case H225_RasMessage::e_unknownMessageResponse :
      OnReceiveUnknownMessageResponse(pdu, pdu);
      break;

    case H225_RasMessage::e_requestInProgress :
      return OnReceiveRequestInProgress(pdu, pdu);

    case H225_RasMessage::e_resourcesAvailableIndicate :
      if (SendCachedResponse(pdu))
        return FALSE;
      OnReceiveResourcesAvailableIndicate(pdu, pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      return OnReceiveResourcesAvailableConfirm(pdu, pdu);

    case H225_RasMessage::e_infoRequestAck :
      return OnReceiveInfoRequestAck(pdu, pdu);

    case H225_RasMessage::e_infoRequestNak :
      return OnReceiveInfoRequestNak(pdu, pdu);

    default :
      OnReceiveUnkown(pdu);
  }

  return FALSE;
}


static void OnSendPDU(H225_RAS & ras, H323RasPDU & pdu)
{
  switch (pdu.GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      ras.OnSendGatekeeperRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_gatekeeperConfirm :
      ras.OnSendGatekeeperConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_gatekeeperReject :
      ras.OnSendGatekeeperReject(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationRequest :
      ras.OnSendRegistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationConfirm :
      ras.OnSendRegistrationConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationReject :
      ras.OnSendRegistrationReject(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationRequest :
      ras.OnSendUnregistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationConfirm :
      ras.OnSendUnregistrationConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationReject :
      ras.OnSendUnregistrationReject(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionRequest :
      ras.OnSendAdmissionRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionConfirm :
      ras.OnSendAdmissionConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionReject :
      ras.OnSendAdmissionReject(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthRequest :
      ras.OnSendBandwidthRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthConfirm :
      ras.OnSendBandwidthConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthReject :
      ras.OnSendBandwidthReject(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageRequest :
      ras.OnSendDisengageRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageConfirm :
      ras.OnSendDisengageConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageReject :
      ras.OnSendDisengageReject(pdu, pdu);
      break;

    case H225_RasMessage::e_locationRequest :
      ras.OnSendLocationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_locationConfirm :
      ras.OnSendLocationConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_locationReject :
      ras.OnSendLocationReject(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequest :
      ras.OnSendInfoRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestResponse :
      ras.OnSendInfoRequestResponse(pdu, pdu);
      break;

    case H225_RasMessage::e_nonStandardMessage :
      ras.OnSendNonStandardMessage(pdu, pdu);
      break;

    case H225_RasMessage::e_unknownMessageResponse :
      ras.OnSendUnknownMessageResponse(pdu, pdu);
      break;

    case H225_RasMessage::e_requestInProgress :
      ras.OnSendRequestInProgress(pdu, pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableIndicate :
      ras.OnSendResourcesAvailableIndicate(pdu, pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      ras.OnSendResourcesAvailableConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestAck :
      ras.OnSendInfoRequestAck(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestNak :
      ras.OnSendInfoRequestNak(pdu, pdu);
      break;

    default :
      break;
  }
}


BOOL H225_RAS::WritePDU(H323RasPDU & pdu)
{
  OnSendPDU(*this, pdu);

  PINDEX idx = responses.GetValuesIndex(Response(lastReceivedFrom, pdu.GetSequenceNumber()));
  if (idx != P_MAX_INDEX)
    responses[idx].SetPDU(pdu);

  return pdu.Write(*transport);
}


BOOL H225_RAS::MakeRequest(Request & request)
{
  PTRACE(3, "RAS\tMaking request: " << request.requestPDU.GetTagName());

  OnSendPDU(*this, request.requestPDU);

  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, &request);
  requestsMutex.Signal();

  BOOL ok = request.Poll(endpoint, *transport);

  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, NULL);
  requestsMutex.Signal();

  return ok;
}


BOOL H225_RAS::CheckForResponse(unsigned reqTag, unsigned seqNum, const PASN_Choice * reason)
{
  PWaitAndSignal mutex(requestsMutex);

  if (!requests.Contains(seqNum)) {
    PTRACE(3, "RAS\tTimed out or received sequence number (" << seqNum << ") for PDU we never requested");
    return FALSE;
  }

  lastRequest = &requests[seqNum];
  lastRequest->responseMutex.Wait();
  lastRequest->CheckResponse(reqTag, reason);
  return TRUE;
}


BOOL H225_RAS::SetUpCallSignalAddresses(H225_ArrayOf_TransportAddress & addresses)
{
  H323TransportAddress address = transport->GetRemoteAddress();
  H225_TransportAddress rasAddress;
  address.SetPDU(rasAddress);

  const OpalListenerList & listeners = endpoint.GetListeners();
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    address = listeners[i].GetLocalAddress();
    address.SetPDU(addresses, *transport);
  }

  return addresses.GetSize() > 0;
}


BOOL H225_RAS::OnReceiveRequestInProgress(const H323RasPDU &, const H225_RequestInProgress & rip)
{
  PWaitAndSignal mutex(requestsMutex);

  unsigned seqNum = rip.m_requestSeqNum;
  if (requests.Contains(seqNum)) {
    PTRACE(3, "RAS\tReceived RIP on sequence number " << seqNum);
    lastRequest = &requests[seqNum];
    lastRequest->responseMutex.Wait();
    lastRequest->OnReceiveRIP(rip);
    return OnReceiveRequestInProgress(rip);
  }

  PTRACE(3, "RAS\tTimed out or received sequence number (" << seqNum << ") for PDU we never requested");
  return FALSE;
}


BOOL H225_RAS::OnReceiveRequestInProgress(const H225_RequestInProgress & /*rip*/)
{
  return TRUE;
}


void H225_RAS::OnSendGatekeeperRequest(H323RasPDU &, H225_GatekeeperRequest & grq)
{
  if (!gatekeeperIdentifier) {
    grq.IncludeOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier);
    grq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendGatekeeperRequest(grq);
}


void H225_RAS::OnSendGatekeeperRequest(H225_GatekeeperRequest & /*grq*/)
{
}


void H225_RAS::OnSendGatekeeperConfirm(H323RasPDU &, H225_GatekeeperConfirm & gcf)
{
  if (!gatekeeperIdentifier) {
    gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_gatekeeperIdentifier);
    gcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendGatekeeperConfirm(gcf);
}


void H225_RAS::OnSendGatekeeperConfirm(H225_GatekeeperConfirm & /*gcf*/)
{
}


void H225_RAS::OnSendGatekeeperReject(H323RasPDU &, H225_GatekeeperReject & grj)
{
  if (!gatekeeperIdentifier) {
    grj.IncludeOptionalField(H225_GatekeeperReject::e_gatekeeperIdentifier);
    grj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendGatekeeperReject(grj);
}


void H225_RAS::OnSendGatekeeperReject(H225_GatekeeperReject & /*grj*/)
{
}


BOOL H225_RAS::OnReceiveGatekeeperRequest(const H323RasPDU &, const H225_GatekeeperRequest & grq)
{
  return OnReceiveGatekeeperRequest(grq);
}


BOOL H225_RAS::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest &)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveGatekeeperConfirm(const H323RasPDU &, const H225_GatekeeperConfirm & gcf)
{
  if (!CheckForResponse(H225_RasMessage::e_gatekeeperRequest, gcf.m_requestSeqNum))
    return FALSE;

  if (gatekeeperIdentifier.IsEmpty())
    gatekeeperIdentifier = gcf.m_gatekeeperIdentifier;
  else {
    PString gkid = gcf.m_gatekeeperIdentifier;
    if (gatekeeperIdentifier *= gkid)
      gatekeeperIdentifier = gkid;
    else {
      PTRACE(2, "RAS\tReceived a GCF from " << gkid
             << " but wanted it from " << gatekeeperIdentifier);
      return FALSE;
    }
  }

  return OnReceiveGatekeeperConfirm(gcf);
}


BOOL H225_RAS::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & /*gcf*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveGatekeeperReject(const H323RasPDU &, const H225_GatekeeperReject & grj)
{
  if (!CheckForResponse(H225_RasMessage::e_gatekeeperRequest, grj.m_requestSeqNum, &grj.m_rejectReason))
    return FALSE;

  return OnReceiveGatekeeperReject(grj);
}


BOOL H225_RAS::OnReceiveGatekeeperReject(const H225_GatekeeperReject & /*grj*/)
{
  return TRUE;
}


void H225_RAS::OnSendRegistrationRequest(H323RasPDU & pdu, H225_RegistrationRequest & rrq)
{
  pdu.Prepare(rrq.m_cryptoTokens, rrq, H225_RegistrationRequest::e_cryptoTokens);
  OnSendRegistrationRequest(rrq);
}


void H225_RAS::OnSendRegistrationRequest(H225_RegistrationRequest & /*rrq*/)
{
}


void H225_RAS::OnSendRegistrationConfirm(H323RasPDU & pdu, H225_RegistrationConfirm & rcf)
{
  if (!gatekeeperIdentifier) {
    rcf.IncludeOptionalField(H225_RegistrationConfirm::e_gatekeeperIdentifier);
    rcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  pdu.Prepare(rcf.m_cryptoTokens, rcf, H225_RegistrationConfirm::e_cryptoTokens);

  OnSendRegistrationConfirm(rcf);
}


void H225_RAS::OnSendRegistrationConfirm(H225_RegistrationConfirm & /*rcf*/)
{
}


void H225_RAS::OnSendRegistrationReject(H323RasPDU & pdu, H225_RegistrationReject & rrj)
{
  if (!gatekeeperIdentifier) {
    rrj.IncludeOptionalField(H225_RegistrationReject::e_gatekeeperIdentifier);
    rrj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  pdu.Prepare(rrj.m_cryptoTokens, rrj, H225_RegistrationReject::e_cryptoTokens);

  OnSendRegistrationReject(rrj);
}


void H225_RAS::OnSendRegistrationReject(H225_RegistrationReject & /*rrj*/)
{
}


BOOL H225_RAS::OnReceiveRegistrationRequest(const H323RasPDU &, const H225_RegistrationRequest & rrq)
{
  return OnReceiveRegistrationRequest(rrq);
}


BOOL H225_RAS::OnReceiveRegistrationRequest(const H225_RegistrationRequest &)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveRegistrationConfirm(const H323RasPDU & pdu, const H225_RegistrationConfirm & rcf)
{
  if (!CheckForResponse(H225_RasMessage::e_registrationRequest, rcf.m_requestSeqNum))
    return FALSE;

  if (lastRequest != NULL) {
    PString endpointIdentifier = rcf.m_endpointIdentifier;
    const H235Authenticators & authenticators = lastRequest->requestPDU.GetAuthenticators();
    for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
      H235Authenticator & authenticator = authenticators[i];
      if (authenticator.UseGkAndEpIdentifiers())
        authenticator.SetLocalId(endpointIdentifier);
    }
  }

  if (!CheckCryptoTokens(pdu, rcf.m_cryptoTokens, H225_RegistrationConfirm::e_cryptoTokens))
    return FALSE;

  return OnReceiveRegistrationConfirm(rcf);
}


BOOL H225_RAS::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & /*rcf*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveRegistrationReject(const H323RasPDU & pdu, const H225_RegistrationReject & rrj)
{
  if (!CheckForResponse(H225_RasMessage::e_registrationRequest, rrj.m_requestSeqNum, &rrj.m_rejectReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, rrj.m_cryptoTokens, H225_RegistrationReject::e_cryptoTokens))
    return FALSE;

  return OnReceiveRegistrationReject(rrj);
}


BOOL H225_RAS::OnReceiveRegistrationReject(const H225_RegistrationReject & /*rrj*/)
{
  return TRUE;
}


void H225_RAS::OnSendUnregistrationRequest(H323RasPDU & pdu, H225_UnregistrationRequest & urq)
{
  pdu.Prepare(urq.m_cryptoTokens, urq, H225_UnregistrationRequest::e_cryptoTokens);
  OnSendUnregistrationRequest(urq);
}


void H225_RAS::OnSendUnregistrationRequest(H225_UnregistrationRequest & /*urq*/)
{
}


void H225_RAS::OnSendUnregistrationConfirm(H323RasPDU & pdu, H225_UnregistrationConfirm & ucf)
{
  pdu.Prepare(ucf.m_cryptoTokens, ucf, H225_UnregistrationConfirm::e_cryptoTokens);
  OnSendUnregistrationConfirm(ucf);
}


void H225_RAS::OnSendUnregistrationConfirm(H225_UnregistrationConfirm & /*ucf*/)
{
}


void H225_RAS::OnSendUnregistrationReject(H323RasPDU & pdu, H225_UnregistrationReject & urj)
{
  pdu.Prepare(urj.m_cryptoTokens, urj, H225_UnregistrationReject::e_cryptoTokens);
  OnSendUnregistrationReject(urj);
}


void H225_RAS::OnSendUnregistrationReject(H225_UnregistrationReject & /*urj*/)
{
}


BOOL H225_RAS::OnReceiveUnregistrationRequest(const H323RasPDU & pdu, const H225_UnregistrationRequest & urq)
{
  if (!CheckCryptoTokens(pdu, urq.m_cryptoTokens, H225_UnregistrationRequest::e_cryptoTokens))
    return FALSE;

  return OnReceiveUnregistrationRequest(urq);
}


BOOL H225_RAS::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & /*urq*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveUnregistrationConfirm(const H323RasPDU & pdu, const H225_UnregistrationConfirm & ucf)
{
  if (!CheckForResponse(H225_RasMessage::e_unregistrationRequest, ucf.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, ucf.m_cryptoTokens, H225_UnregistrationConfirm::e_cryptoTokens))
    return FALSE;

  return OnReceiveUnregistrationConfirm(ucf);
}


BOOL H225_RAS::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & /*ucf*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveUnregistrationReject(const H323RasPDU & pdu, const H225_UnregistrationReject & urj)
{
  if (!CheckForResponse(H225_RasMessage::e_unregistrationRequest, urj.m_requestSeqNum, &urj.m_rejectReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, urj.m_cryptoTokens, H225_UnregistrationReject::e_cryptoTokens))
    return FALSE;

  return OnReceiveUnregistrationReject(urj);
}


BOOL H225_RAS::OnReceiveUnregistrationReject(const H225_UnregistrationReject & /*urj*/)
{
  return TRUE;
}


void H225_RAS::OnSendAdmissionRequest(H323RasPDU & pdu, H225_AdmissionRequest & arq)
{
  pdu.Prepare(arq.m_cryptoTokens, arq, H225_AdmissionRequest::e_cryptoTokens);
  OnSendAdmissionRequest(arq);
}


void H225_RAS::OnSendAdmissionRequest(H225_AdmissionRequest & /*arq*/)
{
}


void H225_RAS::OnSendAdmissionConfirm(H323RasPDU & pdu, H225_AdmissionConfirm & acf)
{
  pdu.Prepare(acf.m_cryptoTokens, acf, H225_AdmissionConfirm::e_cryptoTokens);
  OnSendAdmissionConfirm(acf);
}


void H225_RAS::OnSendAdmissionConfirm(H225_AdmissionConfirm & /*acf*/)
{
}


void H225_RAS::OnSendAdmissionReject(H323RasPDU & pdu, H225_AdmissionReject & arj)
{
  pdu.Prepare(arj.m_cryptoTokens, arj, H225_AdmissionReject::e_cryptoTokens);
  OnSendAdmissionReject(arj);
}


void H225_RAS::OnSendAdmissionReject(H225_AdmissionReject & /*arj*/)
{
}


BOOL H225_RAS::OnReceiveAdmissionRequest(const H323RasPDU & pdu, const H225_AdmissionRequest & arq)
{
  if (!CheckCryptoTokens(pdu, arq.m_cryptoTokens, H225_AdmissionRequest::e_cryptoTokens))
    return FALSE;

  return OnReceiveAdmissionRequest(arq);
}


BOOL H225_RAS::OnReceiveAdmissionRequest(const H225_AdmissionRequest & /*arq*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveAdmissionConfirm(const H323RasPDU & pdu, const H225_AdmissionConfirm & acf)
{
  if (!CheckForResponse(H225_RasMessage::e_admissionRequest, acf.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, acf.m_cryptoTokens, H225_AdmissionConfirm::e_cryptoTokens))
    return FALSE;

  return OnReceiveAdmissionConfirm(acf);
}


BOOL H225_RAS::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & /*acf*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveAdmissionReject(const H323RasPDU & pdu, const H225_AdmissionReject & arj)
{
  if (!CheckForResponse(H225_RasMessage::e_admissionRequest, arj.m_requestSeqNum, &arj.m_rejectReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, arj.m_cryptoTokens, H225_AdmissionReject::e_cryptoTokens))
    return FALSE;

  return OnReceiveAdmissionReject(arj);
}


BOOL H225_RAS::OnReceiveAdmissionReject(const H225_AdmissionReject & /*arj*/)
{
  return TRUE;
}


void H225_RAS::OnSendBandwidthRequest(H323RasPDU & pdu, H225_BandwidthRequest & brq)
{
  pdu.Prepare(brq.m_cryptoTokens, brq, H225_BandwidthRequest::e_cryptoTokens);
  OnSendBandwidthRequest(brq);
}


void H225_RAS::OnSendBandwidthRequest(H225_BandwidthRequest & /*brq*/)
{
}


BOOL H225_RAS::OnReceiveBandwidthRequest(const H323RasPDU & pdu, const H225_BandwidthRequest & brq)
{
  if (!CheckCryptoTokens(pdu, brq.m_cryptoTokens, H225_BandwidthRequest::e_cryptoTokens))
    return FALSE;

  return OnReceiveBandwidthRequest(brq);
}


BOOL H225_RAS::OnReceiveBandwidthRequest(const H225_BandwidthRequest & /*brq*/)
{
  return TRUE;
}


void H225_RAS::OnSendBandwidthConfirm(H323RasPDU & pdu, H225_BandwidthConfirm & bcf)
{
  pdu.Prepare(bcf.m_cryptoTokens, bcf, H225_BandwidthConfirm::e_cryptoTokens);
  OnSendBandwidthConfirm(bcf);
}


void H225_RAS::OnSendBandwidthConfirm(H225_BandwidthConfirm & /*bcf*/)
{
}


BOOL H225_RAS::OnReceiveBandwidthConfirm(const H323RasPDU & pdu, const H225_BandwidthConfirm & bcf)
{
  if (!CheckForResponse(H225_RasMessage::e_bandwidthRequest, bcf.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, bcf.m_cryptoTokens, H225_BandwidthConfirm::e_cryptoTokens))
    return FALSE;

  return OnReceiveBandwidthConfirm(bcf);
}


BOOL H225_RAS::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & /*bcf*/)
{
  return TRUE;
}


void H225_RAS::OnSendBandwidthReject(H323RasPDU & pdu, H225_BandwidthReject & brj)
{
  pdu.Prepare(brj.m_cryptoTokens, brj, H225_BandwidthReject::e_cryptoTokens);
  OnSendBandwidthReject(brj);
}


void H225_RAS::OnSendBandwidthReject(H225_BandwidthReject & /*brj*/)
{
}


BOOL H225_RAS::OnReceiveBandwidthReject(const H323RasPDU & pdu, const H225_BandwidthReject & brj)
{
  if (!CheckForResponse(H225_RasMessage::e_bandwidthRequest, brj.m_requestSeqNum, &brj.m_rejectReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, brj.m_cryptoTokens, H225_BandwidthReject::e_cryptoTokens))
    return FALSE;

  return OnReceiveBandwidthReject(brj);
}


BOOL H225_RAS::OnReceiveBandwidthReject(const H225_BandwidthReject & /*brj*/)
{
  return TRUE;
}


void H225_RAS::OnSendDisengageRequest(H323RasPDU & pdu, H225_DisengageRequest & drq)
{
  pdu.Prepare(drq.m_cryptoTokens, drq, H225_DisengageRequest::e_cryptoTokens);
  OnSendDisengageRequest(drq);
}


void H225_RAS::OnSendDisengageRequest(H225_DisengageRequest & /*drq*/)
{
}


BOOL H225_RAS::OnReceiveDisengageRequest(const H323RasPDU & pdu, const H225_DisengageRequest & drq)
{
  if (!CheckCryptoTokens(pdu, drq.m_cryptoTokens, H225_DisengageRequest::e_cryptoTokens))
    return FALSE;

  return OnReceiveDisengageRequest(drq);
}


BOOL H225_RAS::OnReceiveDisengageRequest(const H225_DisengageRequest & /*drq*/)
{
  return TRUE;
}


void H225_RAS::OnSendDisengageConfirm(H323RasPDU & pdu, H225_DisengageConfirm & dcf)
{
  pdu.Prepare(dcf.m_cryptoTokens, dcf, H225_DisengageConfirm::e_cryptoTokens);
  OnSendDisengageConfirm(dcf);
}


void H225_RAS::OnSendDisengageConfirm(H225_DisengageConfirm & /*dcf*/)
{
}


BOOL H225_RAS::OnReceiveDisengageConfirm(const H323RasPDU & pdu, const H225_DisengageConfirm & dcf)
{
  if (!CheckForResponse(H225_RasMessage::e_disengageRequest, dcf.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, dcf.m_cryptoTokens, H225_DisengageConfirm::e_cryptoTokens))
    return FALSE;

  return OnReceiveDisengageConfirm(dcf);
}


BOOL H225_RAS::OnReceiveDisengageConfirm(const H225_DisengageConfirm & /*dcf*/)
{
  return TRUE;
}


void H225_RAS::OnSendDisengageReject(H323RasPDU & pdu, H225_DisengageReject & drj)
{
  pdu.Prepare(drj.m_cryptoTokens, drj, H225_DisengageReject::e_cryptoTokens);
  OnSendDisengageReject(drj);
}


void H225_RAS::OnSendDisengageReject(H225_DisengageReject & /*drj*/)
{
}


BOOL H225_RAS::OnReceiveDisengageReject(const H323RasPDU & pdu, const H225_DisengageReject & drj)
{
  if (!CheckForResponse(H225_RasMessage::e_disengageRequest, drj.m_requestSeqNum, &drj.m_rejectReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, drj.m_cryptoTokens, H225_DisengageReject::e_cryptoTokens))
    return FALSE;

  return OnReceiveDisengageReject(drj);
}


BOOL H225_RAS::OnReceiveDisengageReject(const H225_DisengageReject & /*drj*/)
{
  return TRUE;
}


void H225_RAS::OnSendLocationRequest(H323RasPDU & pdu, H225_LocationRequest & lrq)
{
  pdu.Prepare(lrq.m_cryptoTokens, lrq, H225_LocationRequest::e_cryptoTokens);
  OnSendLocationRequest(lrq);
}


void H225_RAS::OnSendLocationRequest(H225_LocationRequest & /*lrq*/)
{
}


BOOL H225_RAS::OnReceiveLocationRequest(const H323RasPDU & pdu, const H225_LocationRequest & lrq)
{
  if (!CheckCryptoTokens(pdu, lrq.m_cryptoTokens, H225_LocationRequest::e_cryptoTokens))
    return FALSE;

  return OnReceiveLocationRequest(lrq);
}


BOOL H225_RAS::OnReceiveLocationRequest(const H225_LocationRequest & /*lrq*/)
{
  return TRUE;
}


void H225_RAS::OnSendLocationConfirm(H323RasPDU & pdu, H225_LocationConfirm & lcf)
{
  pdu.Prepare(lcf.m_cryptoTokens, lcf, H225_LocationConfirm::e_cryptoTokens);
  OnSendLocationConfirm(lcf);
}


void H225_RAS::OnSendLocationConfirm(H225_LocationConfirm & /*lcf*/)
{
}


BOOL H225_RAS::OnReceiveLocationConfirm(const H323RasPDU &, const H225_LocationConfirm & lcf)
{
  if (!CheckForResponse(H225_RasMessage::e_locationRequest, lcf.m_requestSeqNum))
    return FALSE;

  if (lastRequest->responseInfo != NULL) {
    H323TransportAddress & locatedAddress = *(H323TransportAddress *)lastRequest->responseInfo;
    locatedAddress = lcf.m_callSignalAddress;
  }

  return OnReceiveLocationConfirm(lcf);
}


BOOL H225_RAS::OnReceiveLocationConfirm(const H225_LocationConfirm & /*lcf*/)
{
  return TRUE;
}


void H225_RAS::OnSendLocationReject(H323RasPDU & pdu, H225_LocationReject & lrj)
{
  pdu.Prepare(lrj.m_cryptoTokens, lrj, H225_LocationReject::e_cryptoTokens);
  OnSendLocationReject(lrj);
}


void H225_RAS::OnSendLocationReject(H225_LocationReject & /*lrj*/)
{
}


BOOL H225_RAS::OnReceiveLocationReject(const H323RasPDU & pdu, const H225_LocationReject & lrj)
{
  if (!CheckForResponse(H225_RasMessage::e_locationRequest, lrj.m_requestSeqNum, &lrj.m_rejectReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, lrj.m_cryptoTokens, H225_LocationReject::e_cryptoTokens))
    return FALSE;

  return OnReceiveLocationReject(lrj);
}


BOOL H225_RAS::OnReceiveLocationReject(const H225_LocationReject & /*lrj*/)
{
  return TRUE;
}


void H225_RAS::OnSendInfoRequest(H323RasPDU & pdu, H225_InfoRequest & irq)
{
  pdu.Prepare(irq.m_cryptoTokens, irq, H225_InfoRequest::e_cryptoTokens);
  OnSendInfoRequest(irq);
}


void H225_RAS::OnSendInfoRequest(H225_InfoRequest & /*irq*/)
{
}


BOOL H225_RAS::OnReceiveInfoRequest(const H323RasPDU & pdu, const H225_InfoRequest & irq)
{
  if (!CheckCryptoTokens(pdu, irq.m_cryptoTokens, H225_InfoRequest::e_cryptoTokens))
    return FALSE;

  return OnReceiveInfoRequest(irq);
}


BOOL H225_RAS::OnReceiveInfoRequest(const H225_InfoRequest & /*irq*/)
{
  return TRUE;
}


void H225_RAS::OnSendInfoRequestResponse(H323RasPDU & pdu, H225_InfoRequestResponse & irr)
{
  pdu.Prepare(irr.m_cryptoTokens, irr, H225_InfoRequestResponse::e_cryptoTokens);
  OnSendInfoRequestResponse(irr);
}


void H225_RAS::OnSendInfoRequestResponse(H225_InfoRequestResponse & /*irr*/)
{
}


BOOL H225_RAS::OnReceiveInfoRequestResponse(const H323RasPDU & pdu, const H225_InfoRequestResponse & irr)
{
  if (!CheckForResponse(H225_RasMessage::e_infoRequest, irr.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, irr.m_cryptoTokens, H225_InfoRequestResponse::e_cryptoTokens))
    return FALSE;

  return OnReceiveInfoRequestResponse(irr);
}


BOOL H225_RAS::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & /*irr*/)
{
  return TRUE;
}


void H225_RAS::OnSendNonStandardMessage(H323RasPDU & pdu, H225_NonStandardMessage & nsm)
{
  pdu.Prepare(nsm.m_cryptoTokens, nsm, H225_NonStandardMessage::e_cryptoTokens);
  OnSendNonStandardMessage(nsm);
}


void H225_RAS::OnSendNonStandardMessage(H225_NonStandardMessage & /*nsm*/)
{
}


BOOL H225_RAS::OnReceiveNonStandardMessage(const H323RasPDU & pdu, const H225_NonStandardMessage & nsm)
{
  if (!CheckCryptoTokens(pdu, nsm.m_cryptoTokens, H225_NonStandardMessage::e_cryptoTokens))
    return FALSE;

  return OnReceiveNonStandardMessage(nsm);
}


BOOL H225_RAS::OnReceiveNonStandardMessage(const H225_NonStandardMessage & /*nsm*/)
{
  return TRUE;
}


void H225_RAS::OnSendUnknownMessageResponse(H323RasPDU & pdu, H225_UnknownMessageResponse & umr)
{
  pdu.Prepare(umr.m_cryptoTokens, umr, H225_UnknownMessageResponse::e_cryptoTokens);
  OnSendUnknownMessageResponse(umr);
}


void H225_RAS::OnSendUnknownMessageResponse(H225_UnknownMessageResponse & /*umr*/)
{
}


BOOL H225_RAS::OnReceiveUnknownMessageResponse(const H323RasPDU & pdu, const H225_UnknownMessageResponse & umr)
{
  if (!CheckCryptoTokens(pdu, umr.m_cryptoTokens, H225_UnknownMessageResponse::e_cryptoTokens))
    return FALSE;

  return OnReceiveUnknownMessageResponse(umr);
}


BOOL H225_RAS::OnReceiveUnknownMessageResponse(const H225_UnknownMessageResponse & /*umr*/)
{
  return TRUE;
}


void H225_RAS::OnSendRequestInProgress(H323RasPDU & pdu, H225_RequestInProgress & rip)
{
  pdu.Prepare(rip.m_cryptoTokens, rip, H225_RequestInProgress::e_cryptoTokens);
  OnSendRequestInProgress(rip);
}


void H225_RAS::OnSendRequestInProgress(H225_RequestInProgress & /*rip*/)
{
}


void H225_RAS::OnSendResourcesAvailableIndicate(H323RasPDU & pdu, H225_ResourcesAvailableIndicate & rai)
{
  pdu.Prepare(rai.m_cryptoTokens, rai, H225_ResourcesAvailableIndicate::e_cryptoTokens);
  OnSendResourcesAvailableIndicate(rai);
}


void H225_RAS::OnSendResourcesAvailableIndicate(H225_ResourcesAvailableIndicate & /*rai*/)
{
}


BOOL H225_RAS::OnReceiveResourcesAvailableIndicate(const H323RasPDU & pdu, const H225_ResourcesAvailableIndicate & rai)
{
  if (!CheckCryptoTokens(pdu, rai.m_cryptoTokens, H225_ResourcesAvailableIndicate::e_cryptoTokens))
    return FALSE;

  return OnReceiveResourcesAvailableIndicate(rai);
}


BOOL H225_RAS::OnReceiveResourcesAvailableIndicate(const H225_ResourcesAvailableIndicate & /*rai*/)
{
  return TRUE;
}


void H225_RAS::OnSendResourcesAvailableConfirm(H323RasPDU & pdu, H225_ResourcesAvailableConfirm & rac)
{
  pdu.Prepare(rac.m_cryptoTokens, rac, H225_ResourcesAvailableConfirm::e_cryptoTokens);
  OnSendResourcesAvailableConfirm(rac);
}


void H225_RAS::OnSendResourcesAvailableConfirm(H225_ResourcesAvailableConfirm & /*rac*/)
{
}


BOOL H225_RAS::OnReceiveResourcesAvailableConfirm(const H323RasPDU & pdu, const H225_ResourcesAvailableConfirm & rac)
{
  if (!CheckForResponse(H225_RasMessage::e_resourcesAvailableIndicate, rac.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, rac.m_cryptoTokens, H225_ResourcesAvailableConfirm::e_cryptoTokens))
    return FALSE;

  return OnReceiveResourcesAvailableConfirm(rac);
}


BOOL H225_RAS::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & /*rac*/)
{
  return TRUE;
}


void H225_RAS::OnSendInfoRequestAck(H323RasPDU & pdu, H225_InfoRequestAck & iack)
{
  pdu.Prepare(iack.m_cryptoTokens, iack, H225_InfoRequestAck::e_cryptoTokens);
  OnSendInfoRequestAck(iack);
}


void H225_RAS::OnSendInfoRequestAck(H225_InfoRequestAck & /*iack*/)
{
}


BOOL H225_RAS::OnReceiveInfoRequestAck(const H323RasPDU & pdu, const H225_InfoRequestAck & iack)
{
  if (!CheckForResponse(H225_RasMessage::e_infoRequestResponse, iack.m_requestSeqNum))
    return FALSE;

  if (!CheckCryptoTokens(pdu, iack.m_cryptoTokens, H225_InfoRequestAck::e_cryptoTokens))
    return FALSE;

  return OnReceiveInfoRequestAck(iack);
}


BOOL H225_RAS::OnReceiveInfoRequestAck(const H225_InfoRequestAck & /*iack*/)
{
  return TRUE;
}


void H225_RAS::OnSendInfoRequestNak(H323RasPDU & pdu, H225_InfoRequestNak & inak)
{
  pdu.Prepare(inak.m_cryptoTokens, inak, H225_InfoRequestNak::e_cryptoTokens);
  OnSendInfoRequestNak(inak);
}


void H225_RAS::OnSendInfoRequestNak(H225_InfoRequestNak & /*inak*/)
{
}


BOOL H225_RAS::OnReceiveInfoRequestNak(const H323RasPDU & pdu, const H225_InfoRequestNak & inak)
{
  if (!CheckForResponse(H225_RasMessage::e_infoRequestResponse, inak.m_requestSeqNum, &inak.m_nakReason))
    return FALSE;

  if (!CheckCryptoTokens(pdu, inak.m_cryptoTokens, H225_InfoRequestNak::e_cryptoTokens))
    return FALSE;

  return OnReceiveInfoRequestNak(inak);
}


BOOL H225_RAS::OnReceiveInfoRequestNak(const H225_InfoRequestNak & /*inak*/)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveUnkown(const H323RasPDU &)
{
  H323RasPDU response;
  response.BuildUnknownMessageResponse(0);
  return response.Write(*transport);
}


unsigned H225_RAS::GetNextSequenceNumber()
{
  PWaitAndSignal mutex(nextSequenceNumberMutex);
  nextSequenceNumber++;
  if (nextSequenceNumber >= 65536)
    nextSequenceNumber = 1;
  return nextSequenceNumber;
}


BOOL H225_RAS::CheckCryptoTokens(const H323RasPDU & pdu,
                                 const H225_ArrayOf_CryptoH323Token & cryptoTokens,
                                 unsigned optionalField)
{
  // If cypto token checking disabled, just return TRUE.
  if (!GetCheckResponseCryptoTokens())
    return TRUE;

  if (lastRequest != NULL && pdu.GetAuthenticators().IsEmpty())
    ((H323RasPDU &)pdu).SetAuthenticators(lastRequest->requestPDU.GetAuthenticators());

  if (pdu.Validate(cryptoTokens, (const PASN_Sequence &)pdu.GetObject(), optionalField))
    return TRUE;

  /* Note that a crypto tokens error is flagged to the requestor in the
     responseResult field but the other thread is NOT signalled. This is so
     it can wait for the full timeout for any other packets that might have
     the correct tokens, preventing a possible DOS attack.
   */
  if (lastRequest != NULL)
    lastRequest->responseResult = Request::BadCryptoTokens;

  return FALSE;
}


void H225_RAS::AgeResponses()
{
  PTime now;

  for (PINDEX i = 0; i < responses.GetSize(); i++) {
    const Response & response = responses[i];
    if ((now - response.lastUsedTime) > response.retirementAge) {
      PTRACE(4, "RAS\tRemoving cached response: " << response);
      responses.RemoveAt(i--);
    }
  }
}


BOOL H225_RAS::SendCachedResponse(const H323RasPDU & pdu)
{
  Response key(lastReceivedFrom, pdu.GetSequenceNumber());

  PINDEX idx = responses.GetValuesIndex(key);
  if (idx != P_MAX_INDEX)
    return responses[idx].SendCachedResponse(*transport);

  responses.Append(new Response(key));
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

H225_RAS::Request::Request(unsigned seqNum, H323RasPDU  & pdu)
  : requestPDU(pdu)
{
  sequenceNumber = seqNum;
}


BOOL H225_RAS::Request::Poll(H323EndPoint & endpoint, OpalTransport & transport)
{
  // Assume a confirm, reject callbacks will set to RejectReceived
  responseResult = AwaitingResponse;

  for (unsigned retry = 1; retry <= endpoint.GetRasRequestRetries(); retry++) {
    // To avoid race condition with RIP must set timeout before sending the packet
    whenResponseExpected = PTimer::Tick() + endpoint.GetRasRequestTimeout();

    if (!requestPDU.Write(transport))
      break;

    PTRACE(3, "RAS\tWaiting on response to seqnum=" << requestPDU.GetSequenceNumber()
           << " for " << setprecision(1) << endpoint.GetRasRequestTimeout() << " seconds");
    while (responseHandled.Wait(whenResponseExpected - PTimer::Tick())) {
      PWaitAndSignal mutex(responseMutex);
      switch (responseResult) {
        case ConfirmReceived :
          return TRUE;

        case RejectReceived :
          return FALSE;

        case RequestInProgress :
          responseResult = ConfirmReceived;
          PTRACE(3, "RAS\tWaiting again on response to seqnum=" << requestPDU.GetSequenceNumber()
                 << " for " << setprecision(1) << (whenResponseExpected - PTimer::Tick()) << " seconds");

        default :
          ; // Keep waiting
      }
    }

    if (responseResult == BadCryptoTokens) {
      PTRACE(2, "RAS\tResponse to seqnum=" << requestPDU.GetSequenceNumber()
             << " had invalid crypto tokens.");
      return FALSE;
    }

    PTRACE(1, "RAS\tTimeout on request seqnum=" << requestPDU.GetSequenceNumber()
           << ", try #" << retry << " of " << endpoint.GetRasRequestRetries());
  }

  responseResult = NoResponseReceived;
  return FALSE;
}


void H225_RAS::Request::CheckResponse(unsigned reqTag, const PASN_Choice * reason)
{
  if (requestPDU.GetTag() != reqTag) {
    PTRACE(3, "RAS\tReceived reply for incorrect PDU tag.");
    responseResult = RejectReceived;
    rejectReason = UINT_MAX;
    return;
  }

  if (reason != NULL) {
    PTRACE(1, "RAS\t" << requestPDU.GetTagName()
           << " rejected: " << reason->GetTagName());
    responseResult = RejectReceived;
    rejectReason = reason->GetTag();
    return;
  }

  responseResult = ConfirmReceived;
}


void H225_RAS::Request::OnReceiveRIP(const H225_RequestInProgress & rip)
{
  responseResult = RequestInProgress;
  whenResponseExpected = PTimer::Tick() + PTimeInterval(rip.m_delay);
}


/////////////////////////////////////////////////////////////////////////////

H225_RAS::Response::Response(const H323TransportAddress & addr, unsigned seqNum)
  : PString(addr),
    retirementAge(ResponseRetirementAge)
{
  sprintf("#%u", seqNum);
  replyPDU = NULL;
}


H225_RAS::Response::~Response()
{
  delete replyPDU;
}


void H225_RAS::Response::SetPDU(const H323RasPDU & pdu)
{
  PTRACE(4, "RAS\tAdding cached response: " << *this);

  delete replyPDU;
  replyPDU = new H323RasPDU(pdu);
  lastUsedTime = PTime();

  if (pdu.GetTag() == H225_RasMessage::e_requestInProgress) {
    const H225_RequestInProgress & rip = pdu;
    retirementAge = ResponseRetirementAge + (unsigned)rip.m_delay;
  }
}


BOOL H225_RAS::Response::SendCachedResponse(OpalTransport & transport)
{
  PTRACE(3, "RAS\tSending cached response: " << *this);

  if (replyPDU != NULL)
    replyPDU->Write(transport);
  else {
    PTRACE(2, "RAS\tRetry made by remote before sending response: " << *this);
  }

  lastUsedTime = PTime();
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
