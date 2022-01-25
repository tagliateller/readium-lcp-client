cmake_minimum_required(VERSION 3.16)

include_directories(${CMAKE_SOURCE_DIR}/src/third-parties)
include_directories(${CMAKE_SOURCE_DIR}/src/lcp-client-lib)

#find_package(CRYPTOPP REQUIRED)

set(libSrcs 
    Acquisition.cpp 
    AesCbcSymmetricAlgorithm.cpp 
    AesGcmSymmetricAlgorithm.cpp
    AlgorithmNames.cpp
    Certificate.cpp
    CertificateExtension.cpp
    CertificateRevocationList.cpp
    CMakeLists.txt
    CrlDistributionPoints.cpp
    CrlUpdater.cpp
    CryptoLcpNode.cpp
    CryptoppCryptoProvider.cpp
    CryptoppUtils.cpp
    DateTime.cpp
    EcdsaSha256SignatureAlgorithm.cpp
    EncryptionProfileNames.cpp
    EncryptionProfilesManager.cpp
    JsonCanonicalizer.cpp
    JsonValueReader.cpp
    Lcp1dot0EncryptionProfile.cpp
    LcpService.cpp
    LcpServiceCreator.cpp
    LcpUtils.cpp
    LinksLcpNode.cpp
    RightsLcpNode.cpp
    RightsService.cpp
    RootLcpNode.cpp
    RsaSha256SignatureAlgorithm.cpp
    Sha256HashAlgorithm.cpp
    SymmetricAlgorithmEncryptedStream.cpp
    ThreadTimer.cpp
    UserLcpNode.cpp
    )

add_library(lcplib SHARED ${libSrcs})

install(TARGETS lcplib DESTINATION lib)
