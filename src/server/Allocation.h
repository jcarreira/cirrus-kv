#ifndef SRC_SERVER_ALLOCATION_H_
#define SRC_SERVER_ALLOCATION_H_



namespace cirrus {
/**
  * This base class describes a resource
  * reserved by one app.
  */
class Allocation {
 public:
    Allocation() {}
    virtual ~Allocation() {}
 private:
};

}  // namespace cirrus

#endif  // SRC_SERVER_ALLOCATION_H_
