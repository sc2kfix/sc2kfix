// sc2kfix include/commandtree.hpp: variant of json.hpp for the console command tree
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#include <cstdint>
#include <cmath>
#include <cctype>
#include <string>
#include <deque>
#include <map>
#include <type_traits>
#include <initializer_list>
#include <ostream>
#include <iostream>

#include <sc2kfix.h>

// Variant of json::JSON specifically for console command trees
std::string string_format(const char* fmt, ...);

namespace console {
	using std::map;
	using std::string;
	using std::enable_if;
	using std::initializer_list;
	using std::is_same;
	using std::is_convertible;
	using std::is_integral;
	using std::is_floating_point;
	using std::is_pointer;

	class CommandTree {
		union BackingData {
			BackingData(ConsoleCommand p) : Command(new ConsoleCommand(p)) {}
			BackingData() : Command() {}

			map<string, CommandTree>* Map;
			ConsoleCommand* Command;
		} Internal;

	public:
		enum class Class {
			Null,
			Object,
			Command
		};

		template<typename Container> class JSONWrapper {
			Container* object;

		public:
			JSONWrapper(Container* val) : object(val) {}
			JSONWrapper(std::nullptr_t) : object(nullptr) {}

			typename Container::iterator begin() { return object ? object->begin() : typename Container::iterator(); }
			typename Container::iterator end() { return object ? object->end() : typename Container::iterator(); }
			typename Container::const_iterator begin() const { return object ? object->begin() : typename Container::iterator(); }
			typename Container::const_iterator end() const { return object ? object->end() : typename Container::iterator(); }
		};

		template <typename Container> class JSONConstWrapper {
			const Container* object;

		public:
			JSONConstWrapper(const Container* val) : object(val) {}
			JSONConstWrapper(std::nullptr_t) : object(nullptr) {}

			typename Container::const_iterator begin() const { return object ? object->begin() : typename Container::const_iterator(); }
			typename Container::const_iterator end() const { return object ? object->end() : typename Container::const_iterator(); }
		};

		CommandTree() : Internal(), Type(Class::Null) {}

		CommandTree(CommandTree&& other) noexcept : Internal(other.Internal), Type(other.Type) {
			other.Type = Class::Null;
			other.Internal.Map = nullptr;
		}

		CommandTree& operator=(CommandTree&& other) noexcept {
			ClearInternal();
			Internal = other.Internal;
			Type = other.Type;
			other.Internal.Map = nullptr;
			other.Type = Class::Null;
			return *this;
		}

		CommandTree(const CommandTree& other) {
			switch (other.Type) {
			case Class::Object:
				Internal.Map = new map<string, CommandTree>(other.Internal.Map->begin(), other.Internal.Map->end());
				break;
			case Class::Command:
				Internal.Command = new ConsoleCommand(*other.Internal.Command);
				break;
			default:
				Internal = other.Internal;
			}
			Type = other.Type;
		}

		CommandTree& operator=(const CommandTree& other) {
			ClearInternal();
			switch (other.Type) {
			case Class::Object:
				Internal.Map = new map<string, CommandTree>(other.Internal.Map->begin(), other.Internal.Map->end());
				break;
			case Class::Command:
				Internal.Command = new ConsoleCommand(*other.Internal.Command);
				break;
			default:
				Internal = other.Internal;
			}
			Type = other.Type;
			return *this;
		}

		~CommandTree() {
			switch (Type) {
			case Class::Object:
				delete Internal.Map;
				break;
			case Class::Command:
				delete Internal.Command;
				break;
			default:;
			}
		}

		CommandTree(ConsoleCommand) : Internal(), Type(Class::Command) {}

		CommandTree(std::nullptr_t) : Internal(), Type(Class::Null) {}

		static CommandTree Make(Class type) {
			CommandTree ret; ret.SetType(type);
			return ret;
		}

		CommandTree& operator=(ConsoleCommand c) {
			SetType(Class::Command); *Internal.Command = ConsoleCommand(c);
			return *this;
		}

		CommandTree& operator[](const string& key) {
			SetType(Class::Object); return Internal.Map->operator[](key);
		}

		CommandTree& at(const string& key) {
			return operator[](key);
		}

		const CommandTree& at(const string& key) const {
			return Internal.Map->at(key);
		}

		bool hasKey(const string& key) const {
			if (Type == Class::Object)
				return Internal.Map->find(key) != Internal.Map->end();
			return false;
		}

		int size() const {
			if (Type == Class::Object)
				return Internal.Map->size();
			return -1;
		}

		Class ObjectType() const { return Type; }

		/// Functions for getting primitives from the CommandTree object.
		bool IsNull() const { return Type == Class::Null; }

		ConsoleCommand ToCommand() const {
			bool b;
			return std::move(ToCommand(b));
		}

		ConsoleCommand ToCommand(bool& ok) const {
			ok = (Type == Class::Command);
			return ok ? *Internal.Command : ConsoleCommand();
		}

		JSONWrapper<map<string, CommandTree>> ObjectRange() {
			if (Type == Class::Object)
				return JSONWrapper<map<string, CommandTree>>(Internal.Map);
			return JSONWrapper<map<string, CommandTree>>(nullptr);
		}

		JSONConstWrapper<map<string, CommandTree>> ObjectRange() const {
			if (Type == Class::Object)
				return JSONConstWrapper<map<string, CommandTree>>(Internal.Map);
			return JSONConstWrapper<map<string, CommandTree>>(nullptr);
		}

		void merge(CommandTree jsonSource) {
			for (auto& p : *jsonSource.Internal.Map) {
				switch (p.second.Type) {
				case Class::Object:
					if (Internal.Map->operator[](p.first).IsNull())
						Internal.Map->operator[](p.first) = p.second;
					else
						Internal.Map->operator[](p.first).merge(p.second);
					break;
				default:
					Internal.Map->operator[](p.first) = p.second;
					break;
				}
			}
		}

		string dump(int depth = 1, string tab = "  ") const {
			string pad = "";
			for (int i = 0; i < depth; ++i, pad += tab);

			switch (Type) {
			case Class::Null:
				return "null";
			case Class::Object: {
				string s = "{\n";
				bool skip = true;
				for (auto& p : *Internal.Map) {
					if (!skip) s += ",\n";
					s += (pad + "\"" + p.first + "\" : " + p.second.dump(depth + 1, tab));
					skip = false;
				}
				s += ("\n" + pad.erase(0, 2) + "}");
				return s;
			}
			case Class::Command:
				return string_format("{ %d, 0x%08X }", Internal.Command->iType, Internal.Command->pCommand);
			default:
				return "";
			}
			return "";
		}

	private:
		void SetType(Class type) {
			if (type == Type)
				return;

			ClearInternal();

			switch (type) {
			case Class::Null:
				Internal.Map = nullptr;
				break;
			case Class::Object:
				Internal.Map = new map<string, CommandTree>();
				break;
			case Class::Command:
				Internal.Command = new ConsoleCommand();
				break;
			}

			Type = type;
		}

	private:
		/* beware: only call if YOU know that Internal is allocated. No checks performed here.
		   This function should be called in a constructed CommandTree just before you are going to
		  overwrite Internal...
		*/
		void ClearInternal() {
			switch (Type) {
			case Class::Object:
				delete Internal.Map;
				break;
			case Class::Command:
				delete Internal.Command;
				break;
			default:;
			}
		}

	private:
		Class Type = Class::Null;
	};

	CommandTree Object();
}