#include "resource/type/Texture.h"

namespace LUS {
void* Texture::GetPointer() {
    return ImageData;
}

size_t Texture::GetPointerSize() {
    return ImageDataSize;
}

Texture::~Texture() {
    if (ImageData != nullptr) {
        delete ImageData;
    }
}
} // namespace LUS
