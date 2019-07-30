#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "envoy/event/dispatcher.h"
#include "envoy/network/address.h"
#include "envoy/network/dns.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/stack_array.h"
#include "common/common/utility.h"
#include "common/event/dispatcher_impl.h"
#include "common/network/address_impl.h"
#include "common/network/dns_impl.h"
#include "common/network/filter_impl.h"
#include "common/network/listen_socket_impl.h"
#include "common/network/utility.h"

#include "test/mocks/network/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "ares.h"
#include "ares_dns.h"
#include "gtest/gtest.h"

using testing::_;
using testing::InSequence;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

namespace Envoy {
namespace Network {
namespace {

// List of IP address (in human readable format).
using IpList = std::list<std::string>;
// Map from hostname to IpList.
using HostMap = std::unordered_map<std::string, IpList>;
// Map from hostname to CNAME
using CNameMap = std::unordered_map<std::string, std::string>;
// Represents a single TestDnsServer query state and lifecycle. This implements
// just enough of RFC 1035 to handle queries we generate in the tests below.
enum record_type { A, AAAA };

class TestDnsServerQuery {
public:
  TestDnsServerQuery(ConnectionPtr connection, const HostMap& hosts_A, const HostMap& hosts_AAAA,
                     const CNameMap& cnames, const std::chrono::seconds& record_ttl)
      : connection_(std::move(connection)), hosts_A_(hosts_A), hosts_AAAA_(hosts_AAAA),
        cnames_(cnames), record_ttl_(record_ttl) {
    connection_->addReadFilter(Network::ReadFilterSharedPtr{new ReadFilter(*this)});
  }

  ~TestDnsServerQuery() { connection_->close(ConnectionCloseType::NoFlush); }

  // Utility to encode a dns string in the rfc format. Example: \004some\004good\006domain
  // RFC link: https://www.ietf.org/rfc/rfc1035.txt
  static std::string encodeDnsName(const std::string& input) {
    auto name_split = StringUtil::splitToken(input, ".");
    std::string res;
    for (const auto& it : name_split) {
      res += static_cast<char>(it.size());
      const std::string part{it};
      res.append(part);
    }
    return res;
  }

private:
  struct ReadFilter : public Network::ReadFilterBaseImpl {
    ReadFilter(TestDnsServerQuery& parent) : parent_(parent) {}

    // Network::ReadFilter
    Network::FilterStatus onData(Buffer::Instance& data, bool) override {
      onDataInternal(data);
      return Network::FilterStatus::StopIteration;
    }

    // Hack: void returning variation of onData to allow gtest assertions.
    void onDataInternal(Buffer::Instance& data) {
      buffer_.add(data);
      while (true) {
        if (size_ == 0) {
          uint16_t size_n;
          if (buffer_.length() < sizeof(size_n)) {
            // If we don't have enough bytes to determine size, wait until we do.
            return;
          }
          void* mem = buffer_.linearize(sizeof(size_n));
          std::memcpy(reinterpret_cast<void*>(&size_n), mem, sizeof(size_n));
          buffer_.drain(sizeof(size_n));
          size_ = ntohs(size_n);
        }

        if (buffer_.length() < size_) {
          // If we don't have enough bytes to read the complete query, wait until
          // we do.
          return;
        }

        // Expect requests to be small, so stack allocation is fine for test code.
        unsigned char* request = static_cast<unsigned char*>(buffer_.linearize(size_));
        // Only expecting a single question.
        ASSERT_EQ(1, DNS_HEADER_QDCOUNT(request));
        // Decode the question and perform lookup.
        const unsigned char* question = request + HFIXEDSZ;
        // The number of bytes the encoded question name takes up in the request.
        // Useful in the response when generating resource records containing the
        // name.
        long name_len;
        // Get host name from query and use the name to lookup a record
        // in a host map. If the query type is of type A, then perform the lookup in
        // the hosts_A_ host map. If the query type is of type AAAA, then perform the
        // lookup in the hosts_AAAA_ host map.
        char* name;
        ASSERT_EQ(ARES_SUCCESS, ares_expand_name(question, request, size_, &name, &name_len));
        const std::list<std::string>* ips = nullptr;
        // We only expect resources of type A or AAAA.
        const int q_type = DNS_QUESTION_TYPE(question + name_len);
        std::string cname;
        // check if we have a cname. If so, we will need to send a response element with the cname
        // and lookup the ips of the cname and send back those ips (if any) too
        auto cit = parent_.cnames_.find(name);
        if (cit != parent_.cnames_.end()) {
          cname = cit->second;
        }
        const char* hostLookup = name;
        const unsigned char* ip_question = question;
        long ip_name_len = name_len;
        std::string encodedCname;
        if (!cname.empty()) {
          ASSERT_TRUE(cname.size() <= 253);
          hostLookup = cname.c_str();
          encodedCname = TestDnsServerQuery::encodeDnsName(cname);
          ip_question = reinterpret_cast<const unsigned char*>(encodedCname.c_str());
          ip_name_len =
              encodedCname.size() + 1; //+1 as we need to include the final null terminator
        }
        ASSERT_TRUE(q_type == T_A || q_type == T_AAAA);
        if (q_type == T_A) {
          auto it = parent_.hosts_A_.find(hostLookup);
          if (it != parent_.hosts_A_.end()) {
            ips = &it->second;
          }
        } else {
          auto it = parent_.hosts_AAAA_.find(hostLookup);
          if (it != parent_.hosts_AAAA_.end()) {
            ips = &it->second;
          }
        }
        ares_free_string(name);

        int answer_size = ips != nullptr ? ips->size() : 0;
        answer_size += !encodedCname.empty() ? 1 : 0;

        // The response begins with the initial part of the request
        // (including the question section).
        const size_t response_base_len = HFIXEDSZ + name_len + QFIXEDSZ;
        STACK_ARRAY(response_buf, unsigned char, response_base_len);
        unsigned char* response_base = response_buf.begin();
        memcpy(response_base, request, response_base_len);
        DNS_HEADER_SET_QR(response_base, 1);
        DNS_HEADER_SET_AA(response_base, 0);
        DNS_HEADER_SET_RCODE(response_base, answer_size > 0 ? NOERROR : NXDOMAIN);
        DNS_HEADER_SET_ANCOUNT(response_base, answer_size);
        DNS_HEADER_SET_NSCOUNT(response_base, 0);
        DNS_HEADER_SET_ARCOUNT(response_base, 0);
        // Total response size will be computed according to cname response size + ip response sizes
        size_t response_ip_rest_len;
        if (q_type == T_A) {
          response_ip_rest_len =
              ips != nullptr ? ips->size() * (ip_name_len + RRFIXEDSZ + sizeof(in_addr)) : 0;
        } else {
          response_ip_rest_len =
              ips != nullptr ? ips->size() * (ip_name_len + RRFIXEDSZ + sizeof(in6_addr)) : 0;
        }
        size_t response_cname_len =
            !encodedCname.empty() ? name_len + RRFIXEDSZ + encodedCname.size() + 1 : 0;
        const uint16_t response_size_n =
            htons(response_base_len + response_ip_rest_len + response_cname_len);
        Buffer::OwnedImpl write_buffer;
        // Write response header
        write_buffer.add(&response_size_n, sizeof(response_size_n));
        write_buffer.add(response_base, response_base_len);

        // if we have a cname, create a resource record
        if (!encodedCname.empty()) {
          unsigned char cname_rr_fixed[RRFIXEDSZ];
          DNS_RR_SET_TYPE(cname_rr_fixed, T_CNAME);
          DNS_RR_SET_LEN(cname_rr_fixed, encodedCname.size() + 1);
          DNS_RR_SET_CLASS(cname_rr_fixed, C_IN);
          DNS_RR_SET_TTL(cname_rr_fixed, parent_.record_ttl_.count());
          write_buffer.add(question, name_len);
          write_buffer.add(cname_rr_fixed, RRFIXEDSZ);
          write_buffer.add(encodedCname.c_str(), encodedCname.size() + 1);
        }

        // Create a resource record for each IP found in the host map.
        unsigned char response_rr_fixed[RRFIXEDSZ];
        if (q_type == T_A) {
          DNS_RR_SET_TYPE(response_rr_fixed, T_A);
          DNS_RR_SET_LEN(response_rr_fixed, sizeof(in_addr));
        } else {
          DNS_RR_SET_TYPE(response_rr_fixed, T_AAAA);
          DNS_RR_SET_LEN(response_rr_fixed, sizeof(in6_addr));
        }
        DNS_RR_SET_CLASS(response_rr_fixed, C_IN);
        DNS_RR_SET_TTL(response_rr_fixed, parent_.record_ttl_.count());
        if (ips != nullptr) {
          for (const auto& it : *ips) {
            write_buffer.add(ip_question, ip_name_len);
            write_buffer.add(response_rr_fixed, RRFIXEDSZ);
            if (q_type == T_A) {
              in_addr addr;
              ASSERT_EQ(1, inet_pton(AF_INET, it.c_str(), &addr));
              write_buffer.add(&addr, sizeof(addr));
            } else {
              in6_addr addr;
              ASSERT_EQ(1, inet_pton(AF_INET6, it.c_str(), &addr));
              write_buffer.add(&addr, sizeof(addr));
            }
          }
        }
        parent_.connection_->write(write_buffer, false);

        // Reset query state, time for the next one.
        buffer_.drain(size_);
        size_ = 0;
      }
    }

    TestDnsServerQuery& parent_;
    // The expected size of the current DNS query to read. If zero, indicates that
    // no DNS query is in progress and that a 2 byte size is expected from the
    // client to indicate the next DNS query size.
    uint16_t size_ = 0;
    Buffer::OwnedImpl buffer_;
  };

private:
  ConnectionPtr connection_;
  const HostMap& hosts_A_;
  const HostMap& hosts_AAAA_;
  const CNameMap& cnames_;
  const std::chrono::seconds& record_ttl_;
};

class TestDnsServer : public ListenerCallbacks {
public:
  TestDnsServer(Event::Dispatcher& dispatcher) : dispatcher_(dispatcher), record_ttl_(0) {}

  void onAccept(ConnectionSocketPtr&& socket, bool) override {
    Network::ConnectionPtr new_connection = dispatcher_.createServerConnection(
        std::move(socket), Network::Test::createRawBufferSocket());
    onNewConnection(std::move(new_connection));
  }

  void onNewConnection(ConnectionPtr&& new_connection) override {
    TestDnsServerQuery* query = new TestDnsServerQuery(std::move(new_connection), hosts_A_,
                                                       hosts_AAAA_, cnames_, record_ttl_);
    queries_.emplace_back(query);
  }

  void addHosts(const std::string& hostname, const IpList& ip, const record_type& type) {
    if (type == A) {
      hosts_A_[hostname] = ip;
    } else if (type == AAAA) {
      hosts_AAAA_[hostname] = ip;
    }
  }

  void addCName(const std::string& hostname, const std::string& cname) {
    cnames_[hostname] = cname;
  }

  void setRecordTtl(const std::chrono::seconds& ttl) { record_ttl_ = ttl; }

private:
  Event::Dispatcher& dispatcher_;

  HostMap hosts_A_;
  HostMap hosts_AAAA_;
  CNameMap cnames_;
  std::chrono::seconds record_ttl_;
  // All queries are tracked so we can do resource reclamation when the test is
  // over.
  std::vector<std::unique_ptr<TestDnsServerQuery>> queries_;
};

} // namespace

class DnsResolverImplPeer {
public:
  DnsResolverImplPeer(DnsResolverImpl* resolver) : resolver_(resolver) {}

  ares_channel channel() const { return resolver_->channel_; }
  const std::unordered_map<int, Event::FileEventPtr>& events() { return resolver_->events_; }
  // Reset the channel state for a DnsResolverImpl such that it will only use
  // TCP and optionally has a zero timeout (for validating timeout behavior).
  void resetChannelTcpOnly(bool zero_timeout) {
    ares_destroy(resolver_->channel_);
    ares_options options;
    // TCP-only connections to TestDnsServer, since even loopback UDP can be
    // lossy with a server under load.
    options.flags = ARES_FLAG_USEVC;
    // Avoid host-specific domain search behavior when testing to improve
    // determinism.
    options.ndomains = 0;
    options.timeout = 0;
    resolver_->initializeChannel(&options, ARES_OPT_FLAGS | ARES_OPT_DOMAINS |
                                               (zero_timeout ? ARES_OPT_TIMEOUTMS : 0));
  }

private:
  DnsResolverImpl* resolver_;
};

class DnsImplConstructor : public testing::Test {
protected:
  DnsImplConstructor() : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher()) {}

  Api::ApiPtr api_;
  Event::DispatcherPtr dispatcher_;
};

TEST_F(DnsImplConstructor, SupportsCustomResolvers) {
  char addr4str[INET_ADDRSTRLEN];
  // we pick a port that isn't 53 as the default resolve.conf might be
  // set to point to localhost.
  auto addr4 = Network::Utility::parseInternetAddressAndPort("127.0.0.1:54");
  char addr6str[INET6_ADDRSTRLEN];
  auto addr6 = Network::Utility::parseInternetAddressAndPort("[::1]:54");
  auto resolver = dispatcher_->createDnsResolver({addr4, addr6});
  auto peer = std::make_unique<DnsResolverImplPeer>(dynamic_cast<DnsResolverImpl*>(resolver.get()));
  ares_addr_port_node* resolvers;
  int result = ares_get_servers_ports(peer->channel(), &resolvers);
  EXPECT_EQ(result, ARES_SUCCESS);
  EXPECT_EQ(resolvers->family, AF_INET);
  EXPECT_EQ(resolvers->udp_port, 54);
  EXPECT_STREQ(inet_ntop(AF_INET, &resolvers->addr.addr4, addr4str, INET_ADDRSTRLEN), "127.0.0.1");
  EXPECT_EQ(resolvers->next->family, AF_INET6);
  EXPECT_EQ(resolvers->next->udp_port, 54);
  EXPECT_STREQ(inet_ntop(AF_INET6, &resolvers->next->addr.addr6, addr6str, INET6_ADDRSTRLEN),
               "::1");
  ares_free_data(resolvers);
}

// Custom instance that dispatches everything to a regular instance except for asString(), where
// it borks the port.
class CustomInstance : public Address::Instance {
public:
  CustomInstance(const std::string& address, uint32_t port) : instance_(address, port) {
    antagonistic_name_ = fmt::format("{}:borked_port_{}", address, port);
  }
  ~CustomInstance() override = default;

  // Address::Instance
  bool operator==(const Address::Instance& rhs) const override {
    return asString() == rhs.asString();
  }
  const std::string& asString() const override { return antagonistic_name_; }
  absl::string_view asStringView() const override { return antagonistic_name_; }
  const std::string& logicalName() const override { return antagonistic_name_; }
  Api::SysCallIntResult bind(int fd) const override { return instance_.bind(fd); }
  Api::SysCallIntResult connect(int fd) const override { return instance_.connect(fd); }
  const Address::Ip* ip() const override { return instance_.ip(); }
  IoHandlePtr socket(Address::SocketType type) const override { return instance_.socket(type); }
  Address::Type type() const override { return instance_.type(); }

private:
  std::string antagonistic_name_;
  Address::Ipv4Instance instance_;
};

TEST_F(DnsImplConstructor, SupportCustomAddressInstances) {
  auto test_instance(std::make_shared<CustomInstance>("127.0.0.1", 45));
  EXPECT_EQ(test_instance->asString(), "127.0.0.1:borked_port_45");
  auto resolver = dispatcher_->createDnsResolver({test_instance});
  auto peer = std::make_unique<DnsResolverImplPeer>(dynamic_cast<DnsResolverImpl*>(resolver.get()));
  ares_addr_port_node* resolvers;
  int result = ares_get_servers_ports(peer->channel(), &resolvers);
  EXPECT_EQ(result, ARES_SUCCESS);
  EXPECT_EQ(resolvers->family, AF_INET);
  EXPECT_EQ(resolvers->udp_port, 45);
  char addr4str[INET_ADDRSTRLEN];
  EXPECT_STREQ(inet_ntop(AF_INET, &resolvers->addr.addr4, addr4str, INET_ADDRSTRLEN), "127.0.0.1");
  ares_free_data(resolvers);
}

TEST_F(DnsImplConstructor, BadCustomResolvers) {
  envoy::api::v2::core::Address pipe_address;
  pipe_address.mutable_pipe()->set_path("foo");
  auto pipe_instance = Network::Utility::protobufAddressToAddress(pipe_address);
  EXPECT_THROW_WITH_MESSAGE(dispatcher_->createDnsResolver({pipe_instance}), EnvoyException,
                            "DNS resolver 'foo' is not an IP address");
}

class DnsImplTest : public testing::TestWithParam<Address::IpVersion> {
public:
  DnsImplTest() : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher()) {}

  void SetUp() override {
    resolver_ = dispatcher_->createDnsResolver({});

    // Instantiate TestDnsServer and listen on a random port on the loopback address.
    server_ = std::make_unique<TestDnsServer>(*dispatcher_);
    socket_ = std::make_unique<Network::TcpListenSocket>(
        Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr, true);
    listener_ = dispatcher_->createListener(*socket_, *server_, true, false);

    // Point c-ares at the listener with no search domains and TCP-only.
    peer_ = std::make_unique<DnsResolverImplPeer>(dynamic_cast<DnsResolverImpl*>(resolver_.get()));
    peer_->resetChannelTcpOnly(zero_timeout());
    ares_set_servers_ports_csv(peer_->channel(), socket_->localAddress()->asString().c_str());
  }

  void TearDown() override {
    // Make sure we clean this up before dispatcher destruction.
    listener_.reset();
    server_.reset();
  }

  std::list<Address::InstanceConstSharedPtr>
  getAddressList(const std::list<DnsResponse>& response) {
    std::list<Address::InstanceConstSharedPtr> address;

    for_each(response.begin(), response.end(),
             [&](DnsResponse resp) { address.emplace_back(resp.address_); });
    return address;
  }

protected:
  // Should the DnsResolverImpl use a zero timeout for c-ares queries?
  virtual bool zero_timeout() const { return false; }
  std::unique_ptr<TestDnsServer> server_;
  std::unique_ptr<DnsResolverImplPeer> peer_;
  Network::MockConnectionHandler connection_handler_;
  Network::TcpListenSocketPtr socket_;
  std::unique_ptr<Network::Listener> listener_;
  Api::ApiPtr api_;
  Event::DispatcherPtr dispatcher_;
  DnsResolverSharedPtr resolver_;
};

static bool hasAddress(const std::list<Address::InstanceConstSharedPtr>& results,
                       const std::string& address) {
  for (const auto& result : results) {
    if (result->ip()->addressAsString() == address) {
      return true;
    }
  }
  return false;
}

// Parameterize the DNS test server socket address.
INSTANTIATE_TEST_SUITE_P(IpVersions, DnsImplTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// Validate that when DnsResolverImpl is destructed with outstanding requests,
// that we don't invoke any callbacks. This is a regression test from
// development, where segfaults were encountered due to callback invocations on
// destruction.
TEST_P(DnsImplTest, DestructPending) {
  EXPECT_NE(nullptr, resolver_->resolve("", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          FAIL();
                                          UNREFERENCED_PARAMETER(results);
                                        }));
  // Also validate that pending events are around to exercise the resource
  // reclamation path.
  EXPECT_GT(peer_->events().size(), 0U);
}

// Validate basic success/fail lookup behavior. The empty request will connect
// to TestDnsServer, but localhost should resolve via the hosts file with no
// asynchronous behavior or network events.
TEST_P(DnsImplTest, LocalLookup) {
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(address_list.empty());

  if (GetParam() == Address::IpVersion::v4) {
    // EXPECT_CALL(dispatcher_, post(_));
    EXPECT_EQ(nullptr, resolver_->resolve("localhost", DnsLookupFamily::V4Only,
                                          [&](std::list<DnsResponse>&& results) -> void {
                                            address_list = getAddressList(results);
                                          }));
    EXPECT_TRUE(hasAddress(address_list, "127.0.0.1"));
    EXPECT_FALSE(hasAddress(address_list, "::1"));
  }

  if (GetParam() == Address::IpVersion::v6) {
    const std::string error_msg =
        "Synchronous DNS IPv6 localhost resolution failed. Please verify localhost resolves to ::1 "
        "in /etc/hosts, since this misconfiguration is a common cause of these failures.";
    EXPECT_EQ(nullptr, resolver_->resolve("localhost", DnsLookupFamily::V6Only,
                                          [&](std::list<DnsResponse>&& results) -> void {
                                            address_list = getAddressList(results);
                                          }))
        << error_msg;
    EXPECT_TRUE(hasAddress(address_list, "::1")) << error_msg;
    EXPECT_FALSE(hasAddress(address_list, "127.0.0.1"));

    EXPECT_EQ(nullptr, resolver_->resolve("localhost", DnsLookupFamily::Auto,
                                          [&](std::list<DnsResponse>&& results) -> void {
                                            address_list = getAddressList(results);
                                          }))
        << error_msg;
    EXPECT_FALSE(hasAddress(address_list, "127.0.0.1"));
    EXPECT_TRUE(hasAddress(address_list, "::1")) << error_msg;
  }
}

TEST_P(DnsImplTest, DnsIpAddressVersionV6) {
  std::list<Address::InstanceConstSharedPtr> address_list;
  server_->addHosts("some.good.domain", {"1::2"}, AAAA);
  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "1::2"));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_FALSE(hasAddress(address_list, "1::2"));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V6Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "1::2"));
}

// Validate exception behavior during c-ares callbacks.
TEST_P(DnsImplTest, CallbackException) {
  // Force immediate resolution, which will trigger a c-ares exception unsafe
  // state providing regression coverage for #4307.
  EXPECT_EQ(nullptr, resolver_->resolve(
                         "1.2.3.4", DnsLookupFamily::V4Only,
                         [&](const std::list<DnsResponse> &
                             /*results*/) -> void { throw EnvoyException("Envoy exception"); }));
  EXPECT_THROW_WITH_MESSAGE(dispatcher_->run(Event::Dispatcher::RunType::Block), EnvoyException,
                            "Envoy exception");
  EXPECT_EQ(nullptr, resolver_->resolve(
                         "1.2.3.4", DnsLookupFamily::V4Only,
                         [&](const std::list<DnsResponse> &
                             /*results*/) -> void { throw std::runtime_error("runtime error"); }));
  EXPECT_THROW_WITH_MESSAGE(dispatcher_->run(Event::Dispatcher::RunType::Block), EnvoyException,
                            "runtime error");
  EXPECT_EQ(nullptr, resolver_->resolve("1.2.3.4", DnsLookupFamily::V4Only,
                                        [&](const std::list<DnsResponse> &
                                            /*results*/) -> void { throw std::string(); }));
  EXPECT_THROW_WITH_MESSAGE(dispatcher_->run(Event::Dispatcher::RunType::Block), EnvoyException,
                            "unknown");
}

TEST_P(DnsImplTest, DnsIpAddressVersion) {
  std::list<Address::InstanceConstSharedPtr> address_list;
  server_->addHosts("some.good.domain", {"1.2.3.4"}, A);
  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "1.2.3.4"));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "1.2.3.4"));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V6Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_FALSE(hasAddress(address_list, "1.2.3.4"));
}

// Validate success/fail lookup behavior via TestDnsServer. This exercises the
// network event handling in DnsResolverImpl.
TEST_P(DnsImplTest, RemoteAsyncLookup) {
  server_->addHosts("some.good.domain", {"201.134.56.7"}, A);
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("some.bad.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(address_list.empty());

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "201.134.56.7"));
}

// Validate that multiple A records are correctly passed to the callback.
TEST_P(DnsImplTest, MultiARecordLookup) {
  server_->addHosts("some.good.domain", {"201.134.56.7", "123.4.5.6", "6.5.4.3"}, A);
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "201.134.56.7"));
  EXPECT_TRUE(hasAddress(address_list, "123.4.5.6"));
  EXPECT_TRUE(hasAddress(address_list, "6.5.4.3"));
}

TEST_P(DnsImplTest, CNameARecordLookupV4) {
  server_->addCName("root.cnam.domain", "result.cname.domain");
  server_->addHosts("result.cname.domain", {"201.134.56.7"}, A);
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("root.cnam.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "201.134.56.7"));
}

TEST_P(DnsImplTest, CNameARecordLookupWithV6) {
  server_->addCName("root.cnam.domain", "result.cname.domain");
  server_->addHosts("result.cname.domain", {"201.134.56.7"}, A);
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("root.cnam.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "201.134.56.7"));
}

TEST_P(DnsImplTest, MultiARecordLookupWithV6) {
  server_->addHosts("some.good.domain", {"201.134.56.7", "123.4.5.6", "6.5.4.3"}, A);
  server_->addHosts("some.good.domain", {"1::2", "1::2:3", "1::2:3:4"}, AAAA);
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "201.134.56.7"));
  EXPECT_TRUE(hasAddress(address_list, "123.4.5.6"));
  EXPECT_TRUE(hasAddress(address_list, "6.5.4.3"));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "1::2"));
  EXPECT_TRUE(hasAddress(address_list, "1::2:3"));
  EXPECT_TRUE(hasAddress(address_list, "1::2:3:4"));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V6Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "1::2"));
  EXPECT_TRUE(hasAddress(address_list, "1::2:3"));
  EXPECT_TRUE(hasAddress(address_list, "1::2:3:4"));
}

// Validate working of cancellation provided by ActiveDnsQuery return.
TEST_P(DnsImplTest, Cancel) {
  server_->addHosts("some.good.domain", {"201.134.56.7"}, A);

  ActiveDnsQuery* query = resolver_->resolve("some.domain", DnsLookupFamily::Auto,
                                             [](std::list<DnsResponse> &&) -> void { FAIL(); });

  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  ASSERT_NE(nullptr, query);
  query->cancel();

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(hasAddress(address_list, "201.134.56.7"));
}

// Validate working of querying ttl of resource record.
TEST_P(DnsImplTest, RecordTtlLookup) {
  if (GetParam() == Address::IpVersion::v4) {
    EXPECT_EQ(nullptr, resolver_->resolve("localhost", DnsLookupFamily::V4Only,
                                          [](std::list<DnsResponse>&& results) -> void {
                                            for (auto address : results) {
                                              EXPECT_EQ(address.ttl_, std::chrono::seconds(0));
                                            }
                                          }));
  }

  if (GetParam() == Address::IpVersion::v6) {
    EXPECT_EQ(nullptr, resolver_->resolve("localhost", DnsLookupFamily::V6Only,
                                          [](std::list<DnsResponse>&& results) -> void {
                                            for (auto address : results) {
                                              EXPECT_EQ(address.ttl_, std::chrono::seconds(0));
                                            }
                                          }));

    EXPECT_EQ(nullptr, resolver_->resolve("localhost", DnsLookupFamily::Auto,
                                          [](std::list<DnsResponse>&& results) -> void {
                                            for (auto address : results) {
                                              EXPECT_EQ(address.ttl_, std::chrono::seconds(0));
                                            }
                                          }));
  }

  server_->addHosts("some.good.domain", {"201.134.56.7", "123.4.5.6", "6.5.4.3"}, A);
  server_->addHosts("some.good.domain", {"1::2", "1::2:3", "1::2:3:4"}, AAAA);
  server_->setRecordTtl(std::chrono::seconds(300));

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          for (auto address : results) {
                                            EXPECT_EQ(address.ttl_, std::chrono::seconds(300));
                                          }

                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::Auto,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          for (auto address : results) {
                                            EXPECT_EQ(address.ttl_, std::chrono::seconds(300));
                                          }
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V6Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          for (auto address : results) {
                                            EXPECT_EQ(address.ttl_, std::chrono::seconds(300));
                                          }
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  std::list<Address::InstanceConstSharedPtr> address_list;

  server_->addHosts("domain.onion", {"1.2.3.4"}, A);
  server_->addHosts("domain.onion.", {"2.3.4.5"}, A);

  // test onion domain
  EXPECT_EQ(nullptr, resolver_->resolve("domain.onion", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));
  EXPECT_TRUE(address_list.empty());

  EXPECT_EQ(nullptr, resolver_->resolve("domain.onion.", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));
  EXPECT_TRUE(address_list.empty());
}

class DnsImplZeroTimeoutTest : public DnsImplTest {
protected:
  bool zero_timeout() const override { return true; }
};

// Parameterize the DNS test server socket address.
INSTANTIATE_TEST_SUITE_P(IpVersions, DnsImplZeroTimeoutTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// Validate that timeouts result in an empty callback.
TEST_P(DnsImplZeroTimeoutTest, Timeout) {
  server_->addHosts("some.good.domain", {"201.134.56.7"}, A);
  std::list<Address::InstanceConstSharedPtr> address_list;
  EXPECT_NE(nullptr, resolver_->resolve("some.good.domain", DnsLookupFamily::V4Only,
                                        [&](std::list<DnsResponse>&& results) -> void {
                                          address_list = getAddressList(results);
                                          dispatcher_->exit();
                                        }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(address_list.empty());
}

// Validate that the resolution timeout timer is enabled if we don't resolve
// immediately.
TEST(DnsImplUnitTest, PendingTimerEnable) {
  InSequence s;
  Event::MockDispatcher dispatcher;
  Event::MockTimer* timer = new NiceMock<Event::MockTimer>();
  EXPECT_CALL(dispatcher, createTimer_(_)).WillOnce(Return(timer));
  DnsResolverImpl resolver(dispatcher, {});
  Event::FileEvent* file_event = new NiceMock<Event::MockFileEvent>();
  EXPECT_CALL(dispatcher, createFileEvent_(_, _, _, _)).WillOnce(Return(file_event));
  EXPECT_CALL(*timer, enableTimer(_));
  EXPECT_NE(nullptr, resolver.resolve("some.bad.domain.invalid", DnsLookupFamily::V4Only,
                                      [&](std::list<DnsResponse>&& results) {
                                        UNREFERENCED_PARAMETER(results);
                                      }));
}

} // namespace Network
} // namespace Envoy
