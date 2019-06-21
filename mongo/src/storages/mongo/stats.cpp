#include <storages/mongo/stats.hpp>

#include <exception>

#include <tracing/span.hpp>

namespace storages::mongo::stats {
namespace {

OperationStatisticsItem::ErrorType ToErrorType(
    MongoError::Kind mongo_error_kind) {
  switch (mongo_error_kind) {
    case MongoError::Kind::kNetwork:
      return OperationStatisticsItem::ErrorType::kNetwork;
    case MongoError::Kind::kClusterUnavailable:
      return OperationStatisticsItem::ErrorType::kNetwork;
    case MongoError::Kind::kIncompatibleServer:
      return OperationStatisticsItem::ErrorType::kBadServerVersion;
    case MongoError::Kind::kAuthentication:
      return OperationStatisticsItem::ErrorType::kAuthFailure;
    case MongoError::Kind::kInvalidQueryArgument:
      return OperationStatisticsItem::ErrorType::kBadQueryArgument;
    case MongoError::Kind::kServer:
      return OperationStatisticsItem::ErrorType::kServer;
    case MongoError::Kind::kWriteConcern:
      return OperationStatisticsItem::ErrorType::kWriteConcern;
    case MongoError::Kind::kDuplicateKey:
      return OperationStatisticsItem::ErrorType::kDuplicateKey;

    case MongoError::Kind::kNoError:
    case MongoError::Kind::kQuery:
    case MongoError::Kind::kOther:;  // other
  }
  return OperationStatisticsItem::ErrorType::kOther;
}

template <typename Rep, typename Period>
auto GetMilliseconds(const std::chrono::duration<Rep, Period>& duration) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

}  // namespace

void OperationStatisticsItem::Reset() {
  for (auto& counter : counters) counter = 0;
  timings.Reset();
}

std::string ToString(OperationStatisticsItem::ErrorType type) {
  using Type = OperationStatisticsItem::ErrorType;
  switch (type) {
    case Type::kSuccess:
      return "success";
    case Type::kNetwork:
      return "network";
    case Type::kClusterUnavailable:
      return "cluster-unavailable";
    case Type::kBadServerVersion:
      return "server-version";
    case Type::kAuthFailure:
      return "auth-failure";
    case Type::kBadQueryArgument:
      return "bad-query-arg";
    case Type::kDuplicateKey:
      return "duplicate-key";
    case Type::kWriteConcern:
      return "write-concern";
    case Type::kServer:
      return "server-error";
    case Type::kOther:
      return "other";

    case Type::kErrorTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

void ReadOperationStatistics::Reset() {
  for (auto& item : items) item.Reset();
}

std::string ToString(ReadOperationStatistics::OpType type) {
  using Type = ReadOperationStatistics::OpType;
  switch (type) {
    case Type::kCount:
      return "count";
    case Type::kCountApprox:
      return "count-approx";
    case Type::kFind:
      return "find";
    case Type::kGetMore:
      return "getmore";

    case Type::kOpTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

void WriteOperationStatistics::Reset() {
  for (auto& item : items) item.Reset();
}

std::string ToString(WriteOperationStatistics::OpType type) {
  using Type = WriteOperationStatistics::OpType;
  switch (type) {
    case Type::kInsertOne:
      return "insert-one";
    case Type::kInsertMany:
      return "insert-many";
    case Type::kReplaceOne:
      return "replace-one";
    case Type::kUpdateOne:
      return "update-one";
    case Type::kUpdateMany:
      return "update-many";
    case Type::kDeleteOne:
      return "delete-one";
    case Type::kDeleteMany:
      return "delete-many";
    case Type::kFindAndModify:
      return "find-and-modify";
    case Type::kFindAndRemove:
      return "find-and-remove";
    case Type::kBulk:
      return "bulk";

    case Type::kOpTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

void PoolConnectStatistics::Reset() {
  requested = 0;
  created = 0;
  closed = 0;
  overload = 0;
  for (auto& item : items) item.Reset();
  request_timings.Reset();
  queue_wait_timings.Reset();
}

std::string ToString(PoolConnectStatistics::OpType type) {
  using Type = PoolConnectStatistics::OpType;
  switch (type) {
    case Type::kPing:
      return "ping";

    case Type::kOpTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

template <typename OpStats>
OperationStopwatch<OpStats>::OperationStopwatch()
    : scope_time_(tracing::Span::CurrentSpan().CreateScopeTime()),
      op_type_(OpStats::OpType::kOpTypesCount) {}

template <typename OpStats>
OperationStopwatch<OpStats>::OperationStopwatch(
    std::shared_ptr<Aggregator<OpStats>> stats_agg,
    typename OpStats::OpType op_type)
    : OperationStopwatch<OpStats>() {
  Reset(std::move(stats_agg), op_type);
}

template <typename OpStats>
OperationStopwatch<OpStats>::~OperationStopwatch() {
  if (stats_agg_) Account(OperationStatisticsItem::ErrorType::kOther);
}

template <typename OpStats>
void OperationStopwatch<OpStats>::Reset(
    std::shared_ptr<Aggregator<OpStats>> stats_agg,
    typename OpStats::OpType op_type) {
  stats_agg_ = std::move(stats_agg);
  op_type_ = op_type;

  scope_time_.Reset(ToString(op_type_));
}

template <typename OpStats>
void OperationStopwatch<OpStats>::AccountSuccess() {
  Account(OperationStatisticsItem::ErrorType::kSuccess);
}

template <typename OpStats>
void OperationStopwatch<OpStats>::AccountError(MongoError::Kind kind) {
  Account(ToErrorType(kind));
}

template <typename OpStats>
void OperationStopwatch<OpStats>::Discard() {
  stats_agg_.reset();
  scope_time_.Discard();
}

template <typename OpStats>
void OperationStopwatch<OpStats>::Account(
    OperationStatisticsItem::ErrorType error_type) noexcept {
  const auto stats_agg = std::exchange(stats_agg_, nullptr);
  if (!stats_agg) return;

  try {
    auto& stats_item = stats_agg->GetCurrentCounter().items[op_type_];
    ++stats_item.counters[error_type];
    stats_item.timings.Account(GetMilliseconds(scope_time_.Reset()));
  } catch (const std::exception&) {
    // ignore
  }
}

// explicit instantiations
template class OperationStopwatch<ReadOperationStatistics>;
template class OperationStopwatch<WriteOperationStatistics>;
template class OperationStopwatch<PoolConnectStatistics>;

PoolRequestStopwatch::PoolRequestStopwatch(
    std::shared_ptr<Aggregator<PoolConnectStatistics>> stats_agg)
    : stats_agg_(std::move(stats_agg)),
      scope_time_(tracing::Span::CurrentSpan().CreateScopeTime("conn-wait")) {}

PoolRequestStopwatch::~PoolRequestStopwatch() {
  try {
    auto& stats = stats_agg_->GetCurrentCounter();
    ++stats.requested;
    stats.request_timings.Account(GetMilliseconds(scope_time_.Reset()));
  } catch (const std::exception&) {
    // ignore
  }
}

PoolQueueStopwatch::PoolQueueStopwatch(
    std::shared_ptr<Aggregator<PoolConnectStatistics>> stats_agg)
    : stats_agg_(std::move(stats_agg)),
      scope_time_(
          tracing::Span::CurrentSpan().CreateScopeTime("conn-throttle")) {}

PoolQueueStopwatch::~PoolQueueStopwatch() {
  if (stats_agg_) Stop();
}

void PoolQueueStopwatch::Stop() noexcept {
  const auto stats_agg = std::exchange(stats_agg_, nullptr);
  if (!stats_agg) return;

  try {
    auto& stats = stats_agg->GetCurrentCounter();
    stats.queue_wait_timings.Account(GetMilliseconds(scope_time_.Reset()));
  } catch (const std::exception&) {
    // ignore
  }
}

}  // namespace storages::mongo::stats
