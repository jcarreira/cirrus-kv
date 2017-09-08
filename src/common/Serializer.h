#ifndef SRC_COMMON_SERIALIZER_H_
#define SRC_COMMON_SERIALIZER_H_


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

}  // namespace cirrus

#endif  // SRC_COMMON_SERIALIZER_H_
