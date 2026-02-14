#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "geo.h"

// Описывает остановку общественного транспорта
struct BusStop {
	std::string name;
	geo::Coordinates geo_point;

	bool empty() const {
		return name.empty();
	}
};

// Описывает маршрут движения транспорта между остановками
struct Route {
	std::string name;
	std::vector<const BusStop*> driving_route;
	bool round_trip = false;

	bool empty() const {
		return name.empty();
	}
};

// Содержит агрегированные статистические данные о маршруте
struct InfoRoute {
	std::string_view name;
	int number_total = 0;
	int number_unique = 0;
	double route_length = 0;
	double curvature = 0;
};

// Содержит информацию о маршрутах, проходящих через остановку
struct InfoStop {
	std::string_view name;
	std::vector<std::string_view> cross_references;
};

using BusStopPair = std::pair<const BusStop*, const BusStop*>; // {from, to}