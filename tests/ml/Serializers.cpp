#include <Serializers.h>

std::pair<std::unique_ptr<char[]>, unsigned int>
lr_model_serializer::operator()(const LRModel& v) {
    auto model_serialized = v.serialize();
    return model_serialized;
}

LRModel
lr_model_deserializer::operator()(void* data, unsigned int des_size) {
    //std::cout << "[DESER-" + name + "]"
    //    << " lr_model_deserializer des_size: " << des_size << std::endl;
    LRModel model(n);
    if (des_size != model.getSerializedSize()) {
        throw std::runtime_error(
                "Wrong deserializer size at lr_model_deserializer");
    }
    model.loadSerialized(data);

    return model;
}

std::pair<std::unique_ptr<char[]>, unsigned int>
lr_gradient_serializer::operator()(const LRGradient& g) {
    //std::cout << "[SER]"
    //    << " LRGradient serializer writing size: "
    //    << g.getSerializedSize()
    //    << std::endl;
    std::unique_ptr<char[]> mem(new char[g.getSerializedSize()]);
    g.serialize(mem.get());

    return std::make_pair(std::move(mem), g.getSerializedSize());
}

LRGradient
lr_gradient_deserializer::operator()(void* data, unsigned int des_size) {
    //std::cout << "[SER]"
    //    << " LRGradient deserializing n: " << n
    //    << " size: " << des_size
    //    << std::endl;
    LRGradient gradient(n);
    if (des_size != gradient.getSerializedSize()) {
        throw std::runtime_error(
                "Wrong deserializer size at lr_gradient_deserializer");
    }

    gradient.loadSerialized(data);
    return gradient;
}

