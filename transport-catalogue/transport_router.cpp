#include "transport_router.h"

#include <deque>
#include <vector>
#include <algorithm>

namespace transport_router {
	using namespace graph;
	namespace detail {
		static std::vector<const BusStop*> GetUniqueStop(const std::deque<Route>& routes) {
			std::vector<const BusStop*> stops;
			for (const Route& route : routes) {
				std::ranges::copy(route.driving_route, std::back_inserter(stops));
			}
			std::ranges::sort(stops);
			auto [it_del, last] = std::ranges::unique(stops);
			stops.erase(it_del, last);
			return stops;
		}
	} // namespace detail

	TransportRouter::TransportRouter(const TransportCatalogue& catalogue)
		: catalogue_{ catalogue } {
	}

	void TransportRouter::SetSetting(RoutingSetting settings) {
		settings_ = std::move(settings);
	}

	void TransportRouter::Initialization() {
		const std::deque<Route>& routes = catalogue_.GetRoutes();
		auto unique_stops = detail::GetUniqueStop(routes);
		map_ = DirectedWeightedGraph<Time>(unique_stops.size() * 2);
		CreateEdges(unique_stops);
		for (const auto& edge : edges_) {
			map_.AddEdge(edge);
		}

		router_ = std::make_unique<Router<Time>>(map_);
	}

	std::optional<InfoBuildRoute> TransportRouter::BuildRoute(std::string_view from, std::string_view  to) const {
		assert(router_ != nullptr);
		InfoBuildRoute result;
		auto from_ptr = catalogue_.GetStop(from);
		auto to_ptr = catalogue_.GetStop(to);
		if (!ref_vertex_.contains(from_ptr) || !ref_vertex_.contains(to_ptr)) {
			return std::nullopt;
		}

		if (auto route = router_->BuildRoute(ref_vertex_.at(from_ptr).in, ref_vertex_.at(to_ptr).in)) {
			for (const auto& edgeid : route.value().edges) {
				result.route.push_back(ref_edge_.at(edgeid));
			}
			result.total_weight = route->weight;
		} else {
			return std::nullopt;
		}

		return result;
	}

	void TransportRouter::CreateEdges(const std::vector<const BusStop*>& stops) {
		edges_.clear();
		CreateWaitEdges(stops);
		CreateBusEdges();
	}

	void TransportRouter::CreateWaitEdges(const std::vector<const BusStop*>& stops) {
		VertexId counter = 0;
		for (const BusStop* from : stops) {
			VertexInfo stop_vertex{
				.in = counter++,
				.out = counter++
			};
			ref_vertex_[from] = stop_vertex;

			Edge<Time> wait{
				.from = stop_vertex.in,
				.to = stop_vertex.out,
				.weight = static_cast<Time>(settings_.bus_wait)
			};

			edges_.push_back(wait);
			ref_edge_[edges_.size() - 1] = EdgeWait{ .stop = from, .time = wait.weight };
		}
	}
	
	void TransportRouter::CreateBusEdges() {
		const auto& routes = catalogue_.GetRoutes();
		for (const auto& route : routes) {
			const auto& vec = route.driving_route;
			for (auto it_from = vec.begin(); it_from != vec.end(); ++it_from) {
				Time total_time = 0;
				auto prev = it_from;
				for (auto it_to = next(it_from); it_to != vec.end(); ++it_to) {
					BusStopPair edge{ *prev, *it_to };
					if (auto dist = catalogue_.GetStopsDistance(edge, !route.round_trip)) {
						total_time += DistToTime(dist.value());
					} else {
						continue;
					}
					prev = it_to;

					Edge<Time> bus{
						.from = ref_vertex_[*it_from].out,
						.to = ref_vertex_[*it_to].in,
						.weight = total_time,
					};
					edges_.push_back(bus);
					ref_edge_[edges_.size() - 1] = EdgeBus{
						.route = &route,
						.time = bus.weight,
						.span_count = static_cast<int>(std::distance(it_from, it_to))
					};
				}
			}
		}
	}

	Time TransportRouter::DistToTime(size_t dist) const {
		constexpr double MINUTES_PER_HOUR = 60;
		constexpr double METERS_PER_KILOMETER = 1000;
		return (dist * MINUTES_PER_HOUR) / (settings_.bus_velocity * METERS_PER_KILOMETER);
	}
} // namespace transport_router

