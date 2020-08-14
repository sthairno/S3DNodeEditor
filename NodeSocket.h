#pragma once
#include<Siv3D.hpp>
#include"Config.h"

namespace NodeEditor
{
	class INode;

	class ISocket
	{
	private:

		static void connectIO(std::shared_ptr<ISocket> in, std::shared_ptr<ISocket> out);
		
	protected:

		bool singleConnect;
		
		virtual bool canConnectSameType(const ISocket& to) = 0;

	public:

		INode& Node;

		const String Name;

		const IOType SocketType;

		const size_t Index;

		Array<std::shared_ptr<ISocket>> ConnectedSocket;

		ISocket(INode& node, const String& desc, const IOType& socketType, const size_t index)
			:Node(node),
			Name(desc),
			SocketType(socketType),
			Index(index)
		{

		}

		virtual Vec2 calcPos(const Config& cfg) = 0;

		bool canConnect(const ISocket& to);

		static void disconnect(std::shared_ptr<ISocket> ptr);

		static void connect(std::shared_ptr<ISocket> ptr, std::shared_ptr<ISocket> to);
	};

	class ValueSocket : public ISocket
	{
	private:

		std::any m_value;

		bool canConnectSameType(const ISocket& to) override;

	public:

		const Type ValueType;

		ValueSocket(INode& node, const String& desc, const IOType& socketType, const size_t index, const Type valType)
			:ISocket(node, desc, socketType, index),
			ValueType(valType)
		{
			singleConnect = socketType == IOType::Input;
		}

		std::any value() const
		{
			return m_value;
		}

		void setValue(std::any value);

		Vec2 calcPos(const Config& cfg) override;
	};

	class ExecSocket : public ISocket
	{
	private:

		bool canConnectSameType(const ISocket& to) override;

	public:
		ExecSocket(INode& node, const String& desc, const IOType& socketType, const size_t index)
			:ISocket(node, desc, socketType, index)
		{
			singleConnect = false;
		}

		Vec2 calcPos(const Config& cfg) override;
	};
}
