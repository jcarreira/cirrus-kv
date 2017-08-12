#include <unistd.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <memory>
#include <chrono>
#include <thread>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"
#include "client/BladeClient.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "iterator/IteratorPolicy.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const unsigned int SIZE = 1;
const char *IP;
bool use_rdma_client;

using ObjectID = uint64_t;

/**
 * An IteratorPolicy that will traverse the specified range in reverse order
 * from the highest oid to the lowest. This range is only traversed once.
 */
class ReversePolicy : public cirrus::IteratorPolicy {
 public:
    /**
     * Default constructor.
     */
    ReversePolicy() {}
    /**
     * Copy constructor, used by clone method.
     */
    explicit ReversePolicy(const ReversePolicy& other): first(other.first),
        last(other.last), read_ahead(other.read_ahead),
        current_id(other.current_id) {}
    /**
     * Used to set the internal state of the policy after creation.
     * @param first_ the first ObjectID in a continuous range.
     * @param last_ the last ObjectID in the continuous range.
     * @param read_ahead_ how many items ahead the iterator shold prefetch.
     * @param position_ an enum indicating if this instance of the policy
     * should instantiate itself at the beginning or end of the range.
     */
    void setState(ObjectID first_, ObjectID last_, uint64_t read_ahead_,
        Position position_) override {
        first = first_;
        last = last_;
        read_ahead = read_ahead_;

        if (position_ == kBegin) {
            current_id = last;
        } else if (position_ == kEnd) {
            current_id = first - 1;
        } else {
            throw cirrus::Exception("Unrecognized position argument.");
        }
    }
    std::vector<ObjectID> getPrefetchList() override {
        std::vector<ObjectID> prefetch_vector;
        prefetch_vector.reserve(read_ahead);
        for (unsigned int i = 1; i <= read_ahead; i++) {
            ObjectID tentative_fetch = current_id - i;
            ObjectID shifted = tentative_fetch - first;
            ObjectID modded = shifted % (last - first + 1);
            ObjectID to_fetch = modded + first;
            if (to_fetch == current_id) {
                break;
            }
            prefetch_vector.push_back(to_fetch);
        }
        return prefetch_vector;
    }

    /**
     * Returns the ObjectID at the current position.
     */
    ObjectID dereference() override {
        return current_id;
    }

    /**
     * Increments the internal state of the policy, moving it to the next
     * ObjectID to be returned.
     */
    void increment() override {
        current_id--;
    }

    /**
     * Returns the value of current_id, which is indicative of the state
     * of the policy.
     */
    uint64_t getState() override {
        return current_id;
    }

    /**
     * An implementation of the clone method from the template.
     */
    std::unique_ptr<IteratorPolicy> clone() override {
        return std::make_unique<ReversePolicy>(*this);
    }

 private:
    /** First ObjectID in the continuous range. */
    ObjectID first;
    /** Last ObjectID in the continuous range. */
    ObjectID last;
    /** How many items ahead to prefetch. */
    uint64_t read_ahead;
    /** The ObjectID that will be returned when dereference is called. */
    ObjectID current_id;
};


/**
  * This test ensures that items can be properly retrieved using
  * the iterator interface.
  */
void test_iterator() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            client.get(),
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);

    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 1, 0, 9);

    int j = 0;
    for (auto it = iter.begin(); it != iter.end(); it++) {
      cirrus::Dummy<SIZE> val = *it;
      if (val.id != j) {
        std::cout << "received " << val.id << " but expected " << j
                  << std::endl;
        throw std::runtime_error("Wrong value");
      }
      j++;
    }
}


/**
  * This test ensures that items can be properly retrieved using
  * the iterator interface, but using c++ range based for loop.
  */
void test_iterator_alt() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            client.get(),
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 1, 0, 9);

    int j = 0;
    for (const auto& data : iter) {
        if (data.id != j) {
            std::cout << "received " << data.id << " but expected " << j
                  << std::endl;
            throw std::runtime_error("Wrong value in alternate");
        }
        j++;
    }
}

/**
 * This test ensures that random prefetching works as expected.
 */
void test_random_prefetching() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            client.get(),
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    // Read 9 ahead so that all objects will be stashed
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 9, 0, 9);
    iter.setMode(cirrus::CirrusIterable<cirrus::Dummy<SIZE>>::kUnordered);
    int j = 0;
    auto start = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;

    for (const auto& data __attribute__((unused)) : iter) {
        end = std::chrono::system_clock::now();
        if (j != 0) {
            // Time how long this last loop took
            duration = end - start;
            auto duration_micro =
                std::chrono::duration_cast<std::chrono::microseconds>(duration);

            // Only if > 1 because the first one seems to sometimes be slow
            if (j > 1) {
                if (duration_micro.count() > 10) {
                    std::cout << "Elapsed is: " << duration_micro.count()
                        << std::endl;
                    throw std::runtime_error("Get took too long, likely not "
                           "prefetched");
                }
            }
        } else {
            // Sleep a bit to let the prefetches finish
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        start = std::chrono::system_clock::now();
        j++;
    }
    if (j != 10) {
        std::cout << j << " items iterated over instead of ten." << std::endl;
        throw std::runtime_error("Either greater or fewer than ten items were "
                "iterated over, meaning either that some items were skipped "
                "or that some were repeated.");
    }
}

/**
 * This test ensures that custom iterators work as expected.
 */
void test_custom_iteration() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            client.get(),
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    // Read 9 ahead so that all objects will be stashed
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 9, 0, 9);

    ReversePolicy reversemode;
    iter.setMode(cirrus::CirrusIterable<cirrus::Dummy<SIZE>>::kCustom,
        &reversemode);

    auto start = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;

    int j = 9;
    for (const auto& data : iter) {
        end = std::chrono::system_clock::now();
        if (j != 9) {
            // Time how long this last loop took
            duration = end - start;
            auto duration_micro =
                std::chrono::duration_cast<std::chrono::microseconds>(duration);

            // Only if after the second because the first few are slow
            if (j < 8) {
                if (duration_micro.count() > 10) {
                    std::cout << "Elapsed is: " << duration_micro.count()
                        << std::endl;
                    throw std::runtime_error("Get took too long, likely not "
                           "prefetched");
                }
            }
        } else {
            // Sleep a bit to let the prefetches finish
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (j != data.id) {
            std::cout << "Expected " << j << " but got "
                << data.id << std::endl;
            throw std::runtime_error("Wrong value returned.");
        }
        start = std::chrono::system_clock::now();
        j--;
    }
    if (j != -1) {
        std::cout << 9 - j << " items iterated over instead of ten."
            << std::endl;
        throw std::runtime_error("Either greater or fewer than ten items were "
                "iterated over, meaning either that some items were skipped "
                "or that some were repeated.");
    }
}


/**
  * Tests that the iterator works with c style arrays.
  */
void test_array() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);

    auto deserializer = cirrus::c_array_deserializer_simple<int>(4);
    auto serializer =
        cirrus::c_int_array_serializer_simple<std::shared_ptr<int>>(4);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<int>> store(IP,
                      PORT,
                      client.get(),
                      serializer,
                      deserializer);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<std::shared_ptr<int>> cm(&store, &policy, 10);

    auto int_array = std::shared_ptr<int>(new int[4],
        std::default_delete<int[]>());

    // Put items in the store
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++) {
            (int_array.get())[j] = (i * 4) + j;
        }
        cm.put(i, int_array);
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<std::shared_ptr<int>> iter(&cm, 1, 0, 9);

    int i = 0;
    for (const auto& ret_array : iter) {
        for (int j = 0; j < 4; j++) {
            if ((ret_array.get())[j] != (i * 4) + j) {
                std::cout << "Expected: " << (i * 4) + j << " but got "
                    << (ret_array.get())[j]
                    << std::endl;
                throw std::runtime_error("Incorrect value returned");
            }
        }
        i++;
    }
}

auto main(int argc, char *argv[]) -> int {
    std::cout << "Test starting" << std::endl;
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test Started." << std::endl;
    test_iterator();
    std::cout << "Starting iterator alt test." << std::endl;
    test_iterator_alt();
    test_array();
    test_random_prefetching();
    test_custom_iteration();
    std::cout << "Test successful" << std::endl;
    return 0;
}
