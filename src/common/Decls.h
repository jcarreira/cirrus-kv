#ifndef SRC_COMMON_DECLS_H_
#define SRC_COMMON_DECLS_H_

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
      TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

#endif  // SRC_COMMON_DECLS_H_
