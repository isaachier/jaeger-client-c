/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: sampling.proto */

#ifndef PROTOBUF_C_sampling_2eproto__INCLUDED
#define PROTOBUF_C_sampling_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif

#include "github.com/gogo/protobuf/gogoproto/gogo.pb-c.h"

typedef struct _Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy;
typedef struct _Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy;
typedef struct _Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy;
typedef struct _Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy;
typedef struct _Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest;
typedef struct _Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse;


/* --- enums --- */


/* --- messages --- */

struct  _Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy
{
  ProtobufCMessage base;
  double sampling_rate;
};
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PROBABILISTIC_SAMPLING_STRATEGY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__descriptor) \
    , 0 }


struct  _Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy
{
  ProtobufCMessage base;
  int32_t max_traces_per_seconds;
};
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__RATE_LIMITING_SAMPLING_STRATEGY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__descriptor) \
    , 0 }


typedef enum {
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY__NOT_SET = 0,
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY_PROBABILISTIC = 2,
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY_RATE_LIMITING = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY)
} Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy__StrategyCase;

struct  _Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy
{
  ProtobufCMessage base;
  char *operation;
  Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy__StrategyCase strategy_case;
  union {
    Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy *probabilistic;
    Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy *rate_limiting;
  };
};
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__operation_sampling_strategy__descriptor) \
    , (char *)protobuf_c_empty_string, JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY__NOT_SET, {0} }


struct  _Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy
{
  ProtobufCMessage base;
  double default_sampling_probability;
  double default_lower_bound_traces_per_second;
  double default_upper_bound_traces_per_second;
  size_t n_per_operation_strategy;
  Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy **per_operation_strategy;
};
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__descriptor) \
    , 0, 0, 0, 0,NULL }


struct  _Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest
{
  ProtobufCMessage base;
  char *service_name;
};
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__sampling_manager__sampling_strategy_request__descriptor) \
    , (char *)protobuf_c_empty_string }


typedef enum {
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY__NOT_SET = 0,
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY_PROBABILISTIC = 1,
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY_RATE_LIMITING = 2,
  JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY_PER_OPERATION = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY)
} Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse__StrategyCase;

struct  _Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse
{
  ProtobufCMessage base;
  Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse__StrategyCase strategy_case;
  union {
    Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy *probabilistic;
    Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy *rate_limiting;
    Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy *per_operation;
  };
};
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&jaegertracing__protobuf__sampling_manager__sampling_strategy_response__descriptor) \
    , JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY__NOT_SET, {0} }


/* Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy methods */
void   jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__init
                     (Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy         *message);
size_t jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__get_packed_size
                     (const Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy   *message);
size_t jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__pack
                     (const Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy   *message,
                      uint8_t             *out);
size_t jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__pack_to_buffer
                     (const Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy   *message,
                      ProtobufCBuffer     *buffer);
Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy *
       jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__free_unpacked
                     (Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy *message,
                      ProtobufCAllocator *allocator);
/* Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy methods */
void   jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__init
                     (Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy         *message);
size_t jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__get_packed_size
                     (const Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy   *message);
size_t jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__pack
                     (const Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy   *message,
                      uint8_t             *out);
size_t jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__pack_to_buffer
                     (const Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy   *message,
                      ProtobufCBuffer     *buffer);
Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy *
       jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__free_unpacked
                     (Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy *message,
                      ProtobufCAllocator *allocator);
/* Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy methods */
void   jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__operation_sampling_strategy__init
                     (Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy         *message);
/* Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy methods */
void   jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__init
                     (Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy         *message);
size_t jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__get_packed_size
                     (const Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy   *message);
size_t jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__pack
                     (const Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy   *message,
                      uint8_t             *out);
size_t jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__pack_to_buffer
                     (const Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy   *message,
                      ProtobufCBuffer     *buffer);
Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy *
       jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__free_unpacked
                     (Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy *message,
                      ProtobufCAllocator *allocator);
/* Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest methods */
void   jaegertracing__protobuf__sampling_manager__sampling_strategy_request__init
                     (Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest         *message);
size_t jaegertracing__protobuf__sampling_manager__sampling_strategy_request__get_packed_size
                     (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest   *message);
size_t jaegertracing__protobuf__sampling_manager__sampling_strategy_request__pack
                     (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest   *message,
                      uint8_t             *out);
size_t jaegertracing__protobuf__sampling_manager__sampling_strategy_request__pack_to_buffer
                     (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest   *message,
                      ProtobufCBuffer     *buffer);
Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest *
       jaegertracing__protobuf__sampling_manager__sampling_strategy_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   jaegertracing__protobuf__sampling_manager__sampling_strategy_request__free_unpacked
                     (Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest *message,
                      ProtobufCAllocator *allocator);
/* Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse methods */
void   jaegertracing__protobuf__sampling_manager__sampling_strategy_response__init
                     (Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse         *message);
size_t jaegertracing__protobuf__sampling_manager__sampling_strategy_response__get_packed_size
                     (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse   *message);
size_t jaegertracing__protobuf__sampling_manager__sampling_strategy_response__pack
                     (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse   *message,
                      uint8_t             *out);
size_t jaegertracing__protobuf__sampling_manager__sampling_strategy_response__pack_to_buffer
                     (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse   *message,
                      ProtobufCBuffer     *buffer);
Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse *
       jaegertracing__protobuf__sampling_manager__sampling_strategy_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   jaegertracing__protobuf__sampling_manager__sampling_strategy_response__free_unpacked
                     (Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy_Closure)
                 (const Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy *message,
                  void *closure_data);
typedef void (*Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy_Closure)
                 (const Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy *message,
                  void *closure_data);
typedef void (*Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy_Closure)
                 (const Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy *message,
                  void *closure_data);
typedef void (*Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy_Closure)
                 (const Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy *message,
                  void *closure_data);
typedef void (*Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest_Closure)
                 (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest *message,
                  void *closure_data);
typedef void (*Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse_Closure)
                 (const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse *message,
                  void *closure_data);

/* --- services --- */

typedef struct _Jaegertracing__Protobuf__SamplingManager__SamplingManager_Service Jaegertracing__Protobuf__SamplingManager__SamplingManager_Service;
struct _Jaegertracing__Protobuf__SamplingManager__SamplingManager_Service
{
  ProtobufCService base;
  void (*get_sampling_strategy)(Jaegertracing__Protobuf__SamplingManager__SamplingManager_Service *service,
                                const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest *input,
                                Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse_Closure closure,
                                void *closure_data);
};
typedef void (*Jaegertracing__Protobuf__SamplingManager__SamplingManager_ServiceDestroy)(Jaegertracing__Protobuf__SamplingManager__SamplingManager_Service *);
void jaegertracing__protobuf__sampling_manager__sampling_manager__init (Jaegertracing__Protobuf__SamplingManager__SamplingManager_Service *service,
                                                                        Jaegertracing__Protobuf__SamplingManager__SamplingManager_ServiceDestroy destroy);
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_MANAGER__BASE_INIT \
    { &jaegertracing__protobuf__sampling_manager__sampling_manager__descriptor, protobuf_c_service_invoke_internal, NULL }
#define JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_MANAGER__INIT(function_prefix__) \
    { JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_MANAGER__BASE_INIT,\
      function_prefix__ ## get_sampling_strategy  }
void jaegertracing__protobuf__sampling_manager__sampling_manager__get_sampling_strategy(ProtobufCService *service,
                                                                                        const Jaegertracing__Protobuf__SamplingManager__SamplingStrategyRequest *input,
                                                                                        Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse_Closure closure,
                                                                                        void *closure_data);

/* --- descriptors --- */

extern const ProtobufCMessageDescriptor jaegertracing__protobuf__sampling_manager__probabilistic_sampling_strategy__descriptor;
extern const ProtobufCMessageDescriptor jaegertracing__protobuf__sampling_manager__rate_limiting_sampling_strategy__descriptor;
extern const ProtobufCMessageDescriptor jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__descriptor;
extern const ProtobufCMessageDescriptor jaegertracing__protobuf__sampling_manager__per_operation_sampling_strategy__operation_sampling_strategy__descriptor;
extern const ProtobufCMessageDescriptor jaegertracing__protobuf__sampling_manager__sampling_strategy_request__descriptor;
extern const ProtobufCMessageDescriptor jaegertracing__protobuf__sampling_manager__sampling_strategy_response__descriptor;
extern const ProtobufCServiceDescriptor jaegertracing__protobuf__sampling_manager__sampling_manager__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_sampling_2eproto__INCLUDED */
