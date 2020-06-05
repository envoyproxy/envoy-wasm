#include "test/integration/integration.h"
#include "test/integration/utility.h"
#include "test/test_common/utility.h"

namespace Envoy {

class DirectResponseIntegrationTest : public testing::TestWithParam<Network::Address::IpVersion>,
                                      public BaseIntegrationTest {
public:
  DirectResponseIntegrationTest() : BaseIntegrationTest(GetParam(), directResponseConfig()) {}

  static std::string directResponseConfig() {
    return absl::StrCat(ConfigHelper::baseConfig(), R"EOF(
    filter_chains:
      filters:
      - name: direct_response
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.network.direct_response.v3.Config
          response:
            inline_string: "hello, world!\n"
      )EOF");
  }

  /**
   * Initializer for an individual test.
   */
  void SetUp() override {
    useListenerAccessLog("%RESPONSE_CODE_DETAILS%");
    BaseIntegrationTest::initialize();
  }

  /**
   *  Destructor for an individual test.
   */
  void TearDown() override {
    test_server_.reset();
    fake_upstreams_.clear();
  }
};

INSTANTIATE_TEST_SUITE_P(IpVersions, DirectResponseIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(DirectResponseIntegrationTest, Hello) {
  std::string response;
  auto connection = createConnectionDriver(
      lookupPort("listener_0"), "hello",
      [&response](Network::ClientConnection& conn, const Buffer::Instance& data) -> void {
        response.append(data.toString());
        conn.close(Network::ConnectionCloseType::FlushWrite);
      });
  connection->run();
  EXPECT_EQ("hello, world!\n", response);
  EXPECT_THAT(waitForAccessLog(listener_access_log_name_),
              testing::HasSubstr(StreamInfo::ResponseCodeDetails::get().DirectResponse));
}

} // namespace Envoy
