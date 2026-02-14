#include "json_reader.h"

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

#include "geo.h"
#include "json_builder.h"

namespace json_reader {
	using namespace json;
	using namespace request_handler;
	namespace detail {
		BusRequest ParseBus(const Dict& req) {
			BusRequest bus_req{
					.name = req.at("name").AsString(),
					.stops{},
					.is_roundtrip = req.at("is_roundtrip").AsBool()
			};
			const Array& stops = req.at("stops").AsArray();
			for (const Node& stop : stops) {
				bus_req.stops.emplace_back(stop.AsString());
			}
			return bus_req;
		}

		StopRequest ParseStop(const Dict& req) {
			StopRequest stop_req{
				.name = req.at("name").AsString(),
				.pos{
					.lat = req.at("latitude").AsDouble(),
					.lng = req.at("longitude").AsDouble()
				},
				.road_distances{}
			};
			const Dict& road_distances = req.at("road_distances").AsMap();
			for (const auto& [name, dist] : road_distances) {
				stop_req.road_distances.emplace(name, dist.AsInt());
			}
			return stop_req;
		}

		StatRequest ParseStat(const Dict& req) {
			StatRequest stat_req{
				.id = req.at("id").AsInt(),
				.type = req.at("type").AsString(),
				.name = req.count("name") ? req.at("name").AsString() : "",
				.from = req.count("from") ? req.at("from").AsString() : "",
				.to = req.count("to") ? req.at("to").AsString() : ""
			};
			return stat_req;
		}

		Node ParseInfo(int id, const Info& info) {
			using namespace std::literals;
			if (std::holds_alternative<std::monostate>(info)) {
				return Builder{}.StartDict()
					.Key("request_id"s).Value(id)
					.Key("error_message"s).Value("not found"s)
					.EndDict().Build();
			}

			if (std::holds_alternative<InfoStop>(info)) {
				InfoStop info_stop = get<InfoStop>(info);
				Array arr;
				for (auto bus : info_stop.cross_references) {
					arr.emplace_back(std::string{ bus });
				}
				return Builder{}.StartDict()
					.Key("request_id"s).Value(id)
					.Key("buses"s).Value(arr)
					.EndDict().Build();
			}

			if (std::holds_alternative<InfoRoute>(info)) {
				InfoRoute info_route = get<InfoRoute>(info);
				return Builder{}.StartDict()
					.Key("request_id"s).Value(id)
					.Key("curvature"s).Value(info_route.curvature)
					.Key("route_length"s).Value(info_route.route_length)
					.Key("stop_count"s).Value(info_route.number_total)
					.Key("unique_stop_count"s).Value(info_route.number_unique)
					.EndDict().Build();
			}
			return Node();
		}

		Node ParseRoute(int id, const std::optional<transport_router::InfoBuildRoute>& build_route) {
			using namespace std::literals;
			using namespace transport_router;
			if (!build_route) {
				return Builder{}.StartDict()
					.Key("request_id"s).Value(id)
					.Key("error_message"s).Value("not found"s)
					.EndDict().Build();
			}

			Array arr;
			for (const auto& edge : build_route.value().route) {
				if (std::holds_alternative<EdgeWait>(edge)) {
					EdgeWait e_wait = get<EdgeWait>(edge);
					arr.push_back(
						Builder{}.StartDict()
						.Key("type").Value("Wait")
						.Key("stop_name").Value(e_wait.stop->name)
						.Key("time").Value(e_wait.time)
						.EndDict().Build()
					);
				}

				if (std::holds_alternative<EdgeBus>(edge)) {
					EdgeBus e_bus = get<EdgeBus>(edge);
					arr.push_back(
						Builder{}.StartDict()
						.Key("type").Value("Bus")
						.Key("bus").Value(e_bus.route->name)
						.Key("span_count").Value(e_bus.span_count)
						.Key("time").Value(e_bus.time)
						.EndDict().Build()
					);
				}
			}

			return Builder{}.StartDict()
				.Key("request_id"s).Value(id)
				.Key("items").Value(std::move(arr))
				.Key("total_time").Value(build_route.value().total_weight)
				.EndDict().Build();
		}

		Node GetMap(const StatRequest& req, const map_renderer::Renderer& renderer) {
			std::ostringstream ss;
			renderer.Drawing(ss);
			return Builder{}.StartDict()
				.Key("request_id").Value(req.id)
				.Key("map").Value(ss.str())
				.EndDict().Build();
		}

		svg::Color ParseColor(const Node& node) {
			if (node.IsString()) {
				return node.AsString();
			}

			if (node.IsArray()) {
				const Array& arr = node.AsArray();
				if (arr.size() == 3) {
					return svg::Rgb(
						static_cast<uint8_t>(arr[0].AsInt()),
						static_cast<uint8_t>(arr[1].AsInt()),
						static_cast<uint8_t>(arr[2].AsInt())
					);
				}
				if (arr.size() == 4) {
					return svg::Rgba(
						static_cast<uint8_t>(arr[0].AsInt()),
						static_cast<uint8_t>(arr[1].AsInt()),
						static_cast<uint8_t>(arr[2].AsInt()),
						arr[3].AsDouble()
					);
				}
			}

			return {};
		}
	} //namespace detail

	void Reader::LoadDoc(std::istream& is) {
		in_doc_ = json::Load(is);
	}

	void Reader::PrintDoc(std::ostream& os) {
		json::Print(out_doc_, os);
	}

	void Reader::FillCatalogue(TransportCatalogue& catalogue) {
		request_handler::FillCatalogue(ParseBaseRequest(), catalogue);
	}

	void Reader::GetData(const TransportCatalogue& catalogue, const map_renderer::Renderer& renderer, const transport_router::TransportRouter& router) {
		Array arr;
		std::vector<StatRequest> stat_requsetes = ParseStatRequest();
		for (const StatRequest& req : stat_requsetes) {
			if (req.type == "Map") {
				arr.push_back(detail::GetMap(req, renderer));
				continue;
			}

			if (req.type == "Route") {
				auto route = router.BuildRoute(req.from, req.to);
				arr.push_back(detail::ParseRoute(req.id, route));
				continue;
			}

			Info info = request_handler::GetInfo(req, catalogue);
			arr.push_back(detail::ParseInfo(req.id, info));
		}

		out_doc_ = Document{ arr };
	}

	void Reader::SetSettingRenderer(map_renderer::Renderer& renderer) {
		renderer.SetSetting(ParseRenderSetting());
	}

	void Reader::SetSettingRouter(transport_router::TransportRouter& router) {
		router.SetSetting(ParseRouterSetting());
	}

	std::deque<InData> Reader::ParseBaseRequest() {
		std::deque<InData> result;
		const Dict& objects = in_doc_.GetRoot().AsMap();
		const Array& base_request = objects.at("base_requests").AsArray();

		for (const Node& node : base_request) {
			const Dict& req = node.AsMap();
			// Остановки добавляются в начало дека для приоритетной обработки
			if (req.at("type").AsString() == "Stop") {
				result.emplace_front(detail::ParseStop(req));
			}
			// Маршруты добавляются без приоритета
			if (req.at("type").AsString() == "Bus") {
				result.emplace_back(detail::ParseBus(req));
			}
		}
		return result;
	}

	std::vector<StatRequest> Reader::ParseStatRequest() {
		std::vector<StatRequest> result;
		const Dict& objects = in_doc_.GetRoot().AsMap();
		const Array& stat_request = objects.at("stat_requests").AsArray();

		for (const Node& node : stat_request) {
			const Dict& req = node.AsMap();
			result.emplace_back(detail::ParseStat(req));
		}
		return result;
	}

	map_renderer::RenderSetting Reader::ParseRenderSetting() {
		const Dict& objects = in_doc_.GetRoot().AsMap();
		const Dict& settings = objects.at("render_settings").AsMap();
		map_renderer::RenderSetting result{
			.width = settings.at("width").AsDouble(),
			.height = settings.at("height").AsDouble(),
			.padding = settings.at("padding").AsDouble(),
			.line_width = settings.at("line_width").AsDouble(),
			.stop_radius = settings.at("stop_radius").AsDouble(),
			.bus_label_font_size = settings.at("bus_label_font_size").AsInt(),
			.stop_label_font_size = settings.at("stop_label_font_size").AsInt(),
			.bus_label_offset{},
			.stop_label_offset{},
			.underlayer_color{},
			.underlayer_width = settings.at("underlayer_width").AsDouble(),
			.color_palette{}
		};
		const Array& bus_offset = settings.at("bus_label_offset").AsArray();
		const Array& stop_offset = settings.at("stop_label_offset").AsArray();
		const Node& under_layer_color = settings.at("underlayer_color");
		const Array& color_palette = settings.at("color_palette").AsArray();

		result.bus_label_offset = svg::Point{ bus_offset[0].AsDouble(), bus_offset[1].AsDouble() };
		result.stop_label_offset = svg::Point{ stop_offset[0].AsDouble(), stop_offset[1].AsDouble() };
		result.underlayer_color = detail::ParseColor(under_layer_color);

		for (const Node& node : color_palette) {
			result.color_palette.emplace_back(detail::ParseColor(node));
		}

		return result;
	}

	transport_router::RoutingSetting Reader::ParseRouterSetting() {
		const Dict& objects = in_doc_.GetRoot().AsMap();
		const Dict& setting = objects.at("routing_settings").AsMap();
		transport_router::RoutingSetting result{
			.bus_wait = setting.at("bus_wait_time").AsInt(),
			.bus_velocity = setting.at("bus_velocity").AsDouble()
		};
		return result;
	}
} //namespace json_reader