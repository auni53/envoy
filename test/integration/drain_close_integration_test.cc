#include "test/integration/http_protocol_integration.h"

namespace Envoy {
namespace {

class DrainCloseIntegrationTest : public Event::TestUsingSimulatedTime, public HttpProtocolIntegrationTest {
};

TEST_P(DrainCloseIntegrationTest, DrainClose) {
  drain_time_ = std::chrono::seconds(1);
  config_helper_.addFilter(ConfigHelper::defaultHealthCheckFilter());
  initialize();

  // BufferingStreamDecoderPtr admin_response = IntegrationUtil::makeSingleRequest(
  //     lookupPort("admin"), "POST", "/drain_listeners", "", admin_request_type, version_);
  // EXPECT_TRUE(admin_response->complete());
  // EXPECT_EQ("200", admin_response->headers().Status()->value().getStringView());
  // EXPECT_EQ("OK\n", admin_response->body());
  absl::Notification drain_sequence_started;
  test_server_->server().dispatcher().post([this, &drain_sequence_started]() {
    test_server_->server().drainManager().startDrainSequence(nullptr);
    drain_sequence_started.Notify();
  });
  drain_sequence_started.WaitForNotification();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  response->waitForEndStream();

  // Advance once to finish the drain sequence, once to finish connection
  // cleanup
  std::cerr << "[AUNI] " << "Advance 1" << "\n";
  simTime().advanceTimeWait(std::chrono::seconds(5));
  std::cerr << "[AUNI] " << "Advance 2" << "\n";
  simTime().advanceTimeWait(std::chrono::seconds(5));
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  if (downstream_protocol_ == Http::CodecClient::Type::HTTP2) {
    EXPECT_TRUE(codec_client_->sawGoAway());
  }
}

INSTANTIATE_TEST_SUITE_P(Protocols, DrainCloseIntegrationTest,
                         testing::ValuesIn(HttpProtocolIntegrationTest::getProtocolTestParams(
                             {Http::CodecClient::Type::HTTP1, Http::CodecClient::Type::HTTP2},
                             {FakeHttpConnection::Type::HTTP1})),
                         HttpProtocolIntegrationTest::protocolTestParamsToString);

}  // namespace
}  // namespace Envoy
