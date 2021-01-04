// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: google/protobuf/any.proto

#include <google/protobuf/any.pb.h>

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// This is a temporary google only hack
#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
#include "third_party/protobuf/version.h"
#endif
// @@protoc_insertion_point(includes)
namespace cv {
namespace protobuf {
class AnyDefaultTypeInternal {
 public:
  ::cv::protobuf::internal::ExplicitlyConstructed<Any>
      _instance;
} _Any_default_instance_;
}  // namespace protobuf
}  // namespace cv
namespace protobuf_google_2fprotobuf_2fany_2eproto {
void InitDefaultsAnyImpl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
  ::cv::protobuf::internal::InitProtobufDefaultsForceUnique();
#else
  ::cv::protobuf::internal::InitProtobufDefaults();
#endif  // GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
  {
    void* ptr = &::cv::protobuf::_Any_default_instance_;
    new (ptr) ::cv::protobuf::Any();
    ::cv::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::cv::protobuf::Any::InitAsDefaultInstance();
}

void InitDefaultsAny() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::cv::protobuf::GoogleOnceInit(&once, &InitDefaultsAnyImpl);
}

::cv::protobuf::Metadata file_level_metadata[1];

const ::cv::protobuf::uint32 TableStruct::offsets[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::cv::protobuf::Any, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::cv::protobuf::Any, type_url_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::cv::protobuf::Any, value_),
};
static const ::cv::protobuf::internal::MigrationSchema schemas[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::cv::protobuf::Any)},
};

static ::cv::protobuf::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::cv::protobuf::Message*>(&::cv::protobuf::_Any_default_instance_),
};

void protobuf_AssignDescriptors() {
  AddDescriptors();
  ::cv::protobuf::MessageFactory* factory = NULL;
  AssignDescriptors(
      "google/protobuf/any.proto", schemas, file_default_instances, TableStruct::offsets, factory,
      file_level_metadata, NULL, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::cv::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_PROTOBUF_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::cv::protobuf::internal::RegisterAllTypes(file_level_metadata, 1);
}

void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
      "\n\031google/protobuf/any.proto\022\017google.prot"
      "obuf\"&\n\003Any\022\020\n\010type_url\030\001 \001(\t\022\r\n\005value\030\002"
      " \001(\014Bo\n\023com.google.protobufB\010AnyProtoP\001Z"
      "%github.com/golang/protobuf/ptypes/any\242\002"
      "\003GPB\252\002\036Google.Protobuf.WellKnownTypesb\006p"
      "roto3"
  };
  ::cv::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 205);
  ::cv::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "google/protobuf/any.proto", &protobuf_RegisterTypes);
}

void AddDescriptors() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::cv::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at dynamic initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;
}  // namespace protobuf_google_2fprotobuf_2fany_2eproto
namespace cv {
namespace protobuf {

// ===================================================================

void Any::InitAsDefaultInstance() {
}
void Any::PackFrom(const ::cv::protobuf::Message& message) {
  _any_metadata_.PackFrom(message);
}

void Any::PackFrom(const ::cv::protobuf::Message& message,
                           const ::std::string& type_url_prefix) {
  _any_metadata_.PackFrom(message, type_url_prefix);
}

bool Any::UnpackTo(::cv::protobuf::Message* message) const {
  return _any_metadata_.UnpackTo(message);
}

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int Any::kTypeUrlFieldNumber;
const int Any::kValueFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

Any::Any()
  : ::cv::protobuf::Message(), _internal_metadata_(NULL), _any_metadata_(&type_url_, &value_) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    ::protobuf_google_2fprotobuf_2fany_2eproto::InitDefaultsAny();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:google.protobuf.Any)
}
Any::Any(const Any& from)
  : ::cv::protobuf::Message(),
      _internal_metadata_(NULL),
      _cached_size_(0),
      _any_metadata_(&type_url_, &value_) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  type_url_.UnsafeSetDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.type_url().size() > 0) {
    type_url_.AssignWithDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited(), from.type_url_);
  }
  value_.UnsafeSetDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.value().size() > 0) {
    value_.AssignWithDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited(), from.value_);
  }
  // @@protoc_insertion_point(copy_constructor:google.protobuf.Any)
}

void Any::SharedCtor() {
  type_url_.UnsafeSetDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  value_.UnsafeSetDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  _cached_size_ = 0;
}

Any::~Any() {
  // @@protoc_insertion_point(destructor:google.protobuf.Any)
  SharedDtor();
}

void Any::SharedDtor() {
  type_url_.DestroyNoArena(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  value_.DestroyNoArena(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
}

void Any::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::cv::protobuf::Descriptor* Any::descriptor() {
  ::protobuf_google_2fprotobuf_2fany_2eproto::protobuf_AssignDescriptorsOnce();
  return ::protobuf_google_2fprotobuf_2fany_2eproto::file_level_metadata[kIndexInFileMessages].descriptor;
}

const Any& Any::default_instance() {
  ::protobuf_google_2fprotobuf_2fany_2eproto::InitDefaultsAny();
  return *internal_default_instance();
}

Any* Any::New(::cv::protobuf::Arena* arena) const {
  Any* n = new Any;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void Any::Clear() {
// @@protoc_insertion_point(message_clear_start:google.protobuf.Any)
  ::cv::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  type_url_.ClearToEmptyNoArena(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  value_.ClearToEmptyNoArena(&::cv::protobuf::internal::GetEmptyStringAlreadyInited());
  _internal_metadata_.Clear();
}

bool Any::MergePartialFromCodedStream(
    ::cv::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::cv::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:google.protobuf.Any)
  for (;;) {
    ::std::pair< ::cv::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::cv::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // string type_url = 1;
      case 1: {
        if (static_cast< ::cv::protobuf::uint8>(tag) ==
            static_cast< ::cv::protobuf::uint8>(10u /* 10 & 0xFF */)) {
          DO_(::cv::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_type_url()));
          DO_(::cv::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->type_url().data(), static_cast<int>(this->type_url().length()),
            ::cv::protobuf::internal::WireFormatLite::PARSE,
            "google.protobuf.Any.type_url"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // bytes value = 2;
      case 2: {
        if (static_cast< ::cv::protobuf::uint8>(tag) ==
            static_cast< ::cv::protobuf::uint8>(18u /* 18 & 0xFF */)) {
          DO_(::cv::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_value()));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::cv::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:google.protobuf.Any)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:google.protobuf.Any)
  return false;
#undef DO_
}

void Any::SerializeWithCachedSizes(
    ::cv::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:google.protobuf.Any)
  ::cv::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string type_url = 1;
  if (this->type_url().size() > 0) {
    ::cv::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->type_url().data(), static_cast<int>(this->type_url().length()),
      ::cv::protobuf::internal::WireFormatLite::SERIALIZE,
      "google.protobuf.Any.type_url");
    ::cv::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->type_url(), output);
  }

  // bytes value = 2;
  if (this->value().size() > 0) {
    ::cv::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      2, this->value(), output);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::cv::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    ::cv::protobuf::internal::WireFormat::SerializeUnknownFields(
        (::cv::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), output);
  }
  // @@protoc_insertion_point(serialize_end:google.protobuf.Any)
}

::cv::protobuf::uint8* Any::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::cv::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:google.protobuf.Any)
  ::cv::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string type_url = 1;
  if (this->type_url().size() > 0) {
    ::cv::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->type_url().data(), static_cast<int>(this->type_url().length()),
      ::cv::protobuf::internal::WireFormatLite::SERIALIZE,
      "google.protobuf.Any.type_url");
    target =
      ::cv::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->type_url(), target);
  }

  // bytes value = 2;
  if (this->value().size() > 0) {
    target =
      ::cv::protobuf::internal::WireFormatLite::WriteBytesToArray(
        2, this->value(), target);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::cv::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    target = ::cv::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        (::cv::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:google.protobuf.Any)
  return target;
}

size_t Any::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:google.protobuf.Any)
  size_t total_size = 0;

  if ((_internal_metadata_.have_unknown_fields() &&  ::cv::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    total_size +=
      ::cv::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        (::cv::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()));
  }
  // string type_url = 1;
  if (this->type_url().size() > 0) {
    total_size += 1 +
      ::cv::protobuf::internal::WireFormatLite::StringSize(
        this->type_url());
  }

  // bytes value = 2;
  if (this->value().size() > 0) {
    total_size += 1 +
      ::cv::protobuf::internal::WireFormatLite::BytesSize(
        this->value());
  }

  int cached_size = ::cv::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Any::MergeFrom(const ::cv::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:google.protobuf.Any)
  GOOGLE_DCHECK_NE(&from, this);
  const Any* source =
      ::cv::protobuf::internal::DynamicCastToGenerated<const Any>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:google.protobuf.Any)
    ::cv::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:google.protobuf.Any)
    MergeFrom(*source);
  }
}

void Any::MergeFrom(const Any& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:google.protobuf.Any)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::cv::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.type_url().size() > 0) {

    type_url_.AssignWithDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited(), from.type_url_);
  }
  if (from.value().size() > 0) {

    value_.AssignWithDefault(&::cv::protobuf::internal::GetEmptyStringAlreadyInited(), from.value_);
  }
}

void Any::CopyFrom(const ::cv::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:google.protobuf.Any)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Any::CopyFrom(const Any& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:google.protobuf.Any)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Any::IsInitialized() const {
  return true;
}

void Any::Swap(Any* other) {
  if (other == this) return;
  InternalSwap(other);
}
void Any::InternalSwap(Any* other) {
  using std::swap;
  type_url_.Swap(&other->type_url_);
  value_.Swap(&other->value_);
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(_cached_size_, other->_cached_size_);
}

::cv::protobuf::Metadata Any::GetMetadata() const {
  protobuf_google_2fprotobuf_2fany_2eproto::protobuf_AssignDescriptorsOnce();
  return ::protobuf_google_2fprotobuf_2fany_2eproto::file_level_metadata[kIndexInFileMessages];
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace protobuf
}  // namespace cv

// @@protoc_insertion_point(global_scope)
