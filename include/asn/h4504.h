//
// h4504.h
//
// Code automatically generated by asnparse.
//

#if ! H323_DISABLE_H4504

#ifndef __H4504_H
#define __H4504_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/asner.h>

#include "h4501.h"
#include "h4501.h"
#include "h225.h"


//
// CallHoldOperation
//

class H4504_CallHoldOperation : public PASN_Enumeration
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_CallHoldOperation, PASN_Enumeration);
#endif
  public:
    H4504_CallHoldOperation(unsigned tag = UniversalEnumeration, TagClass tagClass = UniversalTagClass);

    enum Enumerations {
      e_holdNotific = 101,
      e_retrieveNotific,
      e_remoteHold,
      e_remoteRetrieve
    };

    H4504_CallHoldOperation & operator=(unsigned v);
    PObject * Clone() const;
};


//
// MixedExtension
//

class H4501_Extension;
class H225_NonStandardParameter;

class H4504_MixedExtension : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_MixedExtension, PASN_Choice);
#endif
  public:
    H4504_MixedExtension(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_extension,
      e_nonStandardData
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H4501_Extension &() const;
#else
    operator H4501_Extension &();
    operator const H4501_Extension &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H225_NonStandardParameter &() const;
#else
    operator H225_NonStandardParameter &();
    operator const H225_NonStandardParameter &() const;
#endif

    BOOL CreateObject();
    PObject * Clone() const;
};


//
// Extension
//

class H4504_Extension : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_Extension, PASN_Sequence);
#endif
  public:
    H4504_Extension(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_extensionId;
    PASN_OctetString m_argument;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ArrayOf_MixedExtension
//

class H4504_MixedExtension;

class H4504_ArrayOf_MixedExtension : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_ArrayOf_MixedExtension, PASN_Array);
#endif
  public:
    H4504_ArrayOf_MixedExtension(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H4504_MixedExtension & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// HoldNotificArg
//

class H4504_HoldNotificArg : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_HoldNotificArg, PASN_Sequence);
#endif
  public:
    H4504_HoldNotificArg(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_extensionArg
    };

    H4504_ArrayOf_MixedExtension m_extensionArg;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// RetrieveNotificArg
//

class H4504_RetrieveNotificArg : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_RetrieveNotificArg, PASN_Sequence);
#endif
  public:
    H4504_RetrieveNotificArg(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_extensionArg
    };

    H4504_ArrayOf_MixedExtension m_extensionArg;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// RemoteHoldArg
//

class H4504_RemoteHoldArg : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_RemoteHoldArg, PASN_Sequence);
#endif
  public:
    H4504_RemoteHoldArg(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_extensionArg
    };

    H4504_ArrayOf_MixedExtension m_extensionArg;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// RemoteHoldRes
//

class H4504_RemoteHoldRes : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_RemoteHoldRes, PASN_Sequence);
#endif
  public:
    H4504_RemoteHoldRes(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_extensionRes
    };

    H4504_ArrayOf_MixedExtension m_extensionRes;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// RemoteRetrieveArg
//

class H4504_RemoteRetrieveArg : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_RemoteRetrieveArg, PASN_Sequence);
#endif
  public:
    H4504_RemoteRetrieveArg(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_extensionArg
    };

    H4504_ArrayOf_MixedExtension m_extensionArg;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// RemoteRetrieveRes
//

class H4504_RemoteRetrieveRes : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H4504_RemoteRetrieveRes, PASN_Sequence);
#endif
  public:
    H4504_RemoteRetrieveRes(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_extensionRes
    };

    H4504_ArrayOf_MixedExtension m_extensionRes;

    PINDEX GetDataLength() const;
    BOOL Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


#endif // __H4504_H

#endif // if ! H323_DISABLE_H4504


// End of h4504.h
