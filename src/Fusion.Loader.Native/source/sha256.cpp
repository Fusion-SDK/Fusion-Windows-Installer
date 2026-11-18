#include "fusion/loader/sha256.hpp"

#include <array>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/evp.h>
#endif

namespace fusion::loader {
namespace {
    constexpr char Hex[] = "0123456789abcdef";
}

bool ComputeFileSha256(
    const std::filesystem::path& path,
    std::string& hash,
    std::string& error)
{
    hash.clear();
    error.clear();
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Could not open package file for hashing.";
        return false;
    }

#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hashHandle = nullptr;
    std::vector<UCHAR> hashObject;
    std::array<UCHAR, 32> digest{};
    DWORD objectLength = 0;
    DWORD resultLength = 0;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status < 0) {
        error = "Could not initialize SHA-256 provider.";
        return false;
    }
    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength),
        &resultLength, 0);
    if (status >= 0) {
        hashObject.resize(objectLength);
        status = BCryptCreateHash(algorithm, &hashHandle,
            hashObject.data(), objectLength, nullptr, 0, 0);
    }
    if (status < 0) {
        if (hashHandle) BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        error = "Could not create SHA-256 hash.";
        return false;
    }

    std::array<char, 65536> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) {
            status = BCryptHashData(hashHandle,
                reinterpret_cast<PUCHAR>(buffer.data()),
                static_cast<ULONG>(count), 0);
            if (status < 0) break;
        }
    }
    if (status >= 0 && input.bad()) status = -1;
    if (status >= 0)
        status = BCryptFinishHash(hashHandle, digest.data(),
            static_cast<ULONG>(digest.size()), 0);
    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0) {
        error = "Could not compute SHA-256 for package file.";
        return false;
    }
    hash.reserve(digest.size() * 2);
    for (UCHAR byte : digest) {
        hash.push_back(Hex[byte >> 4]);
        hash.push_back(Hex[byte & 0x0f]);
    }
#else
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context || EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) {
        if (context) EVP_MD_CTX_free(context);
        error = "Could not initialize SHA-256 provider.";
        return false;
    }
    std::array<char, 65536> buffer{};
    bool failed = false;
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0 &&
            EVP_DigestUpdate(context, buffer.data(),
                static_cast<std::size_t>(count)) != 1) {
            failed = true;
            break;
        }
    }
    if (input.bad()) failed = true;
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0;
    if (!failed &&
        EVP_DigestFinal_ex(context, digest.data(), &digestLength) != 1)
        failed = true;
    EVP_MD_CTX_free(context);
    if (failed) {
        error = "Could not compute SHA-256 for package file.";
        return false;
    }
    hash.reserve(digestLength * 2);
    for (unsigned int i = 0; i < digestLength; ++i) {
        hash.push_back(Hex[digest[i] >> 4]);
        hash.push_back(Hex[digest[i] & 0x0f]);
    }
#endif
    return true;
}

}
