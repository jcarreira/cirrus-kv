// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_BLADEMESSAGE_CIRRUS_MESSAGE_BLADEMESSAGE_H_
#define FLATBUFFERS_GENERATED_BLADEMESSAGE_CIRRUS_MESSAGE_BLADEMESSAGE_H_

#include "flatbuffers/flatbuffers.h"

namespace cirrus {
namespace Message {
namespace BladeMessage {

struct Alloc;

struct AllocAck;

struct Dealloc;

struct DeallocAck;

struct BladeMessage;

enum Data {
  Data_NONE = 0,
  Data_Alloc = 1,
  Data_AllocAck = 2,
  Data_Dealloc = 3,
  Data_DeallocAck = 4,
  Data_MIN = Data_NONE,
  Data_MAX = Data_DeallocAck
};

inline const char **EnumNamesData() {
  static const char *names[] = {
    "NONE",
    "Alloc",
    "AllocAck",
    "Dealloc",
    "DeallocAck",
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

struct BladeMessage FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
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
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_DATA_TYPE) &&
           VerifyOffset(verifier, VT_DATA) &&
           VerifyData(verifier, data(), data_type()) &&
           verifier.EndTable();
  }
};

template<> inline const Alloc *BladeMessage::data_as<Alloc>() const {
  return data_as_Alloc();
}

template<> inline const AllocAck *BladeMessage::data_as<AllocAck>() const {
  return data_as_AllocAck();
}

template<> inline const Dealloc *BladeMessage::data_as<Dealloc>() const {
  return data_as_Dealloc();
}

template<> inline const DeallocAck *BladeMessage::data_as<DeallocAck>() const {
  return data_as_DeallocAck();
}

struct BladeMessageBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_data_type(Data data_type) {
    fbb_.AddElement<uint8_t>(BladeMessage::VT_DATA_TYPE, static_cast<uint8_t>(data_type), 0);
  }
  void add_data(flatbuffers::Offset<void> data) {
    fbb_.AddOffset(BladeMessage::VT_DATA, data);
  }
  BladeMessageBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  BladeMessageBuilder &operator=(const BladeMessageBuilder &);
  flatbuffers::Offset<BladeMessage> Finish() {
    const auto end = fbb_.EndTable(start_, 2);
    auto o = flatbuffers::Offset<BladeMessage>(end);
    return o;
  }
};

inline flatbuffers::Offset<BladeMessage> CreateBladeMessage(
    flatbuffers::FlatBufferBuilder &_fbb,
    Data data_type = Data_NONE,
    flatbuffers::Offset<void> data = 0) {
  BladeMessageBuilder builder_(_fbb);
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

inline const cirrus::Message::BladeMessage::BladeMessage *GetBladeMessage(const void *buf) {
  return flatbuffers::GetRoot<cirrus::Message::BladeMessage::BladeMessage>(buf);
}

inline bool VerifyBladeMessageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<cirrus::Message::BladeMessage::BladeMessage>(nullptr);
}

inline void FinishBladeMessageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<cirrus::Message::BladeMessage::BladeMessage> root) {
  fbb.Finish(root);
}

}  // namespace BladeMessage
}  // namespace Message
}  // namespace cirrus

#endif  // FLATBUFFERS_GENERATED_BLADEMESSAGE_CIRRUS_MESSAGE_BLADEMESSAGE_H_
