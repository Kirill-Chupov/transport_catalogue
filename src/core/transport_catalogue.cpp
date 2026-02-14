#include "transport_catalogue.h"
#include <unordered_set>
#include <algorithm>
#include <cassert>

// Регистрирует новую остановку в системе
bool TransportCatalogue::AddStop(BusStop stop) {
	if (stop.empty()) {
		return false;
	}
	stops_.push_back(std::move(stop));
	ref_stops_.emplace(stops_.back().name, &stops_.back());
	return true;
}

// Регистрирует новый маршрут в системе
bool TransportCatalogue::AddRoute(Route route) {
	if (route.empty()) {
		return false;
	}
	routes_.push_back(std::move(route));
	ref_routes_.emplace(routes_.back().name, &routes_.back());
	return true;
}

// Предоставляет доступ к маршруту по имени
const Route* TransportCatalogue::GetRoute(std::string_view name) const {
	return ref_routes_.find(name) != ref_routes_.end() ? ref_routes_.at(name) : nullptr;
}

// Предоставляет доступ к остановке по имени
const BusStop* TransportCatalogue::GetStop(std::string_view name) const {
	return ref_stops_.find(name) != ref_stops_.end() ? ref_stops_.at(name) : nullptr;
}

// Сохраняет расстояние от from к to
void TransportCatalogue::SetStopsDistance(const BusStopPair& p, size_t distance) {
	assert(p.first && p.second); //Должны существовать обе остановки.

	distance_to_neighbor_.emplace(p, distance);
}

// Возвращает расстояние от from к to
std::optional<size_t> TransportCatalogue::GetStopsDistance(const BusStopPair& p, bool bidirectional) const {
	if (p.first == nullptr || p.second == nullptr) {
		return std::nullopt;
	}

	if (distance_to_neighbor_.contains(p)) {
		return distance_to_neighbor_.at(p);
	}

	if (distance_to_neighbor_.contains({ p.second, p.first }) && bidirectional) {
		return distance_to_neighbor_.at({ p.second, p.first });
	}

	return std::nullopt;
}

// Формирует статистику маршрута для отчетов
std::optional<InfoRoute> TransportCatalogue::GetInfoRoute(std::string_view name) const {
	const Route* route_ptr = GetRoute(name);
	if (route_ptr == nullptr) {
		return std::nullopt;
	}

	InfoRoute info;
	std::unordered_set<const BusStop*> unique_stops{ route_ptr->driving_route.begin(), route_ptr->driving_route.end() };
	info.name = route_ptr->name;
	info.number_total = static_cast<int>(route_ptr->driving_route.size());
	info.number_unique = static_cast<int>(unique_stops.size());

	double lenght_shortest = 0;
	for (auto i = 1; i < info.number_total; ++i) {
		const BusStop* from = route_ptr->driving_route[i - 1];
		const BusStop* to = route_ptr->driving_route[i];
		lenght_shortest += ComputeDistance(from->geo_point, to->geo_point);
	}

	for (auto i = 1; i < info.number_total; ++i) {
		const BusStop* from = route_ptr->driving_route[i - 1];
		const BusStop* to = route_ptr->driving_route[i];
		auto temp = GetStopsDistance({ from, to });
		assert(temp.has_value());
		info.route_length += static_cast<double>(temp.value());
	}
	info.curvature = info.route_length / lenght_shortest;

	return info;
}

// Формирует список маршрутов через указанную остановку
std::optional<InfoStop> TransportCatalogue::GetInfoStop(std::string_view name) const {
	const BusStop* stop_ptr = GetStop(name);
	if (stop_ptr == nullptr) {
		return std::nullopt;
	}

	InfoStop info;
	info.name = stop_ptr->name;
	for (auto [name_route, route_ptr] : ref_routes_) {
		auto it = std::ranges::find(route_ptr->driving_route, stop_ptr);
		if (it != route_ptr->driving_route.end()) {
			info.cross_references.push_back(name_route);
		}
	}
	std::ranges::sort(info.cross_references);
	return info;
}

const std::deque<Route>& TransportCatalogue::GetRoutes() const {
	return routes_;
}
