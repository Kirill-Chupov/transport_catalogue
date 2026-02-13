#pragma once

#include <stack>
#include <stdexcept>
#include <memory>
#include <optional>
#include <string>

#include "json.h"

namespace json {
	class ErrorBuilding : public std::logic_error {
	public:
		using logic_error::logic_error;
	};

	class Builder {
	public:
		class DictContext;
		class ArrayContext;
		class KeyContext;

		Builder();
		DictContext StartDict();
		ArrayContext StartArray();
		KeyContext Key(std::string key);
		Builder& Value(Node value);
		Builder& EndDict();
		Builder& EndArray();
		Node Build();

	private:
		friend class DictContext;
		friend class ArrayContext;
		friend class KeyContext;

		std::stack<std::optional<std::string>> last_key_;
		std::stack<std::unique_ptr<Node>> nodes_stack_;
	};

	class Builder::DictContext {
	public:
		DictContext(Builder& builder);

		KeyContext Key(std::string key);
		Builder& EndDict();

	private:
		Builder& builder_;
	};

	class Builder::ArrayContext {
	public:
		ArrayContext(Builder& builder);

		ArrayContext& Value(Node node);
		DictContext StartDict();
		ArrayContext StartArray();
		Builder& EndArray();

	private:
		Builder& builder_;
	};

	class Builder::KeyContext {
	public:
		KeyContext(Builder& builder);

		DictContext Value(Node node);
		DictContext StartDict();
		ArrayContext StartArray();

	private:
		Builder& builder_;
	};
}