#pragma once

#include <vector>
#include <deque>

#include "svg.h"
#include "geo.h"
#include "domain.h"

#include "request_handler.h"

namespace map_renderer {
    inline const double EPSILON = 1e-6;
    bool IsZero(double value);

    class SphereProjector;

    struct RenderSetting {
        double width = 0;
        double height = 0;
        double padding = 0;
        double line_width = 0;
        double stop_radius = 0;
        int bus_label_font_size = 0;
        int stop_label_font_size = 0;
        svg::Point bus_label_offset;
        svg::Point stop_label_offset;
        svg::Color underlayer_color;
        double underlayer_width = 0;
        std::vector<svg::Color> color_palette;
    };

    class Renderer {
    public:
        void SetSetting(const RenderSetting& setting);
        void CreateMap(const TransportCatalogue& catalogue);
        void Drawing(std::ostream& os) const;
    private:
        RenderSetting setting_;
        svg::Document image_;
        size_t current_color_ = 0;
    private:
        void CreateMap(const std::vector<Route>& routes);
        void CreateLineRoute(const std::vector<Route>& routes, const SphereProjector& projector);
        void CreateNameRoute(const std::vector<Route>& routes, const SphereProjector& projector);
        void CreatePointStop(const std::vector<const BusStop*> stops, const SphereProjector& projector);
        void CreateNameStop(const std::vector<const BusStop*> stops, const SphereProjector& projector);
        svg::Color GetColor();
    };
} // namespace map_render
