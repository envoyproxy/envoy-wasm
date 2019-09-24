#include "test/tools/router_check/coverage.h"

#include <algorithm>

#include "envoy/api/v2/core/base.pb.h"

namespace Envoy {
double RouteCoverage::report() {
  uint64_t route_weight = 0;
  for (const auto& covered_field : coverageFields()) {
    if (covered_field) {
      route_weight += 1;
    }
  }
  return static_cast<double>(route_weight) / coverageFields().size();
}

void Coverage::markClusterCovered(const Envoy::Router::RouteEntry& route) {
  coveredRoute(route).setClusterCovered();
}

void Coverage::markVirtualClusterCovered(const Envoy::Router::RouteEntry& route) {
  coveredRoute(route).setVirtualClusterCovered();
}

void Coverage::markVirtualHostCovered(const Envoy::Router::RouteEntry& route) {
  coveredRoute(route).setVirtualHostCovered();
}

void Coverage::markPathRewriteCovered(const Envoy::Router::RouteEntry& route) {
  coveredRoute(route).setPathRewriteCovered();
}

void Coverage::markHostRewriteCovered(const Envoy::Router::RouteEntry& route) {
  coveredRoute(route).setHostRewriteCovered();
}

void Coverage::markRedirectPathCovered(const Envoy::Router::RouteEntry& route) {
  coveredRoute(route).setRedirectPathCovered();
}

double Coverage::report() {
  uint64_t num_routes = 0;
  for (const auto& host : route_config_.virtual_hosts()) {
    for (const auto& route : host.routes()) {
      if (route.route().has_weighted_clusters()) {
        num_routes += route.route().weighted_clusters().clusters_size();
      } else {
        num_routes += 1;
      }
    }
  }
  return 100 * static_cast<double>(covered_routes_.size()) / num_routes;
}

void Coverage::printMissingTests(const std::set<std::string>& all_route_names,
                                 const std::set<std::string>& covered_route_names) {
  std::set<std::string> missing_route_names;
  std::set_difference(all_route_names.begin(), all_route_names.end(), covered_route_names.begin(),
                      covered_route_names.end(),
                      std::inserter(missing_route_names, missing_route_names.end()));
  for (const auto& host : route_config_.virtual_hosts()) {
    for (const auto& route : host.routes()) {
      if (missing_route_names.find(route.name()) != missing_route_names.end()) {
        std::cout << "Missing test for host: " << host.name()
                  << ", route: " << route.match().DebugString() << std::endl;
      }
    }
  }
}

double Coverage::detailedReport() {
  std::set<std::string> all_route_names;
  std::set<std::string> covered_route_names;
  uint64_t num_routes = 0;
  for (const auto& host : route_config_.virtual_hosts()) {
    for (const auto& route : host.routes()) {
      if (route.route().has_weighted_clusters()) {
        num_routes += route.route().weighted_clusters().clusters_size();
      } else {
        num_routes += 1;
      }
      all_route_names.emplace(route.name());
    }
  }
  double cumulative_coverage = 0;
  for (auto& covered_route : covered_routes_) {
    cumulative_coverage += covered_route->report();
    covered_route_names.emplace(covered_route->route().routeName());
  }
  printMissingTests(all_route_names, covered_route_names);
  return 100 * cumulative_coverage / num_routes;
}

RouteCoverage& Coverage::coveredRoute(const Envoy::Router::RouteEntry& route) {
  for (auto& route_coverage : covered_routes_) {
    if (route_coverage->covers(&route)) {
      return *route_coverage;
    }
  }
  std::unique_ptr<RouteCoverage> new_coverage = std::make_unique<RouteCoverage>(&route);
  covered_routes_.push_back(std::move(new_coverage));
  return coveredRoute(route);
};
} // namespace Envoy
