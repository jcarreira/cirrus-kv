#ifndef SRC_COMMON_SERIALIZER_H_
#define SRC_COMMON_SERIALIZER_H_

#include <arpa/inet.h>
#include <vector>

namespace cirrus {

/**
 * A template class outlining the interface that all serializers should have.
 */
template<class T>
class Serializer {
 public:
    /**
     * Purely virtual function. Should return size needed to serialize when
     * object is passed in.
     */
    virtual uint64_t size(const T& object) const = 0;
    /**
     * Purely virtual function. Should serialize the object passed in.
     * @param object the object to be serialized.
     * @param mem the memory the object should be serialized into.
     */
    virtual void serialize(const T& object, void *mem) const = 0;
};

class WriteUnit {
 public:
    virtual uint64_t size() const = 0;
    virtual void serialize(void *mem) const = 0;
};

template<class T>
class WriteUnitTemplate : public WriteUnit {
 public:
    WriteUnitTemplate(const cirrus::Serializer<T>& serializer, const T& obj) :
        serializer(serializer), obj(obj) {}

    void serialize(void *mem) const override {
        serializer.serialize(obj, mem);
        return;
    }

    uint64_t size() const override {
        return serializer.size(obj);
    }

 private:
    const cirrus::Serializer<T>& serializer;
    const T& obj;
};

class WriteUnits {
 public:
    virtual uint64_t size() const = 0;
    virtual void serialize(void *mem) const = 0;
};

template<class T>
class WriteUnitsTemplate : public WriteUnits {
 public:
    WriteUnitsTemplate(const cirrus::Serializer<T>& serializer,
            const std::vector<const T*>& objs) :
        serializer(serializer), objs(objs) {}

    void serialize(void *mem) const override {
        char* ptr = reinterpret_cast<char*>(mem);
        for (uint64_t i = 0; i < objs.size(); ++i) {
            uint64_t object_size = serializer.size(*objs[i]);
            *reinterpret_cast<uint64_t*>(ptr) = htonl(object_size);
            ptr += sizeof(uint64_t);
            serializer.serialize(*objs[i], ptr);
            ptr += object_size;
        }
    }

    uint64_t size() const override {
        uint64_t total_size = 0;
        for (uint64_t i = 0; i < objs.size(); ++i) {
            total_size += serializer.size(*objs[i]) + sizeof(uint64_t);
        }
        return total_size;
    }

 private:
    const cirrus::Serializer<T>& serializer;
    const std::vector<const T*> objs;
};

}  // namespace cirrus

#endif  // SRC_COMMON_SERIALIZER_H_
