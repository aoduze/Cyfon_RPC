#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <string_view>
#include <shared_mutex>

namespace cyfon_rpc {
    struct RouteTarget {
        uint32_t service_id;
        uint32_t method_id;
    };

    class HttpRouter {
    public:
        // 注册路由
        void registerRoute(const std::string& path, uint32_t service_id, uint32_t method_id);

        // 根据路径查找路由
        std::optional<RouteTarget> findroute(const std::string& path);

        void clear();

        // 获取路由数量
        size_t size() const;

    private:
        std::unordered_map<std::string, RouteTarget> routes_;
        mutable std::shared_mutex  mutex_;
    };


}