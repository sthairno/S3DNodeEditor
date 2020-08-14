#include"INode.h"
#include"NodeSocket.h"

void NodeEditor::ISocket::disconnect(std::shared_ptr<ISocket> ptr)
{
	for (auto outSocket : ptr->ConnectedSocket)
	{
		outSocket->ConnectedSocket.remove(ptr);
	}
	ptr->ConnectedSocket.clear();
}

bool NodeEditor::ISocket::canConnect(const ISocket& to)
{
	return
		(SocketType != to.SocketType) &&
		(typeid(*this) == typeid(to)) &&
		canConnectSameType(to);
}

void NodeEditor::ISocket::connectIO(std::shared_ptr<ISocket> in, std::shared_ptr<ISocket> out)
{
	if (in->singleConnect)
	{
		disconnect(in);
	}
	if (out->singleConnect)
	{
		disconnect(out);
	}

	in->ConnectedSocket.push_back(out);
	out->ConnectedSocket.push_back(in);
}

void NodeEditor::ISocket::connect(std::shared_ptr<ISocket> from, std::shared_ptr<ISocket> to)
{
	if (from->SocketType == to->SocketType)
	{
		throw Error(U"接続元と接続先のSocketTypeが同じです");
	}
	switch (from->SocketType)
	{
	case IOType::Input:
		connectIO(from, to);
		break;
	case IOType::Output:
		connectIO(to, from);
		break;
	}
}

bool NodeEditor::ValueSocket::canConnectSameType(const ISocket& to)
{
	const auto& valSocket = dynamic_cast<const ValueSocket&>(to);
	return ValueType == valSocket.ValueType;
}

void NodeEditor::ValueSocket::setValue(std::any value)
{
	if (!value.has_value() || value.type() != ValueType.TypeInfo())
	{
		throw Error(U"指定された値が正しくありません");
	}
	m_value = value;
}

Vec2 NodeEditor::ValueSocket::calcPos(const Config& cfg)
{
	switch (SocketType)
	{
	case IOType::Input:
		return Node.getRect().tl() + Vec2(-cfg.ConnectorSize / 2, cfg.TitleHeight + cfg.font.height() * (Index + Node.getPrevNodeSockets().size() + 0.5f));
	case IOType::Output:
		return Node.getRect().tr() + Vec2(cfg.ConnectorSize / 2, cfg.TitleHeight + cfg.font.height() * (Index + Node.getNextNodeSockets().size() + 0.5f));
	}
	return Vec2(0, 0);
}

bool NodeEditor::ExecSocket::canConnectSameType(const ISocket&)
{
	return true;
}

Vec2 NodeEditor::ExecSocket::calcPos(const Config& cfg)
{
	switch (SocketType)
	{
	case IOType::Input:
		return Node.getRect().tl() + Vec2(-cfg.ConnectorSize / 2, cfg.TitleHeight + cfg.font.height() * (Index + 0.5f));
	case IOType::Output:
		return Node.getRect().tr() + Vec2(cfg.ConnectorSize / 2, cfg.TitleHeight + cfg.font.height() * (Index + 0.5f));
	}
	return Vec2(0, 0);
}
