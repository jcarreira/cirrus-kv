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
    virtual void serialize(const T& object, void* mem) const = 0;
};

}  // namespace cirrus

#endif  // SRC_COMMON_SERIALIZER_H_
