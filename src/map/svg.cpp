#include "svg.h"
#include <format>
#include <ostream>

namespace svg {

	using namespace std::literals;

	struct ColorPrinter {
		std::string operator()(std::monostate) const {
			return "none";
		}

		std::string operator()(const std::string& str) const {
			return str;
		}

		std::string operator()(const svg::Rgb& rgb) const {
			return std::format(R"(rgb({},{},{}))",
				rgb.red,
				rgb.green,
				rgb.blue
			);
		}

		std::string operator()(const svg::Rgba& rgba) const {
			return std::format(R"(rgba({},{},{},{:.6g}))",
				rgba.red,
				rgba.green,
				rgba.blue,
				rgba.opacity
			);
		}
	};

	std::ostream& operator<<(std::ostream& out, Color color) {
		out << std::visit(ColorPrinter{}, color);
		return out;
	}

	std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
		switch (line_cap) {
		case svg::StrokeLineCap::BUTT:
			out << "butt"sv;
			break;
		case svg::StrokeLineCap::ROUND:
			out << "round"sv;
			break;
		case svg::StrokeLineCap::SQUARE:
			out << "square"sv;
			break;
		default:
			break;
		}
		return out;
	}

	std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
		switch (line_join) {
		case svg::StrokeLineJoin::ARCS:
			out << "arcs"sv;
			break;
		case svg::StrokeLineJoin::BEVEL:
			out << "bevel"sv;
			break;
		case svg::StrokeLineJoin::MITER:
			out << "miter"sv;
			break;
		case svg::StrokeLineJoin::MITER_CLIP:
			out << "miter-clip"sv;
			break;
		case svg::StrokeLineJoin::ROUND:
			out << "round"sv;
			break;
		default:
			break;
		}
		return out;
	}

	void Object::Render(const RenderContext& context) const {
		context.RenderIndent();

		// Делегируем вывод тега своим подклассам
		RenderObject(context);

		context.out << std::endl;
	}

	// ---------- Circle ------------------

	Circle& Circle::SetCenter(Point center) {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius) {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const {
		std::string attrs = RenderAttrs();
		context.out << std::format(R"(<circle cx="{:.6g}" cy="{:.6g}" r="{:.6g}"{}/>)",
			center_.x,
			center_.y,
			radius_,
			attrs.empty() ? " " : attrs
		);
	}

	// ---------- Polyline ------------------

	Polyline& Polyline::AddPoint(Point point) {
		vertex_points_.push_back(point);
		return *this;
	}

	void Polyline::RenderObject(const RenderContext& context) const {
		bool first_interation = true;
		std::string points = "";
		for (const auto& point : vertex_points_) {
			points += first_interation ? "" : " ";
			points += std::format("{:.6g},{:.6g}",
				point.x,
				point.y
			);
			first_interation = false;
		}

		std::string attrs = RenderAttrs();
		context.out << std::format(R"(<polyline points="{}"{}/>)",
			std::move(points),
			attrs.empty() ? " " : attrs
		);
	}

	// ---------- Text ------------------

	Text& Text::SetPosition(Point pos) {
		pos_ = pos;
		return *this;
	}

	Text& Text::SetOffset(Point offset) {
		offset_ = offset;
		return *this;
	}

	Text& Text::SetFontSize(uint32_t size) {
		size_ = size;
		return *this;
	}

	Text& Text::SetFontFamily(std::string font_family) {
		font_family_ = std::move(font_family);
		return *this;
	}

	Text& Text::SetFontWeight(std::string font_weight) {
		font_weight_ = std::move(font_weight);
		return *this;
	}

	Text& Text::SetData(std::string data) {
		data_ = std::move(data);
		return *this;
	}

	void Text::RenderObject(const RenderContext& context) const {
		std::string font_family = font_family_.empty() ? "" : std::format(R"(font-family="{}")", font_family_);
		std::string font_weight = font_weight_.empty() ? "" : std::format(R"(font-weight="{}")", font_weight_);
		std::string delimiter = !font_family.empty() && !font_weight.empty() ? " " : "";
		std::string attrs = RenderAttrs();
		context.out << std::format(R"(<text{}x="{:.6g}" y="{:.6g}" dx="{:.6g}" dy="{:.6g}" font-size="{}" {}{}{}>{}</text>)",
			attrs.empty() ? " " : attrs + " ",
			pos_.x,
			pos_.y,
			offset_.x,
			offset_.y,
			size_,
			font_family,
			delimiter,
			font_weight,
			data_
		);
	}

	// ---------- Document ------------------

	void Document::AddPtr(std::unique_ptr<Object>&& obj) {
		data_.emplace_back(std::move(obj));
	}

	void Document::Render(std::ostream& out) const {
		out << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << '\n';
		out << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)" << '\n';
		for (const auto& obj_ptr : data_) {
			obj_ptr->Render({ out, 2 , 2 });
		}
		out << R"(</svg>)";
	}

}  // namespace svg