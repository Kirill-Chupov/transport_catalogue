#pragma once

#include <vector>
#include <deque>

#include "json.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace json_reader {
	class Reader {
	public:
		void LoadDoc(std::istream& is);
		void PrintDoc(std::ostream& os);
		// Инициализация каталога
		void FillCatalogue(TransportCatalogue& catalogue);
		// Генерация ответа на stat_request
		void GetData(const TransportCatalogue& catalogue, const map_renderer::Renderer& renderer, const transport_router::TransportRouter& router);
		// Применение render_setting к Renderer
		void SetSettingRenderer(map_renderer::Renderer& renderer);
		// Применение router_setting к TransportRouter
		void SetSettingRouter(transport_router::TransportRouter& router);

	private:
		json::Document in_doc_{ json::Node{} };
		json::Document out_doc_{ json::Node{} };

	private:
		// Вспомогательные функции парсинга
		std::deque<request_handler::InData> ParseBaseRequest();
		std::vector<request_handler::StatRequest> ParseStatRequest();
		map_renderer::RenderSetting ParseRenderSetting();
		transport_router::RoutingSetting ParseRouterSetting();
	};
}