#include "request_handler.h"

#include <cassert>


namespace request_handler {
	using Neighbors = std::unordered_map< std::string, std::unordered_map<std::string, size_t>>;
	namespace detail {
		void AddStop(const InData& req, Neighbors& neighbors, TransportCatalogue& catalogue) {
			const StopRequest& cur_req = std::get<StopRequest>(req);
			neighbors.emplace(cur_req.name, cur_req.road_distances);
			catalogue.AddStop({ .name = cur_req.name, .geo_point = cur_req.pos });
		}

		void AddRoute(const InData& req, TransportCatalogue& catalogue) {
			const BusRequest& cur_req = std::get<BusRequest>(req);
			std::vector<const BusStop*> stops;
			for (const std::string& stop : cur_req.stops) {
				const BusStop* stop_ptr = catalogue.GetStop(stop);
				assert(stop_ptr != nullptr);
				stops.push_back(stop_ptr);
			}

			if (!stops.empty() && !cur_req.is_roundtrip) {
				std::vector<const BusStop*>temp{ std::next(stops.rbegin()), stops.rend() };
				for (auto elem : temp) {
					stops.push_back(elem);
				}
			}

			catalogue.AddRoute({ .name = cur_req.name, .driving_route = std::move(stops), .round_trip = cur_req.is_roundtrip });
		}

		void SetStopsDistance(const Neighbors& neighbors, TransportCatalogue& catalogue) {
			for (auto [from, map_dist] : neighbors) {
				const BusStop* from_ptr = catalogue.GetStop(from);
				for (auto [to, dist] : map_dist) {
					const BusStop* to_ptr = catalogue.GetStop(to);
					assert(from_ptr != nullptr && to_ptr != nullptr);
					catalogue.SetStopsDistance({ from_ptr, to_ptr }, dist);
				}
			}
		}

		Info GetInfoStop(const StatRequest& req, const TransportCatalogue& catalogue) {
			std::optional<InfoStop> stop_info = catalogue.GetInfoStop(req.name);
			if (stop_info == std::nullopt) {
				return std::monostate();
			}
			return stop_info.value();
		}

		Info GetInfoRoute(const StatRequest& req, const TransportCatalogue& catalogue) {
			std::optional<InfoRoute> route_info = catalogue.GetInfoRoute(req.name);
			if (route_info == std::nullopt) {
				return std::monostate();
			}
			return route_info.value();
		}

	} // namespace deatail

	void FillCatalogue(const std::deque<InData>& base_req, TransportCatalogue& catalogue) {
		Neighbors neighbors;
		for (const InData& req : base_req) {
			if (std::holds_alternative<StopRequest>(req)) {
				detail::AddStop(req, neighbors, catalogue);
				continue;
			}

			if (std::holds_alternative<BusRequest>(req)) {
				detail::AddRoute(req, catalogue);
				continue;
			}
		}

		detail::SetStopsDistance(neighbors, catalogue);
	}

	Info GetInfo(const StatRequest& stat_req, const TransportCatalogue& catalogue) {
		if (stat_req.type == "Bus") {
			return detail::GetInfoRoute(stat_req, catalogue);
		}

		if (stat_req.type == "Stop") {
			return detail::GetInfoStop(stat_req, catalogue);
		}

		return std::monostate();
	}

	std::vector<Route> GetRoutes(const TransportCatalogue& catalogue) {
		const std::deque<Route>& routes = catalogue.GetRoutes();
		std::vector<Route> result{ routes.begin(), routes.end() };
		std::ranges::sort(result, [](const Route& a, const Route& b) {return a.name < b.name; });
		return result;
	}
	
} // namespace request_handler