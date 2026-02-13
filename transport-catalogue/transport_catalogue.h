#pragma once

#include<string>
#include<string_view>
#include<deque>
#include<vector>
#include<unordered_map>
#include<optional>
#include<utility>
#include<cstddef>

#include "domain.h"

// Обеспечивает хранение и поиск данных транспортной сети
class TransportCatalogue {
	struct BusStopPairHasher {
		static constexpr size_t HASH_PRIME = 37;
		size_t operator()(const BusStopPair& p) const {
			return hasher(p.first) + hasher(p.second) * HASH_PRIME;
		}
		inline static std::hash<const BusStop*> hasher{};
	};

public:
	// Регистрирует новую остановку в системе
	bool AddStop(BusStop stop);

	// Регистрирует новый маршрут в системе
	bool AddRoute(Route route);

	// Предоставляет доступ к маршруту по имени
	[[nodiscard]] const Route* GetRoute(std::string_view name) const;

	// Предоставляет доступ к остановке по имени
	[[nodiscard]] const BusStop* GetStop(std::string_view name) const;

	// Сохраняет расстояние от from к to
	void SetStopsDistance(const BusStopPair& p, size_t distance);

	// Возвращает расстояние от from к to
	[[nodiscard]] std::optional<size_t> GetStopsDistance(const BusStopPair& p, bool bidirectional = true) const;

	// Формирует статистику маршрута для отчетов
	[[nodiscard]] std::optional<InfoRoute> GetInfoRoute(std::string_view name) const;

	// Формирует список маршрутов через указанную остановку
	[[nodiscard]] std::optional<InfoStop> GetInfoStop(std::string_view name) const;

	// Предоставляет информацию о существующих маршрутах
	[[nodiscard]] const std::deque<Route>& GetRoutes() const;

private:
	std::deque<BusStop> stops_;
	std::deque<Route> routes_;
	std::unordered_map<std::string_view, const BusStop*> ref_stops_;
	std::unordered_map<std::string_view, const Route*> ref_routes_;
	std::unordered_map<BusStopPair, size_t, BusStopPairHasher> distance_to_neighbor_;
};