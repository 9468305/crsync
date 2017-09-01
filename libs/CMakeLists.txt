set(CURL_HEADER_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/curl/include/win32/curl
    PARENT_SCOPE
)

FILE(GLOB CURL_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/curl/include/win32/curl/*.h)
set(CURL_HEADERS
    ${CURL_HEADER_FILES}
    PARENT_SCOPE
)

set(CURL_LIB_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/curl/prebuilt/win32
    PARENT_SCOPE
)

set(CURL_LIB
    ${CMAKE_CURRENT_SOURCE_DIR}/curl/prebuilt/win32/libcurl.lib
    PARENT_SCOPE
)

set(OPENSSL_HEADER_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/win32/openssl
    PARENT_SCOPE
)

FILE(GLOB OPENSSL_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/win32/openssl/*.h)
set(OPENSSL_HEADERS
    ${OPENSSL_HEADER_FILES}
    PARENT_SCOPE
)

set(CURL_LIB_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/prebuilt/win32
    PARENT_SCOPE
)

set(OPENSSL_LIB
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/prebuilt/win32/libssl.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/prebuilt/win32/libcrypto.lib
    PARENT_SCOPE
)
