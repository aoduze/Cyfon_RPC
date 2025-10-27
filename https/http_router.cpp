#include "http_router.h"
#include <spdlog/spdlog.h>

namespace cyfon_rpc {

    void HttpRouter::registerRoute(const std::string& path,
                                   uint32_t service_id,
                                   uint32_t method_id) {
        std::unqiue_lock lock(mutex_);

        // 检查是否存在
        if(routes_.count(path)) {
            spdlog::warn("Route '{}' already exists, overwriting", path);
        }

        routes_[path] = RouteTarget{service_id, method_id};
        spdlog::info("Registered route: {} -> ServiceID: {}, MethodID: {}", path, 
            service_id, method_id);
    }

    std::optional<RouteTarget> HttpRouter::findroute(std::string_view path) const {
        std::shared_lock lock(mutex_);

        auto it = routes_.find(path);
        if(it == routes_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void HttpRouter::clear() {
        std::unqiue_lock lock(mutex_);

        routes_.clear();
        spdlog::info("Cleared all routes");
    }

    size_t HttpRouter::size() const {
        std::shared_lock lock(mutex_);
        return routes_.size();
    }
}