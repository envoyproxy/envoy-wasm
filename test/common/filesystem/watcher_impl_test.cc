#include <cstdint>
#include <fstream>

#include "common/common/assert.h"
#include "common/event/dispatcher_impl.h"
#include "common/filesystem/watcher_impl.h"

#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Filesystem {

class WatcherImplTest : public testing::Test {
protected:
  WatcherImplTest() : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher()) {}

  Api::ApiPtr api_;
  Event::DispatcherPtr dispatcher_;
};

class WatchCallback {
public:
  MOCK_METHOD(void, called, (uint32_t));
};

TEST_F(WatcherImplTest, All) {
  Filesystem::WatcherPtr watcher = dispatcher_->createFilesystemWatcher();

  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_target").c_str());
  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_link").c_str());
  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_new_target").c_str());
  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_new_link").c_str());

  TestUtility::createDirectory(TestEnvironment::temporaryPath("envoy_test"));
  { std::ofstream file(TestEnvironment::temporaryPath("envoy_test/watcher_target")); }
  TestUtility::createSymlink(TestEnvironment::temporaryPath("envoy_test/watcher_target"),
                             TestEnvironment::temporaryPath("envoy_test/watcher_link"));

  { std::ofstream file(TestEnvironment::temporaryPath("envoy_test/watcher_new_target")); }
  TestUtility::createSymlink(TestEnvironment::temporaryPath("envoy_test/watcher_new_target"),
                             TestEnvironment::temporaryPath("envoy_test/watcher_new_link"));

  WatchCallback callback;
  EXPECT_CALL(callback, called(Watcher::Events::MovedTo)).Times(2);
  watcher->addWatch(TestEnvironment::temporaryPath("envoy_test/watcher_link"),
                    Watcher::Events::MovedTo, [&](uint32_t events) -> void {
                      callback.called(events);
                      dispatcher_->exit();
                    });
  TestUtility::renameFile(TestEnvironment::temporaryPath("envoy_test/watcher_new_link"),
                          TestEnvironment::temporaryPath("envoy_test/watcher_link"));
  dispatcher_->run(Event::Dispatcher::RunType::Block);

  TestUtility::createSymlink(TestEnvironment::temporaryPath("envoy_test/watcher_new_target"),
                             TestEnvironment::temporaryPath("envoy_test/watcher_new_link"));
  TestUtility::renameFile(TestEnvironment::temporaryPath("envoy_test/watcher_new_link"),
                          TestEnvironment::temporaryPath("envoy_test/watcher_link"));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
}

TEST_F(WatcherImplTest, Create) {
  Filesystem::WatcherPtr watcher = dispatcher_->createFilesystemWatcher();

  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_target").c_str());
  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_link").c_str());
  unlink(TestEnvironment::temporaryPath("envoy_test/watcher_new_link").c_str());
  unlink(TestEnvironment::temporaryPath("envoy_test/other_file").c_str());

  TestUtility::createDirectory(TestEnvironment::temporaryPath("envoy_test"));
  { std::ofstream file(TestEnvironment::temporaryPath("envoy_test/watcher_target")); }

  WatchCallback callback;
  EXPECT_CALL(callback, called(Watcher::Events::MovedTo));
  watcher->addWatch(TestEnvironment::temporaryPath("envoy_test/watcher_link"),
                    Watcher::Events::MovedTo, [&](uint32_t events) -> void {
                      callback.called(events);
                      dispatcher_->exit();
                    });

  { std::ofstream file(TestEnvironment::temporaryPath("envoy_test/other_file")); }
  dispatcher_->run(Event::Dispatcher::RunType::NonBlock);

  TestUtility::createSymlink(TestEnvironment::temporaryPath("envoy_test/watcher_target"),
                             TestEnvironment::temporaryPath("envoy_test/watcher_new_link"));
  TestUtility::renameFile(TestEnvironment::temporaryPath("envoy_test/watcher_new_link"),
                          TestEnvironment::temporaryPath("envoy_test/watcher_link"));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
}

TEST_F(WatcherImplTest, Modify) {
  Filesystem::WatcherPtr watcher = dispatcher_->createFilesystemWatcher();

  TestUtility::createDirectory(TestEnvironment::temporaryPath("envoy_test"));
  std::ofstream file(TestEnvironment::temporaryPath("envoy_test/watcher_target"));

  WatchCallback callback;
  watcher->addWatch(TestEnvironment::temporaryPath("envoy_test/watcher_target"),
                    Watcher::Events::Modified, [&](uint32_t events) -> void {
                      callback.called(events);
                      dispatcher_->exit();
                    });
  dispatcher_->run(Event::Dispatcher::RunType::NonBlock);

  file << "text" << std::flush;
  EXPECT_CALL(callback, called(Watcher::Events::Modified));
  dispatcher_->run(Event::Dispatcher::RunType::NonBlock);
}

TEST_F(WatcherImplTest, BadPath) {
  Filesystem::WatcherPtr watcher = dispatcher_->createFilesystemWatcher();

  EXPECT_THROW(
      watcher->addWatch("this_is_not_a_file", Watcher::Events::MovedTo, [&](uint32_t) -> void {}),
      EnvoyException);

  EXPECT_THROW(watcher->addWatch("this_is_not_a_dir/file", Watcher::Events::MovedTo,
                                 [&](uint32_t) -> void {}),
               EnvoyException);
}

TEST_F(WatcherImplTest, ParentDirectoryRemoved) {
  Filesystem::WatcherPtr watcher = dispatcher_->createFilesystemWatcher();

  TestUtility::createDirectory(TestEnvironment::temporaryPath("envoy_test_empty"));

  WatchCallback callback;
  EXPECT_CALL(callback, called(testing::_)).Times(0);

  watcher->addWatch(TestEnvironment::temporaryPath("envoy_test_empty/watcher_link"),
                    Watcher::Events::MovedTo,
                    [&](uint32_t events) -> void { callback.called(events); });

  int rc = rmdir(TestEnvironment::temporaryPath("envoy_test_empty").c_str());
  EXPECT_EQ(0, rc);

  dispatcher_->run(Event::Dispatcher::RunType::NonBlock);
}

TEST_F(WatcherImplTest, RootDirectoryPath) {
  Filesystem::WatcherPtr watcher = dispatcher_->createFilesystemWatcher();

  EXPECT_NO_THROW(watcher->addWatch("/", Watcher::Events::MovedTo, [&](uint32_t) -> void {}));
}

} // namespace Filesystem
} // namespace Envoy
