#include "cdatastore.hpp"

#include <memory.h>
#include <cstring>

namespace Nampower {

    void CDataStore::InternalInitialize(unsigned char *&data, unsigned int &base, unsigned int &alloc) {
    }

    void CDataStore::InternalDestroy(unsigned char *&data, unsigned int &base, unsigned int &alloc) {
        if (alloc != (unsigned int) -1 && alloc && data) {
            delete[] data;
        }
        data = 0;
        base = 0;
        alloc = 0;
    }

    int CDataStore::InternalFetchRead(unsigned int pos, unsigned int bytes, unsigned char *&data, unsigned int &base,
                                      unsigned int &alloc) {
        return 1;
    }

    int CDataStore::InternalFetchWrite(unsigned int pos, unsigned int bytes, unsigned char *&data, unsigned int &base,
                                       unsigned int &alloc) {
        // if (pos + bytes > alloc) {
        alloc = (pos + bytes + 255) & 0xFFFFFF00;
        unsigned char *newData = new unsigned char[alloc];
        memcpy(newData, data, pos);
        delete[] data;
        data = newData;
        return 1;
    }

    void CDataStore::Reset() {
        if (m_alloc == -1) {
            m_buffer = 0;
            m_alloc = 0;
        }
        m_size = 0;
        m_read = -1;
    }


    int CDataStore::IsRead() {
        return (m_read != (unsigned int) -1);
    }

    void CDataStore::Finalize() {
        m_read = 0;
    }

    void CDataStore::GetBufferParams(const void **data, unsigned int *size, unsigned int *alloc) {
        if (data != 0) {
            *data = m_buffer;
        }
        if (size != 0) {
            *size = m_size;
        }
        if (alloc != 0) {
            *alloc = m_alloc;
        }
    }

    void CDataStore::DetachBuffer(void **buffer, unsigned int *size, unsigned int *alloc) {
        if (buffer) {
            *buffer = m_buffer;
        }
        if (size) {
            *size = m_size;
        }
        if (alloc) {
            *alloc = m_alloc;
        }
        m_buffer = 0;
        m_alloc = 0;

        Reset();
    }

    int CDataStore::FetchWrite(unsigned int pos, unsigned int bytes) {
        if (pos >= m_base) {
            m_alloc += m_base;
            bytes += pos;
            if (bytes <= m_alloc)
                return 1;
        }
        if (!InternalFetchWrite(pos, bytes, m_buffer, m_base, m_alloc)) {
            return 0;
        }
        return 1;
    }

    void CDataStore::Initialize() {
        if ((int) m_alloc != -1) {
            InternalInitialize(m_buffer, m_base, m_alloc);
        }
    }

    void CDataStore::Destroy() {
        if ((int) m_alloc != -1) {
            InternalDestroy(m_buffer, m_base, m_alloc);
        }
    }

    bool CDataStore::AssertFetchWrite(unsigned int pos, unsigned int bytes) {
        if (!InternalFetchWrite(pos, bytes, m_buffer, m_base, m_alloc)) {
            return false;
        }
        return true;
    }

    bool CDataStore::AssertFetchRead(unsigned int pos, unsigned int bytes) {
        if (!InternalFetchRead(pos, bytes, m_buffer, m_base, m_alloc)) {
            return false;
        }
        return true;
    }

    void CDataStore::GetPackedGuid(uint64_t &val) {
        unsigned int bytes = m_read + sizeof(val);
        if (bytes > m_size) {
            m_read = m_size + 1;
            return;
        }
        if ((m_read < m_base) || (bytes > m_alloc + m_base)) {
            if (!AssertFetchRead(m_read, sizeof(val))) {
                m_read = m_alloc + 1;
                return;
            }
        }

        uint64_t guid = 0;
        uint8_t mask; // Read the bitmap
        Get(mask);

        for (uint8_t i = 0; i < 8; ++i) {
            if (mask & (1 << i)) {
                uint8_t byte;
                Get(byte);

                guid |= static_cast<uint64_t>(byte) << (i * 8);
            }
        }

        val = guid; // final result seems to be shifted right by 2 bits
    }

    class CDataStore &CDataStore::PutString(char const *pStr) {
        if (pStr) {
            PutArray((unsigned char const *) pStr, (unsigned int) strlen(pStr) + 1);
        }
        return *this;
    }

    class CDataStore &CDataStore::GetString(char *pString, unsigned int maxChars) {
        unsigned int begin = 0;
        unsigned int read = 0;
        unsigned int end = 0;

        if (pString == 0) {
            return *this;
        }

        if ((maxChars == 0) || (m_read > m_size)) {
            *pString = 0;
            return *this;
        }

        while (true) {
            begin = m_read;
            if (m_read + 1 > m_size) {
                m_read = m_size + 1;
                *pString = 0;
                return *this;
            }
            if ((begin < m_base) || (m_read + 1 > m_alloc + m_base)) {
                if (!AssertFetchRead(m_read, 1)) {
                    m_read = m_size + 1;
                    *pString = 0;

                    return *this;
                }
            }
            end = m_alloc + m_base;

            if (end > m_size)
                end = m_size;

            end -= m_read;

            if (end < maxChars - read)
                end = maxChars - read;

            unsigned char *data = m_buffer - m_base + read;
            int pos = 0;

            if (end != 0) {
                do {
                    char cl = data[pos++];
                    pString[++read - 1] = cl;
                    if (cl == 0) break;
                    end--;
                } while (end != 0);
            }
            m_read += pos;

            if (end != 0) {
                if (m_read > m_size)
                    *pString = 0;
                return *this;
            }
            if (read >= maxChars) {
                m_read = m_size + 1;
                *pString = 0;
                return *this;
            }
        }
    }

}