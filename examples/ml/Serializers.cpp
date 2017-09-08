#include <examples/ml/Serializers.h>
#include <iostream>

uint64_t lr_model_serializer::size(const LRModel& model) const {
    return model.getSerializedSize();
}

void lr_model_serializer::serialize(const LRModel& model, void* mem) const {
    model.serializeTo(mem);
}

LRModel
lr_model_deserializer::operator()(const void* data, unsigned int des_size) {
    LRModel model(n);
    if (des_size != model.getSerializedSize()) {
        throw std::runtime_error(
                "Wrong deserializer size at lr_model_deserializer");
    }
    model.loadSerialized(data);

    return model;
}

uint64_t lr_gradient_serializer::size(const LRGradient& g) const {
    return g.getSerializedSize();
}

void lr_gradient_serializer::serialize(const LRGradient& g, void* mem) const {
    g.serialize(mem);
}

LRGradient
lr_gradient_deserializer::operator()(const void* data, unsigned int des_size) {
    LRGradient gradient(n);
    if (des_size != gradient.getSerializedSize()) {
        throw std::runtime_error(
                "Wrong deserializer size at lr_gradient_deserializer");
    }

    gradient.loadSerialized(data);
    return gradient;
}

