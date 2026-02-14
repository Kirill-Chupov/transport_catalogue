#include "map_renderer.h"

#include <algorithm>


namespace map_renderer {
	bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	class SphereProjector {
	public:
		// points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
		template <typename PointInputIt>
		SphereProjector(PointInputIt points_begin, PointInputIt points_end,
			double max_width, double max_height, double padding)
			: padding_(padding) {
			// Если точки поверхности сферы не заданы, вычислять нечего
			if (points_begin == points_end) {
				return;
			}

			// Находим точки с минимальной и максимальной долготой
			const auto [left_it, right_it] = std::minmax_element(
				points_begin, points_end,
				[](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
			min_lon_ = left_it->lng;
			const double max_lon = right_it->lng;

			// Находим точки с минимальной и максимальной широтой
			const auto [bottom_it, top_it] = std::minmax_element(
				points_begin, points_end,
				[](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
			const double min_lat = bottom_it->lat;
			max_lat_ = top_it->lat;

			// Вычисляем коэффициент масштабирования вдоль координаты x
			std::optional<double> width_zoom;
			if (!IsZero(max_lon - min_lon_)) {
				width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
			}

			// Вычисляем коэффициент масштабирования вдоль координаты y
			std::optional<double> height_zoom;
			if (!IsZero(max_lat_ - min_lat)) {
				height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
			}

			if (width_zoom && height_zoom) {
				// Коэффициенты масштабирования по ширине и высоте ненулевые,
				// берём минимальный из них
				zoom_coeff_ = std::min(*width_zoom, *height_zoom);
			} else if (width_zoom) {
				// Коэффициент масштабирования по ширине ненулевой, используем его
				zoom_coeff_ = *width_zoom;
			} else if (height_zoom) {
				// Коэффициент масштабирования по высоте ненулевой, используем его
				zoom_coeff_ = *height_zoom;
			}
		}

		// Проецирует широту и долготу в координаты внутри SVG-изображения
		svg::Point operator()(geo::Coordinates coords) const {
			return {
				(coords.lng - min_lon_) * zoom_coeff_ + padding_,
				(max_lat_ - coords.lat) * zoom_coeff_ + padding_
			};
		}

	private:
		double padding_;
		double min_lon_ = 0;
		double max_lat_ = 0;
		double zoom_coeff_ = 0;
	};

	namespace detail {
		SphereProjector SetProjector(const std::vector<const BusStop*>& stops, const RenderSetting& setting) {
			std::vector<geo::Coordinates> coordinates;
			for (const BusStop* stop : stops) {
				coordinates.push_back(stop->geo_point);
			}

			return SphereProjector (coordinates.begin(), coordinates.end(), setting.width, setting.height, setting.padding);
		}

		void AddNameRoute(svg::Document& image, const std::string& name, const svg::Point& pos, const svg::Color& color, const RenderSetting& setting) {
			svg::Text text = svg::Text()
				.SetData(name)
				.SetPosition(pos)
				.SetOffset(setting.bus_label_offset)
				.SetFontSize(setting.bus_label_font_size)
				.SetFontFamily("Verdana")
				.SetFontWeight("bold")
				.SetFillColor(color);

			svg::Text underlayer = svg::Text()
				.SetData(name)
				.SetPosition(pos)
				.SetOffset(setting.bus_label_offset)
				.SetFontSize(setting.bus_label_font_size)
				.SetFontFamily("Verdana")
				.SetFontWeight("bold")
				.SetFillColor(setting.underlayer_color)
				.SetStrokeColor(setting.underlayer_color)
				.SetStrokeWidth(setting.underlayer_width)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

			image.Add(std::move(underlayer));
			image.Add(std::move(text));
		}

		void AddNameStop(svg::Document& image, const std::string& name, const svg::Point& pos, const RenderSetting& setting) {
			svg::Text text = svg::Text()
				.SetData(name)
				.SetPosition(pos)
				.SetOffset(setting.stop_label_offset)
				.SetFontSize(setting.stop_label_font_size)
				.SetFontFamily("Verdana")
				.SetFillColor("black");

			svg::Text underlayer = svg::Text()
				.SetData(name)
				.SetPosition(pos)
				.SetOffset(setting.stop_label_offset)
				.SetFontSize(setting.stop_label_font_size)
				.SetFontFamily("Verdana")
				.SetFillColor(setting.underlayer_color)
				.SetStrokeColor(setting.underlayer_color)
				.SetStrokeWidth(setting.underlayer_width)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

			image.Add(std::move(underlayer));
			image.Add(std::move(text));
		}

		std::vector<const BusStop*> GetUniqueSortStops(const std::vector<Route>& routes) {
			std::vector<const BusStop*> stops;
			for (const Route& route : routes) {
				for (const BusStop* stop : route.driving_route) {
					stops.push_back(stop);
				}
			}
			
			auto comparator_less = [](const BusStop* a, const BusStop* b) {return a->name < b->name; };
			std::ranges::sort(stops, comparator_less);
			auto comparator_equal = [](const BusStop* a, const BusStop* b) {return a->name == b->name; };
			auto [it_del, last] = std::ranges::unique(stops, comparator_equal);
			stops.erase(it_del, last);

			return stops;
		}
		
	} // namespace detail

	void Renderer::SetSetting(const RenderSetting& setting) {
		setting_ = setting;
	}

	void Renderer::CreateMap(const TransportCatalogue& catalogue) {
		std::vector<Route> routes = request_handler::GetRoutes(catalogue);
		CreateMap(routes);
	}

	void Renderer::Drawing(std::ostream& os) const {
		image_.Render(os);
	}

	void Renderer::CreateMap(const std::vector<Route>& routes) {
		std::vector<const BusStop*> stops = detail::GetUniqueSortStops(routes);
		SphereProjector projector = detail::SetProjector(stops, setting_);
		CreateLineRoute(routes, projector);
		CreateNameRoute(routes, projector);
		CreatePointStop(stops, projector);
		CreateNameStop(stops, projector);
	}

	void Renderer::CreateLineRoute(const std::vector<Route>& routes, const SphereProjector& projector) {
		current_color_ = 0;
		for (const Route& route : routes) {
			if (route.driving_route.empty()) {
				continue;
			}

			svg::Polyline line;
			for (const BusStop* stop : route.driving_route) {
				svg::Point p = projector(stop->geo_point);
				line.AddPoint(p);
			}

			image_.Add(line
				.SetStrokeColor(GetColor())
				.SetFillColor(svg::NoneColor)
				.SetStrokeWidth(setting_.line_width)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
			);
		}
	}

	void Renderer::CreateNameRoute(const std::vector<Route>& routes, const SphereProjector& projector) {
		current_color_ = 0;
		for (const Route& route : routes) {
			const std::vector<const BusStop*>& driving_route = route.driving_route;
			if (driving_route.empty()) {
				continue;
			}

			
			svg::Point pos = projector(driving_route[0]->geo_point);
			const std::string& name = route.name;
			const svg::Color& color = GetColor();

			detail::AddNameRoute(image_, name, pos, color, setting_);
			
			if (!route.round_trip) {
				size_t index = driving_route.size() / 2;
				if (driving_route[0] != driving_route[index]) {
					pos = projector(driving_route[index]->geo_point);
					detail::AddNameRoute(image_, name, pos, color, setting_);
				}
			}
		}
	}

	void Renderer::CreatePointStop(const std::vector<const BusStop*> stops, const SphereProjector& projector) {
		for (const BusStop* stop : stops) {
			svg::Point pos = projector(stop->geo_point);
			image_.Add(svg::Circle().SetCenter(pos).SetRadius(setting_.stop_radius).SetFillColor("white"));
		}
	}
	
	void Renderer::CreateNameStop(const std::vector<const BusStop*> stops, const SphereProjector& projector) {
		for (const BusStop* stop : stops) {
			svg::Point pos = projector(stop->geo_point);
			detail::AddNameStop(image_, stop->name, pos, setting_);
		}
	}

	svg::Color Renderer::GetColor() {
		svg::Color color = setting_.color_palette[current_color_];
		++current_color_;
		current_color_ = current_color_ >= setting_.color_palette.size() ? 0 : current_color_;
		return color;
	}
} // namespace map_render