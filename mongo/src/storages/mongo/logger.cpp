#include "logger.hpp"

#include <stdexcept>

#include <logging/level.hpp>
#include <logging/log.hpp>

namespace storages::mongo::impl {
namespace {

logging::Level ConvertLogLevel(mongoc_log_level_t level) {
  switch (level) {
    case MONGOC_LOG_LEVEL_ERROR:
    case MONGOC_LOG_LEVEL_CRITICAL:
    case MONGOC_LOG_LEVEL_WARNING:
      return logging::Level::kWarning;

    case MONGOC_LOG_LEVEL_MESSAGE:
    case MONGOC_LOG_LEVEL_INFO:
      return logging::Level::kDebug;

    case MONGOC_LOG_LEVEL_DEBUG:
    case MONGOC_LOG_LEVEL_TRACE:
      return logging::Level::kTrace;
  }
  LOG_WARNING() << "Unexpected mongoc log level (" << level << ')';
  return logging::Level::kWarning;
}

}  // namespace

void LogMongocMessage(mongoc_log_level_t level, const char* domain,
                      const char* message, void*) {
  try {
    LOG(ConvertLogLevel(level))
        << "Mongo driver " << mongoc_log_level_str(level) << " [" << domain
        << "]: " << message;
  } catch (const std::exception&) {
    // cannot throw here
  }
}

}  // namespace storages::mongo::impl
