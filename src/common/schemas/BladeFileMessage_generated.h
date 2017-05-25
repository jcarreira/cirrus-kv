// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_BLADEFILEMESSAGE_MESSAGE_BLADEFILEMESSAGE_H_
#define FLATBUFFERS_GENERATED_BLADEFILEMESSAGE_MESSAGE_BLADEFILEMESSAGE_H_

#include "flatbuffers/flatbuffers.h"

namespace Message {
namespace BladeFileMessage {

struct alloc;

struct alloc_ack;

struct BladeFileMessage;

enum Data {
  Data_NONE = 0,
  Data_alloc = 1,
  Data_alloc_ack = 2,
  Data_MIN = Data_NONE,
  Data_MAX = Data_alloc_ack
};

inline const char **EnumNamesData() {
  static const char *names[] = {
    "NONE",
    "alloc",
    "alloc_ack",
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

template<> struct DataTraits<alloc> {
  static const Data enum_value = Data_alloc;
};

template<> struct DataTraits<alloc_ack> {
  static const Data enum_value = Data_alloc_ack;
};

bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type);
bool VerifyDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types);

struct alloc FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_SIZE = 4,
    VT_FILENAME = 6
  };
  uint64_t size() const {
    return GetField<uint64_t>(VT_SIZE, 0);
  }
  const flatbuffers::Vector<int8_t> *filename() const {
    return GetPointer<const flatbuffers::Vector<int8_t> *>(VT_FILENAME);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_SIZE) &&
           VerifyOffset(verifier, VT_FILENAME) &&
           verifier.Verify(filename()) &&
           verifier.EndTable();
  }
};

struct allocBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_size(uint64_t size) {
    fbb_.AddElement<uint64_t>(alloc::VT_SIZE, size, 0);
  }
  void add_filename(flatbuffers::Offset<flatbuffers::Vector<int8_t>> filename) {
    fbb_.AddOffset(alloc::VT_FILENAME, filename);
  }
  allocBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  allocBuilder &operator=(const allocBuilder &);
  flatbuffers::Offset<alloc> Finish() {
    const auto end = fbb_.EndTable(start_, 2);
    auto o = flatbuffers::Offset<alloc>(end);
    return o;
  }
};

inline flatbuffers::Offset<alloc> Createalloc(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t size = 0,
    flatbuffers::Offset<flatbuffers::Vector<int8_t>> filename = 0) {
  allocBuilder builder_(_fbb);
  builder_.add_size(size);
  builder_.add_filename(filename);
  return builder_.Finish();
}

inline flatbuffers::Offset<alloc> CreateallocDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t size = 0,
    const std::vector<int8_t> *filename = nullptr) {
  return Message::BladeFileMessage::Createalloc(
      _fbb,
      size,
      filename ? _fbb.CreateVector<int8_t>(*filename) : 0);
}

struct alloc_ack FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_REMOTE_ADDR = 4,
    VT_PEER_RKEY = 6
  };
  uint64_t remote_addr() const {
    return GetField<uint64_t>(VT_REMOTE_ADDR, 0);
  }
  uint64_t peer_rkey() const {
    return GetField<uint64_t>(VT_PEER_RKEY, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_REMOTE_ADDR) &&
           VerifyField<uint64_t>(verifier, VT_PEER_RKEY) &&
           verifier.EndTable();
  }
};

struct alloc_ackBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_remote_addr(uint64_t remote_addr) {
    fbb_.AddElement<uint64_t>(alloc_ack::VT_REMOTE_ADDR, remote_addr, 0);
  }
  void add_peer_rkey(uint64_t peer_rkey) {
    fbb_.AddElement<uint64_t>(alloc_ack::VT_PEER_RKEY, peer_rkey, 0);
  }
  alloc_ackBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  alloc_ackBuilder &operator=(const alloc_ackBuilder &);
  flatbuffers::Offset<alloc_ack> Finish() {
    const auto end = fbb_.EndTable(start_, 2);
    auto o = flatbuffers::Offset<alloc_ack>(end);
    return o;
  }
};

inline flatbuffers::Offset<alloc_ack> Createalloc_ack(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t remote_addr = 0,
    uint64_t peer_rkey = 0) {
  alloc_ackBuilder builder_(_fbb);
  builder_.add_peer_rkey(peer_rkey);
  builder_.add_remote_addr(remote_addr);
  return builder_.Finish();
}

struct BladeFileMessage FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
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
  const alloc *data_as_alloc() const {
    return data_type() == Data_alloc ? static_cast<const alloc *>(data()) : nullptr;
  }
  const alloc_ack *data_as_alloc_ack() const {
    return data_type() == Data_alloc_ack ? static_cast<const alloc_ack *>(data()) : nullptr;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_DATA_TYPE) &&
           VerifyOffset(verifier, VT_DATA) &&
           VerifyData(verifier, data(), data_type()) &&
           verifier.EndTable();
  }
};

template<> inline const alloc *BladeFileMessage::data_as<alloc>() const {
  return data_as_alloc();
}

template<> inline const alloc_ack *BladeFileMessage::data_as<alloc_ack>() const {
  return data_as_alloc_ack();
}

struct BladeFileMessageBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_data_type(Data data_type) {
    fbb_.AddElement<uint8_t>(BladeFileMessage::VT_DATA_TYPE, static_cast<uint8_t>(data_type), 0);
  }
  void add_data(flatbuffers::Offset<void> data) {
    fbb_.AddOffset(BladeFileMessage::VT_DATA, data);
  }
  BladeFileMessageBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  BladeFileMessageBuilder &operator=(const BladeFileMessageBuilder &);
  flatbuffers::Offset<BladeFileMessage> Finish() {
    const auto end = fbb_.EndTable(start_, 2);
    auto o = flatbuffers::Offset<BladeFileMessage>(end);
    return o;
  }
};

inline flatbuffers::Offset<BladeFileMessage> CreateBladeFileMessage(
    flatbuffers::FlatBufferBuilder &_fbb,
    Data data_type = Data_NONE,
    flatbuffers::Offset<void> data = 0) {
  BladeFileMessageBuilder builder_(_fbb);
  builder_.add_data(data);
  builder_.add_data_type(data_type);
  return builder_.Finish();
}

inline bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type) {
  switch (type) {
    case Data_NONE: {
      return true;
    }
    case Data_alloc: {
      auto ptr = reinterpret_cast<const alloc *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_alloc_ack: {
      auto ptr = reinterpret_cast<const alloc_ack *>(obj);
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

inline const Message::BladeFileMessage::BladeFileMessage *GetBladeFileMessage(const void *buf) {
  return flatbuffers::GetRoot<Message::BladeFileMessage::BladeFileMessage>(buf);
}

inline bool VerifyBladeFileMessageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<Message::BladeFileMessage::BladeFileMessage>(nullptr);
}

inline void FinishBladeFileMessageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Message::BladeFileMessage::BladeFileMessage> root) {
  fbb.Finish(root);
}

}  // namespace BladeFileMessage
}  // namespace Message

#endif  // FLATBUFFERS_GENERATED_BLADEFILEMESSAGE_MESSAGE_BLADEFILEMESSAGE_H_
