/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: agent.proto */

#ifndef PROTOBUF_C_agent_2eproto__INCLUDED
#define PROTOBUF_C_agent_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif

#include "jaeger.pb-c.h"

typedef struct _Jaegertracing__Protobuf__Agent__Empty Jaegertracing__Protobuf__Agent__Empty;


/* --- enums --- */


/* --- messages --- */

struct  _Jaegertracing__Protobuf__Agent__Empty
{
  ProtobufCMessage base;
};
#define JAEGERTRACING__PROTOBUF__AGENT__EMPTY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__agent__empty__descriptor) \
     }


/* Jaegertracing__Protobuf__Agent__Empty methods */
void   jaegertracing__protobuf__agent__empty__init
                     (Jaegertracing__Protobuf__Agent__Empty         *message);
size_t jaegertracing__protobuf__agent__empty__get_packed_size
                     (const Jaegertracing__Protobuf__Agent__Empty   *message);
size_t jaegertracing__protobuf__agent__empty__pack
                     (const Jaegertracing__Protobuf__Agent__Empty   *message,
                      uint8_t             *out);
size_t jaegertracing__protobuf__agent__empty__pack_to_buffer
                     (const Jaegertracing__Protobuf__Agent__Empty   *message,
                      ProtobufCBuffer     *buffer);
Jaegertracing__Protobuf__Agent__Empty *
       jaegertracing__protobuf__agent__empty__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   jaegertracing__protobuf__agent__empty__free_unpacked
                     (Jaegertracing__Protobuf__Agent__Empty *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Jaegertracing__Protobuf__Agent__Empty_Closure)
                 (const Jaegertracing__Protobuf__Agent__Empty *message,
                  void *closure_data);

/* --- services --- */

typedef struct _Jaegertracing__Protobuf__Agent__Agent_Service Jaegertracing__Protobuf__Agent__Agent_Service;
struct _Jaegertracing__Protobuf__Agent__Agent_Service
{
  ProtobufCService base;
  void (*emit_batch)(Jaegertracing__Protobuf__Agent__Agent_Service *service,
                     const Jaegertracing__Protobuf__Batch *input,
                     Jaegertracing__Protobuf__Agent__Empty_Closure closure,
                     void *closure_data);
};
typedef void (*Jaegertracing__Protobuf__Agent__Agent_ServiceDestroy)(Jaegertracing__Protobuf__Agent__Agent_Service *);
void jaegertracing__protobuf__agent__agent__init (Jaegertracing__Protobuf__Agent__Agent_Service *service,
                                                  Jaegertracing__Protobuf__Agent__Agent_ServiceDestroy destroy);
#define JAEGERTRACING__PROTOBUF__AGENT__AGENT__BASE_INIT \
    { &jaegertracing__protobuf__agent__agent__descriptor, protobuf_c_service_invoke_internal, NULL }
#define JAEGERTRACING__PROTOBUF__AGENT__AGENT__INIT(function_prefix__) \
    { JAEGERTRACING__PROTOBUF__AGENT__AGENT__BASE_INIT,\
      function_prefix__ ## emit_batch  }
void jaegertracing__protobuf__agent__agent__emit_batch(ProtobufCService *service,
                                                       const Jaegertracing__Protobuf__Batch *input,
                                                       Jaegertracing__Protobuf__Agent__Empty_Closure closure,
                                                       void *closure_data);

/* --- descriptors --- */

extern const ProtobufCMessageDescriptor jaegertracing__protobuf__agent__empty__descriptor;
extern const ProtobufCServiceDescriptor jaegertracing__protobuf__agent__agent__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_agent_2eproto__INCLUDED */