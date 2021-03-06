extern "C" {
#include "./api.h"
#include "./randombytes.h"
}

#include <nan.h>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

using namespace v8;
using namespace Nan;

class Dilithium
{
  public:
    Dilithium();

    Dilithium(const std::vector<uint8_t> &pk, const std::vector<uint8_t> &sk);

    virtual ~Dilithium() = default;

    std::vector<uint8_t> sign(const std::vector<uint8_t> &message);

    std::vector<uint8_t> getSK() { return _sk; }

    std::vector<uint8_t> getPK() { return _pk; }

    unsigned int getSecretKeySize() { return CRYPTO_SECRETKEYBYTES; }

    unsigned int getPublicKeySize() { return CRYPTO_PUBLICKEYBYTES; }

    static bool sign_open(std::vector<uint8_t> &message_output,
                          const std::vector<uint8_t> &message_signed,
                          const std::vector<uint8_t> &pk);

    static std::vector<uint8_t> extract_message(std::vector<uint8_t> &message_output);

    static std::vector<uint8_t> extract_signature(std::vector<uint8_t> &message_output);

  protected:
    std::vector<uint8_t> _pk;
    std::vector<uint8_t> _sk;
};

Dilithium::Dilithium()
{
    // TODO: Initialize keys randomly (seed?)
    // printf("there have no data\n");
    _pk.resize(CRYPTO_PUBLICKEYBYTES);
    _sk.resize(CRYPTO_SECRETKEYBYTES);
    crypto_sign_keypair(_pk.data(), _sk.data());
}

Dilithium::Dilithium(const std::vector<uint8_t> &pk, const std::vector<uint8_t> &sk) : _pk(pk),
                                                                                       _sk(sk)
{
    printf("data is here\n");
    size_t _pkLen = _pk.size();
    size_t _skLen = _sk.size();
    size_t pkLen = pk.size();
    size_t skLen = sk.size();
    // printf("1:%d\n", _pkLen);
    // printf("2:%d\n", _skLen);
    // printf("3:%d\n"), pkLen;
    // printf("4:%d\n", skLen);
    // printf("CRYPTO_SECRETKEYBYTES:%d\n", CRYPTO_SECRETKEYBYTES);
    // printf("CRYPTO_PUBLICKEYBYTES%d\n", CRYPTO_PUBLICKEYBYTES);

    // printf("hhhhh");
    // TODO: Verify sizes - CRYPTO_SECRETKEYBYTES / CRYPTO_PUBLICKEYBYTES
}

std::vector<uint8_t> Dilithium::sign(const std::vector<uint8_t> &message)
{
    unsigned long long message_signed_size_dummy;

    std::vector<unsigned char> message_signed(message.size() + CRYPTO_BYTES);

    crypto_sign(message_signed.data(),
                &message_signed_size_dummy,
                message.data(),
                message.size(),
                _sk.data());

    return message_signed;

    // TODO: Leon, return only signature?
    //    return std::vector<unsigned char>(message_signed.begin()+message.size(),
    //                                      message_signed.end());
}

bool Dilithium::sign_open(std::vector<uint8_t> &message_output,
                          const std::vector<uint8_t> &message_signed,
                          const std::vector<uint8_t> &pk)
{
    auto message_size = message_signed.size();
    message_output.resize(message_size);

    unsigned long long message_output_dummy;
    auto ret = crypto_sign_open(message_output.data(),
                                &message_output_dummy,
                                message_signed.data(),
                                message_signed.size(),
                                pk.data());
    ;
    // TODO Leon: message_out has size()+CRYPTO_BYTES. Should we return just the message?
    // if ret == 0, success and this return value = 1
    std::cout<<ret<<std::endl;
    return ret == 0;
}

std::vector<uint8_t> Dilithium::extract_message(std::vector<uint8_t> &message_output)
{
    return std::vector<uint8_t>(message_output.begin(), message_output.end() - CRYPTO_BYTES);
}

std::vector<uint8_t> Dilithium::extract_signature(std::vector<uint8_t> &message_output)
{
    return std::vector<uint8_t>(message_output.end() - CRYPTO_BYTES, message_output.end());
}

NAN_METHOD(Keygen)
{
    Dilithium dilithium;
    auto pk = dilithium.getPK();
    auto sk = dilithium.getSK();
    size_t pkLen = dilithium.getPublicKeySize();
    size_t skLen = dilithium.getSecretKeySize();

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();

    Nan::Set(obj, Nan::New("pubKey").ToLocalChecked(), Encode(pk.data(), pkLen, BUFFER));
    Nan::Set(obj, Nan::New("priKey").ToLocalChecked(), Encode(sk.data(), skLen, BUFFER));
    info.GetReturnValue().Set(obj);

    //  info.GetReturnValue().Set(Encode("hello", 5, HEX));
}

NAN_METHOD(Sign)
{
    if (info.Length() < 1)
    {
        return;
    }
    std::vector<uint8_t> pk;
    std::vector<uint8_t> sk;
    std::vector<unsigned char> message;
    pk.reserve(CRYPTO_PUBLICKEYBYTES);
    sk.reserve(CRYPTO_SECRETKEYBYTES);
    unsigned char *skBuf = (unsigned char *)node::Buffer::Data(info[0]);
    unsigned char *msgBuf = (unsigned char *)node::Buffer::Data(info[1]);
    size_t len = node::Buffer::Length(info[1]);
    message.reserve(len);
    sk.insert(sk.begin(), &skBuf[0], &skBuf[CRYPTO_SECRETKEYBYTES - 1]);

    message.insert(message.begin(), &msgBuf[0], &msgBuf[len - 1]);
    Dilithium dilithium(pk, sk);
    auto message_signed = dilithium.sign(message);

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("messageSigned").ToLocalChecked(), Encode(message_signed.data(), message_signed.size(), BUFFER));
    info.GetReturnValue().Set(obj);
}

NAN_METHOD(Open)
{
    std::vector<uint8_t> pk;
    std::vector<uint8_t> sk;
    std::vector<unsigned char> sm;

    pk.reserve(CRYPTO_PUBLICKEYBYTES);
    sk.reserve(CRYPTO_SECRETKEYBYTES);
    size_t len = node::Buffer::Length(info[1]);
    sm.reserve(len);
    
    unsigned char *pkBuf = (unsigned char *)node::Buffer::Data(info[0]);
    unsigned char *smBuf = (unsigned char *)node::Buffer::Data(info[1]);

    
    pk.insert(pk.begin(), &pkBuf[0], &pkBuf[CRYPTO_PUBLICKEYBYTES - 1]);
    sm.insert(sm.begin(), &smBuf[0], &smBuf[len - 1]);
    Dilithium dilithium(pk, sk);
    std::vector<unsigned char> message_out(sm.size());
    auto p = dilithium.getPK();
    auto ret = Dilithium::sign_open(message_out, sm, p);
    std::cout<<ret<<std::endl;
}
NAN_MODULE_INIT(Init)
{
    Nan::Set(target, Nan::New("sign").ToLocalChecked(),
             Nan::GetFunction(Nan::New<FunctionTemplate>(Sign)).ToLocalChecked());
    Nan::Set(target, Nan::New("keygen").ToLocalChecked(),
             Nan::GetFunction(Nan::New<FunctionTemplate>(Keygen)).ToLocalChecked());
    Nan::Set(target, Nan::New("open").ToLocalChecked(),
             Nan::GetFunction(Nan::New<FunctionTemplate>(Open)).ToLocalChecked());
}

NODE_MODULE(myaddon, Init)
