#pragma once

#include "router.h"
#include "domain.h"
#include "request_handler.h"

#include <memory>
#include <optional>
#include <variant>
#include <unordered_map>
#include <string_view>

namespace transport_router {
	using Time = double;

	struct RoutingSetting {
		int bus_wait = 0;
		double bus_velocity = 0;
	};

	struct VertexInfo {
		graph::VertexId in;
		graph::VertexId out;
	};
	
	struct EdgeBus {
		const Route* route = nullptr;
		Time time = 0;
		int span_count = 0;
	};

	struct EdgeWait {
		const BusStop* stop = nullptr;
		Time time = 0;
	};

	using EdgeInfo = std::variant<EdgeBus, EdgeWait>;

	struct InfoBuildRoute {
		std::vector<EdgeInfo> route;
		Time total_weight = 0;
	};

	class TransportRouter {
	public:
		TransportRouter(const TransportCatalogue& catalogue);

		void SetSetting(RoutingSetting settings);
		void Initialization();
		std::optional<InfoBuildRoute> BuildRoute(std::string_view from, std::string_view  to) const;

	private:
		const TransportCatalogue& catalogue_;
		RoutingSetting settings_;
		std::unique_ptr<graph::Router<Time>> router_;

		graph::DirectedWeightedGraph<Time> map_;
		std::vector<graph::Edge<Time>> edges_;

		std::unordered_map<const BusStop*, VertexInfo> ref_vertex_;
		std::unordered_map<graph::EdgeId, EdgeInfo> ref_edge_;

	private:
		void CreateEdges(const std::vector<const BusStop*>& stops);
		void CreateWaitEdges(const std::vector<const BusStop*>& stops);
		void CreateBusEdges();
		Time DistToTime(size_t dist) const;
	};
} // namespace transport_router