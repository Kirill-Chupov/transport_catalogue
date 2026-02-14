#include "json.h"

#include <string_view>
#include <algorithm>
#include <sstream>

using namespace std;

namespace json {

	namespace detail {

		Node LoadNode(std::string_view input);
		std::string StringParsing(std::string_view str);
		std::string StringSerialization(const std::string& str);

		using Iterator = std::string_view::iterator;

		std::pair<std::string_view, Node> ParsePair(std::string_view input) {
			auto key_start = std::ranges::find(input, '\"');
			auto key_end = std::find(++key_start, input.end(), '\"');
			auto value_start = std::find(key_end, input.end(), ':') + 1;

			std::string_view key(&(*key_start), key_end - key_start);
			std::string_view value(&(*value_start), input.end() - value_start);

			return std::make_pair(key, LoadNode(value));
		}

		Iterator FindMatchingBracket(std::string_view input, Iterator start, char op_bracket, char cl_bracket) {
			int count = 1;
			auto it = start + 1;
			while (count > 0 && it != input.end()) {
				count += *it == op_bracket ? 1 : *it == cl_bracket ? -1 : 0;
				++it;
			}
			if (count != 0) {
				throw ParsingError("Unmatched bracket");
			}
			return it;
		}

		Iterator FindValueEnd(std::string_view input, Iterator start) {
			auto comma_pos = std::find(start, input.end(), ',');
			auto braсket_op = std::find_if(start, comma_pos, [](const auto& c) {return c == '[' || c == '{'; });
			if (braсket_op != comma_pos) {
				char cl_bracker = *braсket_op == '{' ? '}' : ']';
				return FindMatchingBracket(input, braсket_op, *braсket_op, cl_bracker);
			}
			return comma_pos;
		}

		Node LoadArray(std::string_view input) {
			Array result;

			auto start_pos = input.begin();
			while (true) {
				start_pos = std::find_if(start_pos, input.end(), [](const auto& c) {return c != ' ' && c != ','; });
				if (start_pos == input.end()) {
					break;
				}

				if (*start_pos == '{' || *start_pos == '[') {
					char cl_bracker = *start_pos == '{' ? '}' : ']';
					auto it = FindMatchingBracket(input, start_pos, *start_pos, cl_bracker);
					result.emplace_back(LoadNode({ start_pos, it }));
					start_pos = it == input.end() ? input.end() : ++it;
					continue;
				}

				auto comma_pos = std::find(start_pos, input.end(), ',');
				auto end_pos = comma_pos;

				std::string_view sub_str(&(*start_pos), end_pos - start_pos);
				result.emplace_back(LoadNode(sub_str));
				start_pos = end_pos;
			}
			return Node{ result };
		}

		Node LoadMap(std::string_view input) {
			Dict result;
			auto start_pos = input.begin();
			while (true) {
				start_pos = std::find_if(start_pos, input.end(), [](const auto& c) {return c != ' ' && c != ','; });
				if (start_pos == input.end()) {
					break;
				}

				if (*start_pos == '{' || *start_pos == '[') {
					char cl_bracker = *start_pos == '{' ? '}' : ']';
					auto it = FindMatchingBracket(input, start_pos, *start_pos, cl_bracker);
					result.emplace(ParsePair({ start_pos, it }));
					start_pos = it == input.end() ? input.end() : ++it;
					continue;
				}

				auto colon = std::find(start_pos, input.end(), ':');
				auto end_pos = FindValueEnd(input, next(colon));

				std::string_view sub_str(&(*start_pos), end_pos - start_pos);
				result.emplace(ParsePair(sub_str));
				start_pos = end_pos;
			}
			return Node{ result };
		}

		Node LoadString(std::string_view input) {
			return Node{ detail::StringParsing(input) };
		}

		Node LoadNumber(std::string_view input) {

			auto invalid_symbol = [](char c) {return !std::isdigit(c) && (c != '.' && c != '+' && c != '-' && c != 'e' && c != 'E'); };
			if (std::ranges::find_if(input, invalid_symbol) != input.end()) {
				throw(ParsingError("Invalid Number"));
			}

			std::string str{ input };

			double num_d = std::stod(str);
			int num_i = static_cast<int>(num_d);

			auto is_double = [](char c) {return c == '.' || c == 'e' || c == 'E'; };

			if (std::ranges::find_if(str, is_double) != str.end()) {
				return Node{ num_d };
			}

			return Node{ num_i };
		}

		Node LoadPrimitive(std::string_view input) {
			auto predicate = [](char c) {return c == ']' || c == '}'; };
			if (std::ranges::find_if(input, predicate) != input.end()) {
				throw(ParsingError("Invalid Primitive"));
			}

			if (input == "null") {
				return Node{ nullptr };
			}

			if (input == "true" || input == "false") {
				return input == "true" ? Node{ true } : Node{ false };
			}

			return LoadNumber(input);
		}

		Node LoadNode(std::string_view input) {
			if (input.empty()) {
				return {};
			}

			auto predicate = [](char c) {return c != '\n' && c != '\r' && c != '\t' && c != ' '; };
			auto it_first_symbol = std::find_if(input.begin(), input.end(), predicate);
			auto it_last_symbol = input.end();
			if (*it_first_symbol == '{') {
				auto rfound = std::find(input.rbegin(), input.rend(), '}');
				if (rfound == input.rend()) {
					throw(ParsingError("Invalid Dict"));
				}
				auto offset = std::distance(rfound, input.rbegin());
				it_last_symbol = it_last_symbol + offset;
				return LoadMap({ ++it_first_symbol, --it_last_symbol });
			}

			if (*it_first_symbol == '[') {
				auto rfound = std::find(input.rbegin(), input.rend(), ']');
				if (rfound == input.rend()) {
					throw(ParsingError("Invalid Array"));
				}
				auto offset = std::distance(rfound, input.rbegin());
				it_last_symbol = it_last_symbol + offset;
				return LoadArray({ ++it_first_symbol, --it_last_symbol });
			}

			if (*it_first_symbol == '\"') {
				auto rfound = std::find(input.rbegin(), input.rend(), '\"');
				auto offset = std::distance(rfound, input.rbegin());
				it_last_symbol = it_last_symbol + offset;
				if (++it_first_symbol > --it_last_symbol) {
					throw(ParsingError("Invalid String"));
				}
				return LoadString({ it_first_symbol, it_last_symbol });
			}

			auto offset = std::distance(std::find_if(input.rbegin(), input.rend(), predicate), input.rbegin());
			it_last_symbol = it_last_symbol + offset;

			return LoadPrimitive({ it_first_symbol, it_last_symbol });
		}

		std::string StringParsing(std::string_view input) {
			std::string str;
			for (auto it = input.begin(); it != input.end(); ++it) {
				if (*it == '\\' && std::next(it) != input.end()) {
					switch (*++it) {
					case 'r':  str += '\r'; break;
					case 'n':  str += '\n'; break;
					case 't':  str += '\t'; break;
					case '"':  str += '\"'; break;
					case '\\': str += '\\'; break;
					default:   str += '\\'; str += *it;  // недопустимый escape
					}
				} else {
					str += *it;
				}
			}
			return str;
		}

		std::string StringSerialization(const std::string& str) {
			std::string result;
			for (char c : str) {
				switch (c) {
				case '\r': result += "\\r"; break;
				case '\n': result += "\\n"; break;
				case '\t': result += "\\t"; break;
				case '\"': result += "\\\""; break;
				case '\\': result += "\\\\"; break;
				default:   result += c;
				}
			}
			return "\"" + result + "\"";
		}

		std::string MakeIndent(int level) {
			static const int indent = 4;
			return std::string(level * indent, ' ');
		}

		void PrintArray(const Node& node, std::ostream& output, int level = 0) {
			const Array& arr = node.AsArray();
			if (arr.empty()) {
				output << "[]";
				return;
			}

			output << "[\n";
			for (auto it = arr.begin(); it != arr.end(); ++it) {
				output << MakeIndent(level + 1);
				PrintNode(*it, output, level + 1);
				output << (it == std::prev(arr.end()) ? "\n" : ",\n");
			}
			output << MakeIndent(level) << "]";
		}

		void PrintMap(const Node& node, std::ostream& output, int level = 0) {
			const Dict& map = node.AsMap();
			if (map.empty()) {
				output << "{}";
				return;
			}

			output << "{\n";
			for (auto it = map.begin(); it != map.end(); ++it) {
				auto [key, value] = *it;
				output << MakeIndent(level + 1) << "\"" << key << "\": ";
				PrintNode(value, output, level + 1);
				output << (it == std::prev(map.end()) ? "\n" : ",\n");
			}
			output << MakeIndent(level) << "}";
		}

	}  // namespace detail

	//-----Node-----

	bool Node::IsNull() const {
		return std::holds_alternative<std::nullptr_t>(*this);
	}

	bool Node::IsBool() const {
		return std::holds_alternative<bool>(*this);
	}

	bool Node::IsInt() const {
		return std::holds_alternative<int>(*this);
	}

	bool Node::IsDouble() const {
		return IsPureDouble() || IsInt();
	}

	bool Node::IsPureDouble() const {
		return std::holds_alternative<double>(*this);
	}

	bool Node::IsString() const {
		return std::holds_alternative<std::string>(*this);
	}

	bool Node::IsArray() const {
		return std::holds_alternative<json::Array>(*this);
	}

	bool Node::IsMap() const {
		return std::holds_alternative<json::Dict>(*this);
	}

	bool Node::AsBool() const {
		if (!IsBool()) {
			throw(std::logic_error("Invalid Bool"));
		}
		return get<bool>(*this);
	}

	int Node::AsInt() const {
		if (!IsInt()) {
			throw(std::logic_error("Invalid Int"));
		}
		return get<int>(*this);
	}

	double Node::AsDouble() const {
		if (!IsDouble()) {
			throw(std::logic_error("Invalid Double"));
		}
		return IsPureDouble() ? get<double>(*this) : static_cast<double>(AsInt());
	}

	const string& Node::AsString() const {
		if (!IsString()) {
			throw(std::logic_error("Invalid String"));
		}
		return get<std::string>(*this);
	}

	const Array& Node::AsArray() const {
		return const_cast<Node*>(this)->AsArray();
	}

	const Dict& Node::AsMap() const {
		return const_cast<Node*>(this)->AsMap();
	}

	Array& Node::AsArray() {
		if (!IsArray()) {
			throw(std::logic_error("Invalid Array"));
		}
		return get<Array>(*this);
	}

	Dict& Node::AsMap() {
		if (!IsMap()) {
			throw(std::logic_error("Invalid Map"));
		}
		return get<Dict>(*this);
	}


	bool operator==(const Node& lhs, const Node& rhs) {
		if (lhs.IsNull() && rhs.IsNull()) {
			return true;
		}

		if (lhs.IsBool() && rhs.IsBool()) {
			return lhs.AsBool() == rhs.AsBool();
		}

		if (lhs.IsInt() && rhs.IsInt()) {
			return lhs.AsInt() == rhs.AsInt();
		}

		if (lhs.IsPureDouble() && rhs.IsPureDouble()) {
			return lhs.AsDouble() == rhs.AsDouble();
		}

		if (lhs.IsString() && rhs.IsString()) {
			return lhs.AsString() == rhs.AsString();
		}

		if (lhs.IsArray() && rhs.IsArray()) {
			return lhs.AsArray() == rhs.AsArray();
		}

		if (lhs.IsMap() && rhs.IsMap()) {
			return lhs.AsMap() == rhs.AsMap();
		}

		return false;
	}

	Node LoadNode(istream& input) {
		std::string str;
		while (input.good()) {
			std::string temp;
			std::getline(input, temp);
			str += temp;
		}
		return detail::LoadNode(str);
	}

	//-----Document-----

	Document::Document(Node root)
		: root_(std::move(root)) {
	}

	const Node& Document::GetRoot() const {
		return root_;
	}

	Document Load(istream& input) {
		return Document{ LoadNode(input) };
	}

	bool operator==(const Document& lhs, const Document& rhs) {
		return lhs.GetRoot() == rhs.GetRoot();
	}

	void Print(const Document& doc, std::ostream& output) {
		PrintNode(doc.GetRoot(), output);
	}

	void PrintNode(const Node& node, std::ostream& output, int level) {
		if (node.IsNull()) {
			output << "null";
			return;
		}

		if (node.IsBool()) {
			output << (node.AsBool() ? "true" : "false");
			return;
		}

		if (node.IsInt()) {
			output << node.AsInt();
			return;
		}

		if (node.IsPureDouble()) {
			output << node.AsDouble();
			return;
		}

		if (node.IsString()) {
			output << detail::StringSerialization(node.AsString());
			return;
		}

		if (node.IsArray()) {
			detail::PrintArray(node, output, level);
		}

		if (node.IsMap()) {
			detail::PrintMap(node, output, level);
		}
	}
}  // namespace json