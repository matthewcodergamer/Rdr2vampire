#include "nightwalker/game/ModelStreamRequest.h"
#include <algorithm>
namespace nightwalker::game {
ModelStreamStatus ModelStreamRequest::Begin(IGameApi& api, ModelHash model, std::uint64_t nowMs, std::uint64_t timeoutMs) noexcept {
 Release(api);
 model_=model;startedAtMs_=nowMs;timeoutMs_=std::clamp<std::uint64_t>(timeoutMs,250,15000);
 if(!api.IsPedModelAvailable(model_)){status_=ModelStreamStatus::InvalidModel;return status_;}
 if(api.IsModelLoaded(model_)){status_=ModelStreamStatus::Loaded;return status_;}
 api.RequestModel(model_);status_=ModelStreamStatus::Loading;return status_;
}
ModelStreamStatus ModelStreamRequest::Update(IGameApi& api,std::uint64_t nowMs) noexcept {
 if(status_!=ModelStreamStatus::Loading)return status_;
 if(api.IsModelLoaded(model_)){status_=ModelStreamStatus::Loaded;return status_;}
 if(nowMs-startedAtMs_>=timeoutMs_){api.ReleaseModel(model_);status_=ModelStreamStatus::TimedOut;}
 return status_;
}
void ModelStreamRequest::Release(IGameApi& api) noexcept {
 if(model_!=0&&(status_==ModelStreamStatus::Loading||status_==ModelStreamStatus::Loaded))api.ReleaseModel(model_);
 model_=0;startedAtMs_=0;timeoutMs_=0;status_=ModelStreamStatus::Idle;
}
}
