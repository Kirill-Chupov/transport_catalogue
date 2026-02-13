#include "json_builder.h"


namespace json {
	Builder::Builder() {
		nodes_stack_.emplace(std::make_unique<Node>());
	}

	Builder::DictContext Builder::StartDict() {
		nodes_stack_.emplace(std::make_unique<Node>(Dict()));
		last_key_.emplace();
		return DictContext(*this);
	}

	Builder& Builder::EndDict() {
		if (!nodes_stack_.top()->IsMap()) {
			throw(ErrorBuilding("Object is not Dict"));
		}
		auto node = std::move(nodes_stack_.top());
		nodes_stack_.pop();
		last_key_.pop();
		Value(std::move(*node));
		return *this;
	}

	Builder::ArrayContext Builder::StartArray() {
		nodes_stack_.emplace(std::make_unique<Node>(Array()));
		return ArrayContext(*this);
	}

	Builder& Builder::EndArray() {
		if (!nodes_stack_.top()->IsArray()) {
			throw(ErrorBuilding("Object is not Array"));
		}
		auto node = std::move(nodes_stack_.top());
		nodes_stack_.pop();
		Value(std::move(*node));
		return *this;
	}

	Builder::KeyContext Builder::Key(std::string key) {
		if (!nodes_stack_.top()->IsMap()) {
			throw(ErrorBuilding("Object is not Dict"));
		}

		if (last_key_.top()) {
			throw(ErrorBuilding("Key after Key"));
		}

		last_key_.top() = std::move(key);
		return *this;
	}

	Builder& Builder::Value(json::Node value) {
		std::unique_ptr<Node>& node = nodes_stack_.top();
		if (node->IsArray()) {
			node->AsArray().emplace_back(std::move(value));
			return *this;
		}

		if (node->IsMap()) {
			if (!last_key_.top()) {
				throw(ErrorBuilding("Key empty"));
			}
			node->AsMap().emplace(std::move(last_key_.top().value()), std::move(value));
			last_key_.top() = std::nullopt;
			return *this;
		}

		if (!node->IsNull()) {
			throw(ErrorBuilding("Value after Value"));
		}

		*node = std::move(value);
		return *this;
	}

	Node Builder::Build() {
		if (nodes_stack_.top()->IsNull()) {
			throw(ErrorBuilding("Build empty"));
		}

		if (nodes_stack_.size() > 1) { 
			throw(ErrorBuilding("Layer not closed")); 
		}

		return *nodes_stack_.top();
	}

	//-----DictContext-----

	Builder::DictContext::DictContext(Builder& builder)
		: builder_{ builder } {
	}

	Builder::KeyContext Builder::DictContext::Key(std::string key) {
		builder_.Key(std::move(key));
		return KeyContext(builder_);
	}

	Builder& Builder::DictContext::EndDict() {
		return builder_.EndDict();
	}

	//-----ArrayContext-----

	Builder::ArrayContext::ArrayContext(Builder& builder)
		:builder_{ builder } {
	}

	Builder::ArrayContext& Builder::ArrayContext::Value(Node node) {
		builder_.Value(std::move(node));
		return *this;
	}

	Builder::DictContext Builder::ArrayContext::StartDict() {
		return builder_.StartDict();
	}

	Builder::ArrayContext Builder::ArrayContext::StartArray() {
		return builder_.StartArray();
	}

	Builder& Builder::ArrayContext::EndArray() {
		return builder_.EndArray();
	}

	//-----KeyContext-----

	Builder::KeyContext::KeyContext(Builder& builder)
		: builder_{ builder } {
	}

	Builder::DictContext Builder::KeyContext::Value(Node node) {
		builder_.Value(std::move(node));
		return DictContext(builder_);
	}

	Builder::DictContext Builder::KeyContext::StartDict() {
		return builder_.StartDict();
	}

	Builder::ArrayContext Builder::KeyContext::StartArray() {
		return builder_.StartArray();
	}
}