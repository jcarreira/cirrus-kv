// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_BLADEOBJECTSTOREMESSAGE_CIRRUS_MESSAGE_BLADEOBJECTSTOREMESSAGE_H_
#define FLATBUFFERS_GENERATED_BLADEOBJECTSTOREMESSAGE_CIRRUS_MESSAGE_BLADEOBJECTSTOREMESSAGE_H_

#include "flatbuffers/flatbuffers.h"

namespace cirrus {
namespace Message {
namespace BladeObjectStoreMessage {

struct Alloc;

struct AllocAck;

struct Dealloc;

struct DeallocAck;

struct KeepAlive;

struct KeepAliveAck;

struct Sub;

struct SubAck;

struct Flush;

struct FlushAck;

struct Lock;

struct LockAck;

struct BladeObjectStoreMessage;

enum Data {
  Data_NONE = 0,
  Data_Alloc = 1,
  Data_AllocAck = 2,
  Data_Dealloc = 3,
  Data_DeallocAck = 4,
  Data_KeepAlive = 5,
  Data_KeepAliveAck = 6,
  Data_Sub = 7,
  Data_SubAck = 8,
  Data_Flush = 9,
  Data_FlushAck = 10,
  Data_Lock = 11,
  Data_LockAck = 12,
  Data_MIN = Data_NONE,
  Data_MAX = Data_LockAck
};

inline const char **EnumNamesData() {
  static const char *names[] = {
    "NONE",
    "Alloc",
    "AllocAck",
    "Dealloc",
    "DeallocAck",
    "KeepAlive",
    "KeepAliveAck",
    "Sub",
    "SubAck",
    "Flush",
    "FlushAck",
    "Lock",
    "LockAck",
    nullptr
  };
  return names;
}

inline const char *EnumNameData(Data e) {
  const size_t index = static_cast<int>(e);
  return EnumNamesData()[index];
}

template<typename T> struct DataTraits {
  static const Data enum_value = Data_NONE;
};

template<> struct DataTraits<Alloc> {
  static const Data enum_value = Data_Alloc;
};

template<> struct DataTraits<AllocAck> {
  static const Data enum_value = Data_AllocAck;
};

template<> struct DataTraits<Dealloc> {
  static const Data enum_value = Data_Dealloc;
};

template<> struct DataTraits<DeallocAck> {
  static const Data enum_value = Data_DeallocAck;
};

template<> struct DataTraits<KeepAlive> {
  static const Data enum_value = Data_KeepAlive;
};

template<> struct DataTraits<KeepAliveAck> {
  static const Data enum_value = Data_KeepAliveAck;
};

template<> struct DataTraits<Sub> {
  static const Data enum_value = Data_Sub;
};

template<> struct DataTraits<SubAck> {
  static const Data enum_value = Data_SubAck;
};

template<> struct DataTraits<Flush> {
  static const Data enum_value = Data_Flush;
};

template<> struct DataTraits<FlushAck> {
  static const Data enum_value = Data_FlushAck;
};

template<> struct DataTraits<Lock> {
  static const Data enum_value = Data_Lock;
};

template<> struct DataTraits<LockAck> {
  static const Data enum_value = Data_LockAck;
};

bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type);
bool VerifyDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types);

struct Alloc FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_SIZE = 4
  };
  uint64_t size() const {
    return GetField<uint64_t>(VT_SIZE, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_SIZE) &&
           verifier.EndTable();
  }
};

struct AllocBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_size(uint64_t size) {
    fbb_.AddElement<uint64_t>(Alloc::VT_SIZE, size, 0);
  }
  AllocBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  AllocBuilder &operator=(const AllocBuilder &);
  flatbuffers::Offset<Alloc> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<Alloc>(end);
    return o;
  }
};

inline flatbuffers::Offset<Alloc> CreateAlloc(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t size = 0) {
  AllocBuilder builder_(_fbb);
  builder_.add_size(size);
  return builder_.Finish();
}

struct AllocAck FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_MR_ID = 4,
    VT_REMOTE_ADDR = 6,
    VT_PEER_RKEY = 8
  };
  uint64_t mr_id() const {
    return GetField<uint64_t>(VT_MR_ID, 0);
  }
  uint64_t remote_addr() const {
    return GetField<uint64_t>(VT_REMOTE_ADDR, 0);
  }
  uint64_t peer_rkey() const {
    return GetField<uint64_t>(VT_PEER_RKEY, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_MR_ID) &&
           VerifyField<uint64_t>(verifier, VT_REMOTE_ADDR) &&
           VerifyField<uint64_t>(verifier, VT_PEER_RKEY) &&
           verifier.EndTable();
  }
};

struct AllocAckBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_mr_id(uint64_t mr_id) {
    fbb_.AddElement<uint64_t>(AllocAck::VT_MR_ID, mr_id, 0);
  }
  void add_remote_addr(uint64_t remote_addr) {
    fbb_.AddElement<uint64_t>(AllocAck::VT_REMOTE_ADDR, remote_addr, 0);
  }
  void add_peer_rkey(uint64_t peer_rkey) {
    fbb_.AddElement<uint64_t>(AllocAck::VT_PEER_RKEY, peer_rkey, 0);
  }
  AllocAckBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  AllocAckBuilder &operator=(const AllocAckBuilder &);
  flatbuffers::Offset<AllocAck> Finish() {
    const auto end = fbb_.EndTable(start_, 3);
    auto o = flatbuffers::Offset<AllocAck>(end);
    return o;
  }
};

inline flatbuffers::Offset<AllocAck> CreateAllocAck(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t mr_id = 0,
    uint64_t remote_addr = 0,
    uint64_t peer_rkey = 0) {
  AllocAckBuilder builder_(_fbb);
  builder_.add_peer_rkey(peer_rkey);
  builder_.add_remote_addr(remote_addr);
  builder_.add_mr_id(mr_id);
  return builder_.Finish();
}

struct Dealloc FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ADDR = 4
  };
  uint64_t addr() const {
    return GetField<uint64_t>(VT_ADDR, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_ADDR) &&
           verifier.EndTable();
  }
};

struct DeallocBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_addr(uint64_t addr) {
    fbb_.AddElement<uint64_t>(Dealloc::VT_ADDR, addr, 0);
  }
  DeallocBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  DeallocBuilder &operator=(const DeallocBuilder &);
  flatbuffers::Offset<Dealloc> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<Dealloc>(end);
    return o;
  }
};

inline flatbuffers::Offset<Dealloc> CreateDealloc(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t addr = 0) {
  DeallocBuilder builder_(_fbb);
  builder_.add_addr(addr);
  return builder_.Finish();
}

struct DeallocAck FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_RESULT = 4
  };
  int8_t result() const {
    return GetField<int8_t>(VT_RESULT, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int8_t>(verifier, VT_RESULT) &&
           verifier.EndTable();
  }
};

struct DeallocAckBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_result(int8_t result) {
    fbb_.AddElement<int8_t>(DeallocAck::VT_RESULT, result, 0);
  }
  DeallocAckBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  DeallocAckBuilder &operator=(const DeallocAckBuilder &);
  flatbuffers::Offset<DeallocAck> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<DeallocAck>(end);
    return o;
  }
};

inline flatbuffers::Offset<DeallocAck> CreateDeallocAck(
    flatbuffers::FlatBufferBuilder &_fbb,
    int8_t result = 0) {
  DeallocAckBuilder builder_(_fbb);
  builder_.add_result(result);
  return builder_.Finish();
}

struct KeepAlive FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_RAND = 4
  };
  uint64_t rand() const {
    return GetField<uint64_t>(VT_RAND, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_RAND) &&
           verifier.EndTable();
  }
};

struct KeepAliveBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_rand(uint64_t rand) {
    fbb_.AddElement<uint64_t>(KeepAlive::VT_RAND, rand, 0);
  }
  KeepAliveBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  KeepAliveBuilder &operator=(const KeepAliveBuilder &);
  flatbuffers::Offset<KeepAlive> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<KeepAlive>(end);
    return o;
  }
};

inline flatbuffers::Offset<KeepAlive> CreateKeepAlive(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t rand = 0) {
  KeepAliveBuilder builder_(_fbb);
  builder_.add_rand(rand);
  return builder_.Finish();
}

struct KeepAliveAck FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_RAND = 4
  };
  uint64_t rand() const {
    return GetField<uint64_t>(VT_RAND, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_RAND) &&
           verifier.EndTable();
  }
};

struct KeepAliveAckBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_rand(uint64_t rand) {
    fbb_.AddElement<uint64_t>(KeepAliveAck::VT_RAND, rand, 0);
  }
  KeepAliveAckBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  KeepAliveAckBuilder &operator=(const KeepAliveAckBuilder &);
  flatbuffers::Offset<KeepAliveAck> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<KeepAliveAck>(end);
    return o;
  }
};

inline flatbuffers::Offset<KeepAliveAck> CreateKeepAliveAck(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t rand = 0) {
  KeepAliveAckBuilder builder_(_fbb);
  builder_.add_rand(rand);
  return builder_.Finish();
}

struct Sub FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_OID = 4,
    VT_ADDR = 6
  };
  uint64_t oid() const {
    return GetField<uint64_t>(VT_OID, 0);
  }
  const flatbuffers::String *addr() const {
    return GetPointer<const flatbuffers::String *>(VT_ADDR);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_OID) &&
           VerifyOffset(verifier, VT_ADDR) &&
           verifier.Verify(addr()) &&
           verifier.EndTable();
  }
};

struct SubBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_oid(uint64_t oid) {
    fbb_.AddElement<uint64_t>(Sub::VT_OID, oid, 0);
  }
  void add_addr(flatbuffers::Offset<flatbuffers::String> addr) {
    fbb_.AddOffset(Sub::VT_ADDR, addr);
  }
  SubBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SubBuilder &operator=(const SubBuilder &);
  flatbuffers::Offset<Sub> Finish() {
    const auto end = fbb_.EndTable(start_, 2);
    auto o = flatbuffers::Offset<Sub>(end);
    return o;
  }
};

inline flatbuffers::Offset<Sub> CreateSub(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t oid = 0,
    flatbuffers::Offset<flatbuffers::String> addr = 0) {
  SubBuilder builder_(_fbb);
  builder_.add_oid(oid);
  builder_.add_addr(addr);
  return builder_.Finish();
}

inline flatbuffers::Offset<Sub> CreateSubDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t oid = 0,
    const char *addr = nullptr) {
  return cirrus::Message::BladeObjectStoreMessage::CreateSub(
      _fbb,
      oid,
      addr ? _fbb.CreateString(addr) : 0);
}

struct SubAck FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_OID = 4
  };
  uint64_t oid() const {
    return GetField<uint64_t>(VT_OID, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_OID) &&
           verifier.EndTable();
  }
};

struct SubAckBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_oid(uint64_t oid) {
    fbb_.AddElement<uint64_t>(SubAck::VT_OID, oid, 0);
  }
  SubAckBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SubAckBuilder &operator=(const SubAckBuilder &);
  flatbuffers::Offset<SubAck> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<SubAck>(end);
    return o;
  }
};

inline flatbuffers::Offset<SubAck> CreateSubAck(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t oid = 0) {
  SubAckBuilder builder_(_fbb);
  builder_.add_oid(oid);
  return builder_.Finish();
}

struct Flush FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_OID = 4
  };
  uint64_t oid() const {
    return GetField<uint64_t>(VT_OID, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_OID) &&
           verifier.EndTable();
  }
};

struct FlushBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_oid(uint64_t oid) {
    fbb_.AddElement<uint64_t>(Flush::VT_OID, oid, 0);
  }
  FlushBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FlushBuilder &operator=(const FlushBuilder &);
  flatbuffers::Offset<Flush> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<Flush>(end);
    return o;
  }
};

inline flatbuffers::Offset<Flush> CreateFlush(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t oid = 0) {
  FlushBuilder builder_(_fbb);
  builder_.add_oid(oid);
  return builder_.Finish();
}

struct FlushAck FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_OID = 4
  };
  uint64_t oid() const {
    return GetField<uint64_t>(VT_OID, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_OID) &&
           verifier.EndTable();
  }
};

struct FlushAckBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_oid(uint64_t oid) {
    fbb_.AddElement<uint64_t>(FlushAck::VT_OID, oid, 0);
  }
  FlushAckBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FlushAckBuilder &operator=(const FlushAckBuilder &);
  flatbuffers::Offset<FlushAck> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<FlushAck>(end);
    return o;
  }
};

inline flatbuffers::Offset<FlushAck> CreateFlushAck(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t oid = 0) {
  FlushAckBuilder builder_(_fbb);
  builder_.add_oid(oid);
  return builder_.Finish();
}

struct Lock FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4
  };
  uint64_t id() const {
    return GetField<uint64_t>(VT_ID, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_ID) &&
           verifier.EndTable();
  }
};

struct LockBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(uint64_t id) {
    fbb_.AddElement<uint64_t>(Lock::VT_ID, id, 0);
  }
  LockBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  LockBuilder &operator=(const LockBuilder &);
  flatbuffers::Offset<Lock> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<Lock>(end);
    return o;
  }
};

inline flatbuffers::Offset<Lock> CreateLock(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t id = 0) {
  LockBuilder builder_(_fbb);
  builder_.add_id(id);
  return builder_.Finish();
}

struct LockAck FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4
  };
  uint64_t id() const {
    return GetField<uint64_t>(VT_ID, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_ID) &&
           verifier.EndTable();
  }
};

struct LockAckBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(uint64_t id) {
    fbb_.AddElement<uint64_t>(LockAck::VT_ID, id, 0);
  }
  LockAckBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  LockAckBuilder &operator=(const LockAckBuilder &);
  flatbuffers::Offset<LockAck> Finish() {
    const auto end = fbb_.EndTable(start_, 1);
    auto o = flatbuffers::Offset<LockAck>(end);
    return o;
  }
};

inline flatbuffers::Offset<LockAck> CreateLockAck(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t id = 0) {
  LockAckBuilder builder_(_fbb);
  builder_.add_id(id);
  return builder_.Finish();
}

struct BladeObjectStoreMessage FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_DATA_TYPE = 4,
    VT_DATA = 6
  };
  Data data_type() const {
    return static_cast<Data>(GetField<uint8_t>(VT_DATA_TYPE, 0));
  }
  const void *data() const {
    return GetPointer<const void *>(VT_DATA);
  }
  template<typename T> const T *data_as() const;
  const Alloc *data_as_Alloc() const {
    return data_type() == Data_Alloc ? static_cast<const Alloc *>(data()) : nullptr;
  }
  const AllocAck *data_as_AllocAck() const {
    return data_type() == Data_AllocAck ? static_cast<const AllocAck *>(data()) : nullptr;
  }
  const Dealloc *data_as_Dealloc() const {
    return data_type() == Data_Dealloc ? static_cast<const Dealloc *>(data()) : nullptr;
  }
  const DeallocAck *data_as_DeallocAck() const {
    return data_type() == Data_DeallocAck ? static_cast<const DeallocAck *>(data()) : nullptr;
  }
  const KeepAlive *data_as_KeepAlive() const {
    return data_type() == Data_KeepAlive ? static_cast<const KeepAlive *>(data()) : nullptr;
  }
  const KeepAliveAck *data_as_KeepAliveAck() const {
    return data_type() == Data_KeepAliveAck ? static_cast<const KeepAliveAck *>(data()) : nullptr;
  }
  const Sub *data_as_Sub() const {
    return data_type() == Data_Sub ? static_cast<const Sub *>(data()) : nullptr;
  }
  const SubAck *data_as_SubAck() const {
    return data_type() == Data_SubAck ? static_cast<const SubAck *>(data()) : nullptr;
  }
  const Flush *data_as_Flush() const {
    return data_type() == Data_Flush ? static_cast<const Flush *>(data()) : nullptr;
  }
  const FlushAck *data_as_FlushAck() const {
    return data_type() == Data_FlushAck ? static_cast<const FlushAck *>(data()) : nullptr;
  }
  const Lock *data_as_Lock() const {
    return data_type() == Data_Lock ? static_cast<const Lock *>(data()) : nullptr;
  }
  const LockAck *data_as_LockAck() const {
    return data_type() == Data_LockAck ? static_cast<const LockAck *>(data()) : nullptr;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_DATA_TYPE) &&
           VerifyOffset(verifier, VT_DATA) &&
           VerifyData(verifier, data(), data_type()) &&
           verifier.EndTable();
  }
};

template<> inline const Alloc *BladeObjectStoreMessage::data_as<Alloc>() const {
  return data_as_Alloc();
}

template<> inline const AllocAck *BladeObjectStoreMessage::data_as<AllocAck>() const {
  return data_as_AllocAck();
}

template<> inline const Dealloc *BladeObjectStoreMessage::data_as<Dealloc>() const {
  return data_as_Dealloc();
}

template<> inline const DeallocAck *BladeObjectStoreMessage::data_as<DeallocAck>() const {
  return data_as_DeallocAck();
}

template<> inline const KeepAlive *BladeObjectStoreMessage::data_as<KeepAlive>() const {
  return data_as_KeepAlive();
}

template<> inline const KeepAliveAck *BladeObjectStoreMessage::data_as<KeepAliveAck>() const {
  return data_as_KeepAliveAck();
}

template<> inline const Sub *BladeObjectStoreMessage::data_as<Sub>() const {
  return data_as_Sub();
}

template<> inline const SubAck *BladeObjectStoreMessage::data_as<SubAck>() const {
  return data_as_SubAck();
}

template<> inline const Flush *BladeObjectStoreMessage::data_as<Flush>() const {
  return data_as_Flush();
}

template<> inline const FlushAck *BladeObjectStoreMessage::data_as<FlushAck>() const {
  return data_as_FlushAck();
}

template<> inline const Lock *BladeObjectStoreMessage::data_as<Lock>() const {
  return data_as_Lock();
}

template<> inline const LockAck *BladeObjectStoreMessage::data_as<LockAck>() const {
  return data_as_LockAck();
}

struct BladeObjectStoreMessageBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_data_type(Data data_type) {
    fbb_.AddElement<uint8_t>(BladeObjectStoreMessage::VT_DATA_TYPE, static_cast<uint8_t>(data_type), 0);
  }
  void add_data(flatbuffers::Offset<void> data) {
    fbb_.AddOffset(BladeObjectStoreMessage::VT_DATA, data);
  }
  BladeObjectStoreMessageBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  BladeObjectStoreMessageBuilder &operator=(const BladeObjectStoreMessageBuilder &);
  flatbuffers::Offset<BladeObjectStoreMessage> Finish() {
    const auto end = fbb_.EndTable(start_, 2);
    auto o = flatbuffers::Offset<BladeObjectStoreMessage>(end);
    return o;
  }
};

inline flatbuffers::Offset<BladeObjectStoreMessage> CreateBladeObjectStoreMessage(
    flatbuffers::FlatBufferBuilder &_fbb,
    Data data_type = Data_NONE,
    flatbuffers::Offset<void> data = 0) {
  BladeObjectStoreMessageBuilder builder_(_fbb);
  builder_.add_data(data);
  builder_.add_data_type(data_type);
  return builder_.Finish();
}

inline bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type) {
  switch (type) {
    case Data_NONE: {
      return true;
    }
    case Data_Alloc: {
      auto ptr = reinterpret_cast<const Alloc *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_AllocAck: {
      auto ptr = reinterpret_cast<const AllocAck *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Dealloc: {
      auto ptr = reinterpret_cast<const Dealloc *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_DeallocAck: {
      auto ptr = reinterpret_cast<const DeallocAck *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_KeepAlive: {
      auto ptr = reinterpret_cast<const KeepAlive *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_KeepAliveAck: {
      auto ptr = reinterpret_cast<const KeepAliveAck *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Sub: {
      auto ptr = reinterpret_cast<const Sub *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_SubAck: {
      auto ptr = reinterpret_cast<const SubAck *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Flush: {
      auto ptr = reinterpret_cast<const Flush *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_FlushAck: {
      auto ptr = reinterpret_cast<const FlushAck *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Lock: {
      auto ptr = reinterpret_cast<const Lock *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_LockAck: {
      auto ptr = reinterpret_cast<const LockAck *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return false;
  }
}

inline bool VerifyDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types) {
  if (values->size() != types->size()) return false;
  for (flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyData(
        verifier,  values->Get(i), types->GetEnum<Data>(i))) {
      return false;
    }
  }
  return true;
}

inline const cirrus::Message::BladeObjectStoreMessage::BladeObjectStoreMessage *GetBladeObjectStoreMessage(const void *buf) {
  return flatbuffers::GetRoot<cirrus::Message::BladeObjectStoreMessage::BladeObjectStoreMessage>(buf);
}

inline bool VerifyBladeObjectStoreMessageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<cirrus::Message::BladeObjectStoreMessage::BladeObjectStoreMessage>(nullptr);
}

inline void FinishBladeObjectStoreMessageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<cirrus::Message::BladeObjectStoreMessage::BladeObjectStoreMessage> root) {
  fbb.Finish(root);
}

}  // namespace BladeObjectStoreMessage
}  // namespace Message
}  // namespace cirrus

#endif  // FLATBUFFERS_GENERATED_BLADEOBJECTSTOREMESSAGE_CIRRUS_MESSAGE_BLADEOBJECTSTOREMESSAGE_H_
