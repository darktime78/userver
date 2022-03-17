#pragma once

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/taxi_config/source.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Helper component to configure logging.
///
/// The functionality is not in Trace or Logger components because that
/// introduces circular dependency between Logger and DynamicConfig.
///
/// ## Dynamic config
/// * @ref USERVER_NO_LOG_SPANS
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// limited-logging-enable | set to true to make LOG_LIMITED drop repeated logs | -
/// limited-logging-interval | utils::StringToDuration suitable duration string to group repeated logs into one message | -
///
/// ## Config example:
///
/// @snippet components/common_component_list_test.cpp Sample logging configurator component config

// clang-format on
class LoggingConfigurator final : public impl::ComponentBase {
 public:
  static constexpr auto kName = "logging-configurator";

  LoggingConfigurator(const ComponentConfig& config,
                      const ComponentContext& context);

  ~LoggingConfigurator() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnConfigUpdate(const dynamic_config::Snapshot& config);

  concurrent::AsyncEventSubscriberScope config_subscription_;
};

/// }@

template <>
inline constexpr bool kHasValidate<LoggingConfigurator> = true;

}  // namespace components

USERVER_NAMESPACE_END
