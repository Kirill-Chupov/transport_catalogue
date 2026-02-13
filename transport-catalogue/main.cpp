#include <iostream>
#include <string>

#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "transport_router.h"

using namespace std;

int main() {
	TransportCatalogue catalogue;
	json_reader::Reader reader;
	map_renderer::Renderer renderer;
	transport_router::TransportRouter router(catalogue);
	reader.LoadDoc(std::cin);
	reader.FillCatalogue(catalogue);
	reader.SetSettingRenderer(renderer);
	reader.SetSettingRouter(router);
	router.Initialization();
	renderer.CreateMap(catalogue);	
	reader.GetData(catalogue, renderer, router);
	reader.PrintDoc(std::cout);
}