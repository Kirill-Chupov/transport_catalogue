#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

	class Node;
	// Псеводнимы основных сущностей JSON
	using Dict = std::map<std::string, Node>;
	using Array = std::vector<Node>;

	// Исключение при ошибках парсинга JSON
	class ParsingError : public std::runtime_error {
	public:
		using runtime_error::runtime_error;
	};

	class Node
		: private std::variant<std::nullptr_t, bool, int, double, std::string, Array, Dict> {
	public:
		using variant::variant;
		
		bool IsNull() const;
		bool IsBool() const;
		bool IsInt() const;
		bool IsDouble() const;
		bool IsPureDouble() const;
		bool IsString() const;
		bool IsArray() const;
		bool IsMap() const;

		bool AsBool() const;
		int AsInt() const;
		double AsDouble() const;
		const std::string& AsString() const;
		const Array& AsArray() const;
		const Dict& AsMap() const;

		Array& AsArray();
		Dict& AsMap();
	};

	bool operator==(const Node& lhs, const Node& rhs);

	class Document {
	public:
		explicit Document(Node root);

		const Node& GetRoot() const;

	private:
		Node root_;
	};

	Document Load(std::istream& input);

	bool operator==(const Document& lhs, const Document& rhs);

	void Print(const Document& doc, std::ostream& output);

	void PrintNode(const Node& node, std::ostream& output, int level = 0);

}  // namespace json