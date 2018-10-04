//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.7
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class OpalProtocolMessage : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalProtocolMessage(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(OpalProtocolMessage obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalProtocolMessage() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalProtocolMessage(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public string protocol {
    set {
      OPALPINVOKE.OpalProtocolMessage_protocol_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalProtocolMessage_protocol_get(swigCPtr);
      return ret;
    } 
  }

  public string callToken {
    set {
      OPALPINVOKE.OpalProtocolMessage_callToken_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalProtocolMessage_callToken_get(swigCPtr);
      return ret;
    } 
  }

  public string identifier {
    set {
      OPALPINVOKE.OpalProtocolMessage_identifier_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalProtocolMessage_identifier_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_void payload {
    set {
      OPALPINVOKE.OpalProtocolMessage_payload_set(swigCPtr, SWIGTYPE_p_void.getCPtr(value));
    } 
    get {
      global::System.IntPtr cPtr = OPALPINVOKE.OpalProtocolMessage_payload_get(swigCPtr);
      SWIGTYPE_p_void ret = (cPtr == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_void(cPtr, false);
      return ret;
    } 
  }

  public uint size {
    set {
      OPALPINVOKE.OpalProtocolMessage_size_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalProtocolMessage_size_get(swigCPtr);
      return ret;
    } 
  }

  public OpalProtocolMessage() : this(OPALPINVOKE.new_OpalProtocolMessage(), true) {
  }

}
