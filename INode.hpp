#pragma once
#include<Siv3D.hpp>
#include"Config.hpp"
#include"Input.hpp"
#include"3rdparty/nameof.hpp"

namespace NodeEditor
{
	class ISocket;
}
#include"NodeSocket.hpp"

namespace NodeEditor
{
	class INode : public ISerializable
	{
	private:

		SizeF m_size;

		bool m_isGrab = false;

		Stopwatch m_backColStw = Stopwatch(Duration(1s), true);

		double m_backHue = 0;

		//Serialize
		Array<std::shared_ptr<ValueSocket>> m_inputSockets;

		//Serialize
		Array<std::shared_ptr<ValueSocket>> m_outputSockets;

		//Serialize
		Array<std::shared_ptr<ExecSocket>> m_prevNodeSockets;

		//Serialize
		Array<std::shared_ptr<ExecSocket>> m_nextNodeSockets;

		RectF m_rect;

		RectF m_titleRect;

		RectF m_contentRect;

		RectF m_socketRect;

		RectF m_childRect;

		Optional<String> m_errorMsg;

		void calcSize(const Config& cfg);

		void calcRect(const Config& cfg);

		void drawBackground(const Config& cfg);

		void setBackCol(const double hue);

	protected:

		SizeF ChildSize = SizeF(0, 0);

		size_t NextExecIdx = 0;

		virtual void childRun() {};

		virtual void childUpdate(const Config&, Input&) {};

		virtual void childDraw(const Config&) {};

		virtual void childSerialize(JSONWriter& writer) const
		{
			writer.startObject();
			writer.endObject();
		};

		virtual void childDeserialize(const JSONValue&) {};

		template<class T>
		void setOutput(const size_t index, const T& input)
		{
			m_outputSockets[index]->setValue(std::any(input));
		}

		template<class T>
		T getInput(const size_t index) const
		{
			return std::any_cast<T>(m_inputSockets[index]->value());
		}

		void cfgInputSockets(Array<std::pair<Type, String>> cfg);

		void cfgOutputSockets(Array<std::pair<Type, String>> cfg);

		void cfgPrevExecSocket(const Array<String>& names);

		void cfgNextExecSocket(const Array<String>& names);

	public:

		//Serialize
		Vec2 Location = Vec2(0, 0);

		String Name = U"";

		//Serialize
		String Class = U"";

		//Serialize
		size_t ID = 0;

		bool Selecting = false;

		INode()
		{

		}

		void run();

		void update(const Config& cfg, Input& input);

		void draw(const Config& cfg);

		template<class T>
		void setInput(const size_t idx, const T& input)
		{
			auto& inSocket = m_inputSockets[idx];
			if (inSocket->ValueType != Type::getType<T>())
			{
				throw Error(U"入力できない型の値を入力しました");
			}
			inSocket->value(std::any(input));
		}

		template<class T>
		T getOutput(const size_t idx)
		{
			auto& outSocket = m_outputSockets[idx];
			if (outSocket->ValueType == Type::getType<void>())
			{
				throw Error(U"このノードの出力はありません");
			}
			if (!outSocket->value().has_value())
			{
				run();
			}
			return std::any_cast<T>(outSocket->value());
		}

		RectF getRect() const
		{
			return RectF(Location, m_size);
		}

		~INode();

		//ソケット取得

		const Array<std::shared_ptr<ValueSocket>> getInputSockets() const
		{
			return m_inputSockets;
		}

		const Array<std::shared_ptr<ValueSocket>> getOutputSockets() const
		{
			return m_outputSockets;
		}

		const Array<std::shared_ptr<ExecSocket>> getPrevNodeSockets() const
		{
			return m_prevNodeSockets;
		}

		const Array<std::shared_ptr<ExecSocket>> getNextNodeSockets() const
		{
			return m_nextNodeSockets;
		}

		const Array<std::shared_ptr<ISocket>> getAllInputSockets() const
		{
			Array<std::shared_ptr<ISocket>> result(m_inputSockets.size() + m_prevNodeSockets.size());
			size_t idx = 0;
			for (const auto& socket : m_inputSockets)
			{
				result[idx++] = socket;
			}
			for (const auto& socket : m_prevNodeSockets)
			{
				result[idx++] = socket;
			}
			return result;
		}

		const Array<std::shared_ptr<ISocket>> getAllOutputSockets() const
		{
			Array<std::shared_ptr<ISocket>> result(m_outputSockets.size() + m_nextNodeSockets.size());
			size_t idx = 0;
			for (const auto& socket : m_outputSockets)
			{
				result[idx++] = socket;
			}
			for (const auto& socket : m_nextNodeSockets)
			{
				result[idx++] = socket;
			}
			return result;
		}

		const Array<std::shared_ptr<ValueSocket>> getValueSockets() const
		{
			Array<std::shared_ptr<ValueSocket>> result;
			result.append(m_inputSockets);
			result.append(m_outputSockets);
			return result;
		}

		const Array<std::shared_ptr<ExecSocket>> getExecSockets() const
		{
			Array<std::shared_ptr<ExecSocket>> result;
			result.append(m_prevNodeSockets);
			result.append(m_nextNodeSockets);
			return result;
		}

		const Array<std::shared_ptr<ISocket>> getSockets() const
		{
			Array<std::shared_ptr<ISocket>> result(m_inputSockets.size() + m_prevNodeSockets.size() + m_outputSockets.size() + m_nextNodeSockets.size());
			size_t idx = 0;
			for (const auto& socket : m_inputSockets)
			{
				result[idx++] = socket;
			}
			for (const auto& socket : m_prevNodeSockets)
			{
				result[idx++] = socket;
			}
			for (const auto& socket : m_outputSockets)
			{
				result[idx++] = socket;
			}
			for (const auto& socket : m_nextNodeSockets)
			{
				result[idx++] = socket;
			}
			return result;
		}

		//Jsonシリアライズ/デシリアライズ

		void serialize(JSONWriter&) const override;

		void deserialize(const JSONValue&) override;

		void deserializeSockets(const JSONValue&, Array<std::shared_ptr<INode>>&);
	};

	namespace detail
	{
		template<class Type>
		class FunctionNode;

		//戻り値ありの関数
		template<class Result, class... Args>
		class FunctionNode<Result(Args...)> : public NodeEditor::INode
		{
		private:

			std::function<Result(Args...)> m_function;

			void childRun() override
			{
				size_t argIdx = sizeof...(Args) - 1;
				setOutput<Result>(0, m_function(getInput<Args>(argIdx--)...));
			}

		public:

			FunctionNode(const String& name, const Array<String>& socketNames, std::function<Result(Args...)> function)
			{
				if (socketNames.size() != (sizeof...(Args) + 1))
				{
					throw Error(U"");
				}

				m_function = function;

				Name = name;

				size_t argIdx = 0;
				cfgOutputSockets({ {Type::getType<Result>(),socketNames[argIdx++]} });
				cfgInputSockets({ {Type::getType<Args>(),socketNames[argIdx++]}... });
				cfgPrevExecSocket({ U"" });
				cfgNextExecSocket({ U"" });
			}
		};

		//戻り値なしの関数
		template<class... Args>
		class FunctionNode<void(Args...)> : public NodeEditor::INode
		{
		private:

			std::function<void(Args...)> m_function;

			void childRun() override
			{
				size_t argIdx = sizeof...(Args) - 1;
				m_function(getInput<Args>(argIdx--)...);
			}

		public:

			FunctionNode(const String& name, const Array<String>& socketNames, std::function<void(Args...)> function)
			{
				if (socketNames.size() != sizeof...(Args))
				{
					throw Error(U"");
				}

				m_function = function;

				Name = name;

				size_t argIdx = 0;
				cfgInputSockets({ {Type::getType<Args>(),socketNames[argIdx++]}... });
				cfgPrevExecSocket({ U"" });
				cfgNextExecSocket({ U"" });
			}
		};
	}
}
