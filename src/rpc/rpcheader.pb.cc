// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: rpcheader.proto

#include "rpcheader.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
namespace mprpc {
constexpr RpcHeader::RpcHeader(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : service_name_(&::PROTOBUF_NAMESPACE_ID::internal::fixed_address_empty_string)
  , method_name_(&::PROTOBUF_NAMESPACE_ID::internal::fixed_address_empty_string)
  , args_size_(0){}
struct RpcHeaderDefaultTypeInternal {
  constexpr RpcHeaderDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~RpcHeaderDefaultTypeInternal() {}
  union {
    RpcHeader _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RpcHeaderDefaultTypeInternal _RpcHeader_default_instance_;
}  // namespace mprpc
static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_rpcheader_2eproto[1];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_rpcheader_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_rpcheader_2eproto = nullptr;

const uint32_t TableStruct_rpcheader_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::mprpc::RpcHeader, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::mprpc::RpcHeader, service_name_),
  PROTOBUF_FIELD_OFFSET(::mprpc::RpcHeader, method_name_),
  PROTOBUF_FIELD_OFFSET(::mprpc::RpcHeader, args_size_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::mprpc::RpcHeader)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::mprpc::_RpcHeader_default_instance_),
};

const char descriptor_table_protodef_rpcheader_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\017rpcheader.proto\022\005mprpc\"I\n\tRpcHeader\022\024\n"
  "\014service_name\030\001 \001(\014\022\023\n\013method_name\030\002 \001(\014"
  "\022\021\n\targs_size\030\003 \001(\005b\006proto3"
  ;
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_rpcheader_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_rpcheader_2eproto = {
  false, false, 107, descriptor_table_protodef_rpcheader_2eproto, "rpcheader.proto", 
  &descriptor_table_rpcheader_2eproto_once, nullptr, 0, 1,
  schemas, file_default_instances, TableStruct_rpcheader_2eproto::offsets,
  file_level_metadata_rpcheader_2eproto, file_level_enum_descriptors_rpcheader_2eproto, file_level_service_descriptors_rpcheader_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable* descriptor_table_rpcheader_2eproto_getter() {
  return &descriptor_table_rpcheader_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY static ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptorsRunner dynamic_init_dummy_rpcheader_2eproto(&descriptor_table_rpcheader_2eproto);
namespace mprpc {

// ===================================================================

class RpcHeader::_Internal {
 public:
};

RpcHeader::RpcHeader(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:mprpc.RpcHeader)
}
RpcHeader::RpcHeader(const RpcHeader& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  service_name_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    service_name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_service_name().empty()) {
    service_name_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, from._internal_service_name(), 
      GetArenaForAllocation());
  }
  method_name_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    method_name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_method_name().empty()) {
    method_name_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, from._internal_method_name(), 
      GetArenaForAllocation());
  }
  args_size_ = from.args_size_;
  // @@protoc_insertion_point(copy_constructor:mprpc.RpcHeader)
}

inline void RpcHeader::SharedCtor() {
service_name_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  service_name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
method_name_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  method_name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
args_size_ = 0;
}

RpcHeader::~RpcHeader() {
  // @@protoc_insertion_point(destructor:mprpc.RpcHeader)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

inline void RpcHeader::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  service_name_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  method_name_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void RpcHeader::ArenaDtor(void* object) {
  RpcHeader* _this = reinterpret_cast< RpcHeader* >(object);
  (void)_this;
}
void RpcHeader::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void RpcHeader::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void RpcHeader::Clear() {
// @@protoc_insertion_point(message_clear_start:mprpc.RpcHeader)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  service_name_.ClearToEmpty();
  method_name_.ClearToEmpty();
  args_size_ = 0;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* RpcHeader::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // bytes service_name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_service_name();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // bytes method_name = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          auto str = _internal_mutable_method_name();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 args_size = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24)) {
          args_size_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* RpcHeader::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:mprpc.RpcHeader)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // bytes service_name = 1;
  if (!this->_internal_service_name().empty()) {
    target = stream->WriteBytesMaybeAliased(
        1, this->_internal_service_name(), target);
  }

  // bytes method_name = 2;
  if (!this->_internal_method_name().empty()) {
    target = stream->WriteBytesMaybeAliased(
        2, this->_internal_method_name(), target);
  }

  // int32 args_size = 3;
  if (this->_internal_args_size() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(3, this->_internal_args_size(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mprpc.RpcHeader)
  return target;
}

size_t RpcHeader::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mprpc.RpcHeader)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // bytes service_name = 1;
  if (!this->_internal_service_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
        this->_internal_service_name());
  }

  // bytes method_name = 2;
  if (!this->_internal_method_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
        this->_internal_method_name());
  }

  // int32 args_size = 3;
  if (this->_internal_args_size() != 0) {
    total_size += ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32SizePlusOne(this->_internal_args_size());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData RpcHeader::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    RpcHeader::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*RpcHeader::GetClassData() const { return &_class_data_; }

void RpcHeader::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to,
                      const ::PROTOBUF_NAMESPACE_ID::Message& from) {
  static_cast<RpcHeader *>(to)->MergeFrom(
      static_cast<const RpcHeader &>(from));
}


void RpcHeader::MergeFrom(const RpcHeader& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:mprpc.RpcHeader)
  GOOGLE_DCHECK_NE(&from, this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_service_name().empty()) {
    _internal_set_service_name(from._internal_service_name());
  }
  if (!from._internal_method_name().empty()) {
    _internal_set_method_name(from._internal_method_name());
  }
  if (from._internal_args_size() != 0) {
    _internal_set_args_size(from._internal_args_size());
  }
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void RpcHeader::CopyFrom(const RpcHeader& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mprpc.RpcHeader)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool RpcHeader::IsInitialized() const {
  return true;
}

void RpcHeader::InternalSwap(RpcHeader* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      &service_name_, lhs_arena,
      &other->service_name_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      &method_name_, lhs_arena,
      &other->method_name_, rhs_arena
  );
  swap(args_size_, other->args_size_);
}

::PROTOBUF_NAMESPACE_ID::Metadata RpcHeader::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_rpcheader_2eproto_getter, &descriptor_table_rpcheader_2eproto_once,
      file_level_metadata_rpcheader_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace mprpc
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::mprpc::RpcHeader* Arena::CreateMaybeMessage< ::mprpc::RpcHeader >(Arena* arena) {
  return Arena::CreateMessageInternal< ::mprpc::RpcHeader >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
