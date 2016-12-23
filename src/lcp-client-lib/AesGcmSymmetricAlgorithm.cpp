// Copyright (c) 2016 Mantano
// Licensed to the Readium Foundation under one or more contributor license agreements.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation and/or
//    other materials provided with the distribution.
// 3. Neither the name of the organization nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "AesGcmSymmetricAlgorithm.h"
#include "AlgorithmNames.h"
#include "CryptoppUtils.h"
#include "IDecryptionContext.h"
#include "public/StreamInterfaces.h"

namespace lcp
{
    // https://www.cryptopp.com/wiki/GCM_Mode
    // https://www.cryptopp.com/wiki/AuthenticatedDecryptionFilter
    AesGcmSymmetricAlgorithm::AesGcmSymmetricAlgorithm(
        const KeyType & key,
        KeySize keySize
        )
        : m_key(key)
        , m_keySize(keySize)
    {
        KeyType emptyIv(CryptoPP::AES::BLOCKSIZE);
        m_decryptor.SetKeyWithIV(&key.at(0), key.size(), &emptyIv.at(0));
    }

    std::string AesGcmSymmetricAlgorithm::Name() const
    {
        switch (m_keySize)
        {
        case lcp::AesGcmSymmetricAlgorithm::Key128:
            throw std::logic_error("not implemented");
        case lcp::AesGcmSymmetricAlgorithm::Key192:
            throw std::logic_error("not implemented");
        case lcp::AesGcmSymmetricAlgorithm::Key256:
            return AlgorithmNames::AesGcm256Id;
        default:
            throw std::logic_error("not implemented");
        }
    }

    std::string AesGcmSymmetricAlgorithm::Decrypt(
        const std::string & encryptedDataBase64
        )
    {
        CryptoPP::SecByteBlock rawData;
        CryptoppUtils::Base64ToSecBlock(encryptedDataBase64, rawData);

        const unsigned char * cipherData = rawData.data();
        size_t cipherSize = rawData.size();

        KeyType iv = this->BuildIV(rawData.data(), rawData.size(), &cipherData, &cipherSize);
        m_decryptor.SetKeyWithIV(&m_key.at(0), m_key.size(), &iv.at(0));


        std::string decryptedDataStr;

        CryptoPP::ArraySource source( // really, a StringSource
            cipherData, //const byte *string
            cipherSize, //size_t length
            true, //bool pumpAll
            new CryptoPP::AuthenticatedDecryptionFilter( //BufferedTransformation *attachment = NULL
                    m_decryptor, //AuthenticatedSymmetricCipher &c
                    new CryptoPP::StringSink(decryptedDataStr), //BufferedTransformation *attachment NULL
                    //new CryptoPP::ArraySink(...) // alternatively, could also construct std::string manually via ArraySink
                    CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_END | CryptoPP::AuthenticatedDecryptionFilter::THROW_EXCEPTION, //AuthenticatedDecryptionFilter::DEFAULT_FLAGS, //word32 flags DEFAULT_FLAGS
                    16, // int truncatedDigestSize -1
                    CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_PADDING // BlockPaddingScheme padding DEFAULT_PADDING
                )
            );

        return decryptedDataStr;
    }

    size_t AesGcmSymmetricAlgorithm::Decrypt(
        const unsigned char * data,
        size_t dataLength,
        unsigned char * decryptedData,
        size_t decryptedDataLength
        )
    {
        const unsigned char * cipherData = data;
        size_t cipherSize = dataLength;

        KeyType iv = this->BuildIV(data, dataLength, &cipherData, &cipherSize);
        m_decryptor.SetKeyWithIV(&m_key.at(0), m_key.size(), &iv.at(0));

        return this->InnerDecrypt(
                cipherData,
                cipherSize,
            decryptedData,
            decryptedDataLength,
            true
            );
    }

    size_t AesGcmSymmetricAlgorithm::PlainTextSize(IReadableStream * stream)
    {
        return static_cast<size_t>(stream->Size())
            - 12 // Nonce-IV (prefix)
            - 16; // Authentication tag / overhead for integrity verification of whole cipher (suffix)
    }

    void AesGcmSymmetricAlgorithm::Decrypt(
        IDecryptionContext * context,
        IReadableStream * stream,
        unsigned char * decryptedData,
        size_t decryptedDataLength
        )
    {
        DecRangeInfo rangeInfo = context->GetDecryptionRange();
        size_t plainTextSize = this->PlainTextSize(stream);
        if (rangeInfo.position + rangeInfo.length > plainTextSize)
        {
            throw std::out_of_range("params to decrypt out of range");
        }

        bool full = false;
        if (rangeInfo.position == 0 && rangeInfo.length == plainTextSize && plainTextSize == decryptedDataLength) {
            full = true;
        }

        if (full) {
            stream->SetReadPosition(0);
            size_t streamSize = stream->Size();

            std::vector<unsigned char> inBuffer(streamSize);
            std::vector<unsigned char> outBuffer(streamSize);

            stream->Read(&inBuffer.at(0), streamSize);

            const unsigned char * cipherData = &inBuffer.at(0);
            size_t cipherSize = streamSize;

            KeyType iv = this->BuildIV(&inBuffer.at(0), inBuffer.size(), &cipherData, &cipherSize);
            m_decryptor.SetKeyWithIV(&m_key.at(0), m_key.size(), &iv.at(0));

            size_t outSize = this->InnerDecrypt(
                    cipherData,
                    cipherSize,
                    &outBuffer.at(0),
                    streamSize,
                    full
            );

            if (outSize < rangeInfo.length)
            {
                throw std::out_of_range("range length is out of range");
            }

            outBuffer.resize(outSize);
            memcpy_s(decryptedData, decryptedDataLength, &outBuffer.at(0), rangeInfo.length);
        } else {

            size_t ivSize = 12;
            std::vector<unsigned char> ivBuffer(ivSize);

            stream->SetReadPosition(0);
            size_t streamSize = stream->Size();
            stream->Read(&ivBuffer.at(0), ivBuffer.size());

            KeyType iv;
            iv.resize(ivBuffer.size());
            iv.assign(&ivBuffer.at(0), &ivBuffer.at(0) + ivBuffer.size());

            m_decryptor.SetKeyWithIV(&m_key.at(0), m_key.size(), &iv.at(0));





            size_t rangePos_nWholeBlocksToSkip = rangeInfo.position / CryptoPP::AES::BLOCKSIZE;

            size_t rangePos_nBytesAfterWholeBlocksToSkip = rangeInfo.position % CryptoPP::AES::BLOCKSIZE;

            size_t rangeLength_nBytesInFirstBlock =
                    (rangePos_nBytesAfterWholeBlocksToSkip == 0) ?
                    0 :
                    (CryptoPP::AES::BLOCKSIZE - rangePos_nBytesAfterWholeBlocksToSkip);

            size_t rangeLength_nWholeBlocks = 1;

            if (rangeInfo.length > rangeLength_nBytesInFirstBlock)
            {
                size_t rangeLength_nBytesAfterFirstBlock = rangeInfo.length - rangeLength_nBytesInFirstBlock;

                rangeLength_nWholeBlocks += rangeLength_nBytesAfterFirstBlock / CryptoPP::AES::BLOCKSIZE;

                size_t rangeLength_nBytesInLastBlock = rangeLength_nBytesAfterFirstBlock % CryptoPP::AES::BLOCKSIZE;
                if (rangeLength_nBytesInLastBlock > 0)
                {
                    rangeLength_nWholeBlocks++;
                }
            }


            size_t readLength = rangeLength_nWholeBlocks * CryptoPP::AES::BLOCKSIZE;




            // adjusted for 12 bytes init. vector prefix
            size_t readPosition = ivSize + (rangeInfo.position - rangePos_nBytesAfterWholeBlocksToSkip);

            size_t endPos = readPosition + readLength;
            if (endPos > streamSize)
            {
                throw std::out_of_range("encrypted stream is out of range");
            }

            std::vector<unsigned char> inBuffer(readLength);
            std::vector<unsigned char> outBuffer(readLength);

            stream->SetReadPosition(readPosition);

            stream->Read(&inBuffer.at(0), readLength);

            size_t outSize = this->InnerDecrypt(
                    &inBuffer.at(0),
                    readLength,
                    &outBuffer.at(0),
                    readLength,
                    full
            );

            if (outSize < rangeInfo.length)
            {
                throw std::out_of_range("range length is out of range");
            }

            outBuffer.resize(outSize);
            memcpy_s(decryptedData, decryptedDataLength, &outBuffer.at(rangePos_nBytesAfterWholeBlocksToSkip), rangeInfo.length);
        }
    }

    size_t AesGcmSymmetricAlgorithm::InnerDecrypt(
        const unsigned char * cipherData,
        size_t cipherSize,
        unsigned char * decryptedData,
        size_t decryptedDataLength,
        bool verifyIntegrityAuthenticatedEncryption
        )
    {
        CryptoPP::AuthenticatedDecryptionFilter filter( //BufferedTransformation*
                m_decryptor, //AuthenticatedSymmetricCipher &c
                NULL, //BufferedTransformation *attachment NULL
                    verifyIntegrityAuthenticatedEncryption ?
                    (CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_END | CryptoPP::AuthenticatedDecryptionFilter::THROW_EXCEPTION) :
                    CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_END,
                            //AuthenticatedDecryptionFilter::DEFAULT_FLAGS, //word32 flags DEFAULT_FLAGS
                16, // int truncatedDigestSize -1
                CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_PADDING // BlockPaddingScheme padding DEFAULT_PADDING
        );
        filter.Put(cipherData, cipherSize);
//https://www.cryptopp.com/docs/ref/class_buffered_transformation.html#a0c25529ded99db20ad35ccef3f7234e6
//        filter.Skip()

        filter.MessageEnd();
        size_t resultSize = static_cast<size_t>(filter.MaxRetrievable());

        if (decryptedDataLength < resultSize)
        {
            throw std::invalid_argument("decrypted data buffer is too small");
        }

        filter.Get(decryptedData, resultSize);
        return resultSize;
    }

    KeyType AesGcmSymmetricAlgorithm::BuildIV(
        const unsigned char * data,
        size_t dataLength,
        const unsigned char ** cipherData,
        size_t * cipherSize
        )
    {
        // Length of block equals to length of IV
        size_t blockAndIvSize = 12; //CryptoPP::AES::BLOCKSIZE; 16
        KeyType iv;
        iv.resize(blockAndIvSize);

        if (dataLength < blockAndIvSize + CryptoPP::AES::BLOCKSIZE) //blockAndIvSize
        {
            throw std::invalid_argument("input data to decrypt is too small");
        }

        iv.assign(data, data + blockAndIvSize);
        *cipherData = data + blockAndIvSize;
        *cipherSize = dataLength - blockAndIvSize;

        return iv;
    }
}
