#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <deque>

#include "transport_catalogue.h"


namespace request_handler {
	struct StopRequest {
		std::string name{};
		geo::Coordinates pos{};
		std::unordered_map<std::string, size_t> road_distances;
	};

	struct BusRequest {
		std::string name{};
		std::vector<std::string> stops;
		bool is_roundtrip = false;
	};

	struct StatRequest {
		int id = 0;
		std::string type{};
		std::string name{};

		std::string from{};
		std::string to{};
	};

	using InData = std::variant<std::monostate, StopRequest, BusRequest>;
	using Info = std::variant<std::monostate, InfoStop, InfoRoute>;

	void FillCatalogue(const std::deque<InData>& base_req, TransportCatalogue& catalogue);
	Info GetInfo(const StatRequest& stat_req, const TransportCatalogue& catalogue);
	std::vector<Route> GetRoutes(const TransportCatalogue& catalogue);
} // namespace request_handler
