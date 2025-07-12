#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "endian_convert.h"

namespace sylar
{
    static Logger::ptr g_logger = LOG_ROOT();

    template <class T>
    static T CreateMask(uint32_t bits)
    {
        return (1 << (sizeof(T) * 8 - bits)) - 1;
    }

    ///////////////////// Address ///////////////////////
    // 52 - 10:44
    bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                         int family, int type, int protocol)
    {
        addrinfo hints, *results, *next;
        hints.ai_flags
    }

    int Address::getFamily() const
    {
        return getAddr()->sa_family;
    }

    std::string Address::toString()
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen)
    {
        if (addr == nullptr)
        {
            return nullptr;
        }

        Address::ptr result;
        switch (addr->sa_family)
        {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
        }
        return result;
    }

    int Address::getFamily() const
    {
        return getAddr()->sa_family;
    }

    std::string Address::toString()
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address &rhs) const
    {
        socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
        int result = memcmp(getAddr(), rhs.getAddr(), minlen);
        if (result < 0)
        {
            return true;
        }
        else if (result > 0)
        {
            return false;
        }
        else if (getAddrLen() < rhs.getAddrLen())
        {
            return true;
        }
        return false;
    }
    bool Address::operator==(const Address &rhs) const
    {
        return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }
    bool Address::operator!=(const Address &rhs) const
    {
        return !(*this == rhs);
    }

    ///////////////////////// IPAddress /////////////////////////////

    IPAddress::ptr IPAddress::Create(const char *address, uint32_t port)
    {
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(addrinfo));

        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;
        int error = getaddrinfo(address, NULL, &hints, &results);
        if (error)
        {
            LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
                                << ", " << port << ") error=" << error
                                << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try
        {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if (result)
            {
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
        }
        catch (...)
        {
            freeaddrinfo(results);
            return nullptr;
        }
    }

    ///////////////////////// IPv4 /////////////////////////////

    IPv4Address::ptr IPv4Address::Create(const char *address, uint32_t port)
    {
        IPv4Address::ptr ret(new IPv4Address);
        ret->m_addr.sin_port = toBigEndian(port);
        int result = inet_pton(AF_INET, address, &ret->m_addr.sin_addr);
        if (result <= 0)
        {
            LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                                << port << ") rt=" << result << " errno=" << errno
                                << " errstr=" << strerror(errno);
            return nullptr;
        }
        return ret;
    }

    IPv4Address::IPv4Address(const sockaddr_in &address)
        : m_addr(address)
    {
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = toBigEndian(address);
        m_addr.sin_port = toBigEndian(port);
    }

    const sockaddr *IPv4Address::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    std::ostream &IPv4Address::insert(std::ostream &os) const
    {
        uint32_t addr = toBigEndian(m_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
           << ((addr >> 16) & 0xff) << "."
           << ((addr >> 8) & 0xff) << "."
           << (addr & 0xff);
        os << ":" << toBigEndian(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
    {
        if (prefix_len > 32)
        {
            return nullptr;
        }

        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr |= toBigEndian(CreateMask<uint32_t>(prefix_len));

        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len)
    {
        if (prefix_len > 32)
        {
            return nullptr;
        }

        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr &= ~toBigEndian(CreateMask<uint32_t>(prefix_len));

        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
    {
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~toBigEndian(CreateMask<uint32_t>(prefix_len));

        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    uint32_t IPv4Address::getPort() const
    {
        return toBigEndian(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint32_t v)
    {
        m_addr.sin_port = toBigEndian(v);
    }

    ///////////////////////// IPv6 /////////////////////////////

    IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port)
    {
        IPv6Address::ptr rt(new IPv6Address);
        rt->m_addr.sin6_port = toBigEndian(port);
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
        if (result <= 0)
        {
            LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                                << port << ") rt=" << result << " errno=" << errno
                                << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv6Address::IPv6Address()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6 &address)
        : m_addr(address)
    {
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = toBigEndian(port);
        memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
    }

    const sockaddr *IPv6Address::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    std::ostream &IPv6Address::insert(std::ostream &os) const
    {
        os << "[";
        uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
        bool used_zeros = false;
        for (size_t i = 0; i < 8; i++)
        {
            if (addr[i] == 0 && !used_zeros)
            {
                continue;
            }
            if (i && addr[i - 1] == 0 && !used_zeros)
            {
                os << ":";
                used_zeros = true;
            }
            if (i)
            {
                os << ":";
            }
            os << std::hex << (int)toBigEndian(addr[i]) << std::dec;
        }

        if (!used_zeros && addr[7] == 0)
        {
            os << "::";
        }

        os << "]:" << toBigEndian(m_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
    {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
        for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
        {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len)
    {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &= ~CreateMask<uint8_t>(prefix_len % 8);
        for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
        {
            baddr.sin6_addr.s6_addr[i] &= 0x00;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
    {
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len / 8] =
            ~CreateMask<uint8_t>(prefix_len % 8);

        for (uint32_t i = 0; i < prefix_len / 8; ++i)
        {
            subnet.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint32_t IPv6Address::getPort() const
    {
        return toBigEndian(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint32_t v)
    {
        m_addr.sin6_port = toBigEndian(v);
    }

    ///////////////////////// UnixAddress ////////////////////

    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

    UnixAddress::UnixAddress()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_addrLen = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string &path)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_addrLen = path.size() + 1;

        if (!path.empty() && path[0] == '\0')
        {
            --m_addrLen;
        }

        if (m_addrLen > sizeof(m_addr.sun_path))
        {
            throw std::logic_error("path too long");
        }

        memcpy(m_addr.sun_path, path.c_str(), m_addrLen);
        m_addrLen += offsetof(sockaddr_un, sun_path);
    }

    const sockaddr *UnixAddress::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const
    {
        return m_addrLen;
    }
    std::ostream &UnixAddress::insert(std::ostream &os) const
    {
        int pos = offsetof(sockaddr_un, sun_path);
        if (m_addrLen > pos && m_addr.sun_path[0] == '\0')
        {
            return os << "\\0" << std::string(m_addr.sun_path + 1, m_addrLen - pos - 1);
        }
        return os << m_addr.sun_path;
    }

    ///////////////////////// UnknownAddress ////////////////////
    UnknownAddress::UnknownAddress(int family)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }

    UnknownAddress::UnknownAddress(const sockaddr &addr)
        : m_addr(addr)
    {
    }

    const sockaddr *UnknownAddress::getAddr() const
    {
        return &m_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    std::ostream &UnknownAddress::insert(std::ostream &os) const
    {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

}