#include "INode.hpp"

Triangle EquilateralTriangle(const Vec2& center, const double& r, const double& theta)
{
	return Triangle(Circular(r, theta), Circular(r, theta + 120_deg), Circular(r, theta + 240_deg))
		.setCentroid(center);
}

void NodeEditor::INode::calcSize(const Config& cfg)
{
	m_size.y = Max(m_inputSockets.size() + m_prevNodeSockets.size(), m_outputSockets.size() + m_nextNodeSockets.size()) * cfg.font.height() + cfg.TitleHeight + cfg.RectR + ChildSize.y;
	float inWidthMax = 0, outWidthMax = 0;
	for (auto& prevSocket : m_prevNodeSockets)
	{
		const float width = static_cast<float>(cfg.font(prevSocket->Name).region().w);
		if (width > inWidthMax)
		{
			inWidthMax = width;
		}
	}
	for (auto& inSocket : m_inputSockets)
	{
		const auto tex = cfg.getTypeIcon(inSocket->ValueType);
		const float width = static_cast<float>(cfg.font(inSocket->Name).region().w + (tex ? tex->width() : 0));
		if (width > inWidthMax)
		{
			inWidthMax = width;
		}
	}
	for (auto& nextSocket : m_nextNodeSockets)
	{
		const float width = static_cast<float>(cfg.font(nextSocket->Name).region().w);
		if (width > outWidthMax)
		{	
			outWidthMax = width;
		}
	}
	for (auto& outSocket : m_outputSockets)
	{
		const auto tex = cfg.getTypeIcon(outSocket->ValueType);
		const float width = static_cast<float>(cfg.font(outSocket->Name).region().w + (tex ? tex->width() : 0));
		if (width > outWidthMax)
		{
			outWidthMax = width;
		}
	}
	m_size.x = Max<float>({ cfg.WidthMin, inWidthMax + cfg.IOMargin + outWidthMax, (float)ChildSize.x });
}

void NodeEditor::INode::calcRect(const Config& cfg)
{
	m_rect = RectF(Location, m_size);
	{
		m_titleRect = RectF(Location, m_size.x, cfg.TitleHeight);
		m_contentRect = RectF(m_rect.x, m_rect.y + cfg.TitleHeight, m_rect.w, m_rect.h - cfg.TitleHeight - cfg.RectR);
		{
			m_socketRect = RectF(m_contentRect.pos, m_contentRect.w, Max(m_inputSockets.size() + m_prevNodeSockets.size(), m_outputSockets.size() + m_nextNodeSockets.size()) * cfg.font.height());
			m_childRect = RectF(m_socketRect.bl() + Vec2(0, 1), m_contentRect.w, ChildSize.y);
		}
	}
}

void NodeEditor::INode::drawBackground(const Config& cfg)
{
	double s = 0;

	const auto& roundRect = m_rect.rounded(cfg.RectR);

	if (m_backColStw.elapsed() < 1s)
	{
		s = (1 - m_backColStw.elapsed() / 1s) * 0.8;
	}

	//四角の描画
	roundRect.draw(HSV(m_backHue, s, 0.7));
	m_contentRect.draw(ColorF(0.9));

	//タイトルの描画
	cfg.font(Name).drawAt(m_titleRect.center(), Palette::Black);
}

void NodeEditor::INode::setBackCol(const double hue)
{
	m_backColStw.restart();
	m_backHue = hue;
}

void NodeEditor::INode::cfgInputSockets(Array<std::pair<Type, String>> cfg)
{
	m_inputSockets = Array<std::shared_ptr<ValueSocket>>(cfg.size());
	for (size_t i = 0; i < cfg.size(); i++)
	{
		m_inputSockets[i] = std::make_shared<ValueSocket>(*this, cfg[i].second, IOType::Input, i, cfg[i].first);
	}
}

void NodeEditor::INode::cfgOutputSockets(Array<std::pair<Type, String>> cfg)
{
	m_outputSockets = Array<std::shared_ptr<ValueSocket>>(cfg.size());
	for (size_t i = 0; i < cfg.size(); i++)
	{
		m_outputSockets[i] = std::make_shared<ValueSocket>(*this, cfg[i].second, IOType::Output, i, cfg[i].first);
	}
}

void NodeEditor::INode::cfgPrevExecSocket(const Array<String>& names)
{
	m_prevNodeSockets = Array<std::shared_ptr<ExecSocket>>(names.size());
	for (size_t i = 0; i < names.size(); i++)
	{
		m_prevNodeSockets[i] = std::make_shared<ExecSocket>(*this, names[i], IOType::Input, i);
	}
}

void NodeEditor::INode::cfgNextExecSocket(const Array<String>& names)
{
	m_nextNodeSockets = Array<std::shared_ptr<ExecSocket>>(names.size());
	for (size_t i = 0; i < names.size(); i++)
	{
		m_nextNodeSockets[i] = std::make_shared<ExecSocket>(*this, names[i], IOType::Output, i);
	}
}

void NodeEditor::INode::run()
{
	m_errorMsg.reset();
	try
	{
		for (auto& inSocket : m_inputSockets)
		{
			if (!inSocket->ConnectedSocket)
			{
				throw Error(U"ノード名:\"{}\", \"入力ソケット:\"{}\"のノードが指定されていません"_fmt(Name, inSocket->Name));
			}
			inSocket->ConnectedSocket[0]->Node.run();
			inSocket->setValue(dynamic_pointer_cast<ValueSocket>(inSocket->ConnectedSocket[0])->value());
		}

		setBackCol(220);
		childRun();

		for (auto& outSocket : m_outputSockets)
		{
			if (!outSocket->value().has_value())
			{
				throw Error(U"ノード名:\"{}\", \"出力ソケット:\"{}\"の出力が指定されていません"_fmt(Name, outSocket->Name));
			}
		}

		//次のノードを実行
		if (m_nextNodeSockets)
		{
			for (auto& nextSocket : m_nextNodeSockets[NextExecIdx]->ConnectedSocket)
			{
				nextSocket->Node.run();
			}
		}
	}
	catch (Error& ex)
	{
		setBackCol(20);
		m_errorMsg = ex.what();
	}
}

void NodeEditor::INode::update(const Config& cfg, Input& input)
{
	calcSize(cfg);

	if (m_isGrab)
	{
		if (MouseL.pressed())
		{
			Location += Cursor::DeltaF();
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		else
		{
			m_isGrab = false;
		}
	}

	calcRect(cfg);

	if (input.mouseOver(m_titleRect))
	{
		Cursor::RequestStyle(CursorStyle::Hand);
		if (input.leftClicked(m_titleRect))
		{
			m_isGrab = true;
		}
	}

	if (ChildSize != SizeF(0, 0))
	{
		const Transformer2D transform(Mat3x2::Translate(m_childRect.pos), true);
		childUpdate(cfg, input);
	}
}

void NodeEditor::INode::draw(const Config& cfg)
{
	drawBackground(cfg);

	//エラーメッセージの吹き出し描画
	if (m_errorMsg)
	{
		const auto bottomCenter = m_rect.topCenter();
		const auto text = cfg.font(m_errorMsg.value());
		const auto topCenter = bottomCenter - Vec2(0, 10);
		const RectF balloonRect(Arg::bottomCenter = topCenter, text.region().size + SizeF(20, 0));

		balloonRect.draw(Palette::White);
		Triangle(topCenter - Vec2(5, 0), topCenter + Vec2(5, 0), bottomCenter).draw(Palette::White);
		text.drawAt(balloonRect.center(), Palette::Black);
	}

	//ノードの描画
	Vec2 fontBasePos = m_contentRect.tl();

	for (auto& prevSocket : m_prevNodeSockets)
	{
		auto pos = prevSocket->calcPos(cfg);
		auto triangle = EquilateralTriangle(pos, cfg.ConnectorSize / 2, 90_deg);

		cfg.font(prevSocket->Name).draw(Arg::topLeft = fontBasePos, Palette::Black);
		fontBasePos.y += cfg.font.height();

		if (prevSocket->ConnectedSocket)
		{
			triangle.draw(Palette::White);
		}
		else
		{
			triangle.drawFrame(1, Palette::White);
		}
	}

	for (int i = 0; i < m_inputSockets.size(); i++)
	{
		auto& inSocket = m_inputSockets[i];
		const auto tex = cfg.getTypeIcon(inSocket->ValueType);
		const Vec2 fontPos = fontBasePos + Vec2(0, cfg.font.height() * i) + Vec2(tex ? tex->width() : 0, 0);

		auto fontlc = cfg.font(inSocket->Name).draw(Arg::topLeft = fontPos, Palette::Black).leftCenter();
		if (tex)
		{
			tex->draw(Arg::rightCenter = fontlc);
		}

		auto pos = inSocket->calcPos(cfg);
		auto circle = Circle(pos, cfg.ConnectorSize / 2);
		if (inSocket->ConnectedSocket)
		{
			circle.draw(Palette::White);
		}
		else
		{
			circle.drawFrame(1, Palette::White);
		}
	}

	fontBasePos = m_contentRect.tr();

	for (auto& nextSocket : m_nextNodeSockets)
	{
		auto pos = nextSocket->calcPos(cfg);
		auto triangle = EquilateralTriangle(pos, cfg.ConnectorSize / 2, 90_deg);

		cfg.font(nextSocket->Name).draw(Arg::topRight = fontBasePos, Palette::Black);
		fontBasePos.y += cfg.font.height();

		if (nextSocket->ConnectedSocket)
		{
			triangle.draw(Palette::White);
		}
		else
		{
			triangle.drawFrame(1, Palette::White);
		}
	}

	for (int i = 0; i < m_outputSockets.size(); i++)
	{
		auto& outSocket = m_outputSockets[i];
		const auto tex = cfg.getTypeIcon(outSocket->ValueType);
		const Vec2 fontPos = fontBasePos + Vec2(0, cfg.font.height() * i) - Vec2(tex ? tex->width() : 0, 0);

		cfg.font(outSocket->Name).draw(Arg::topRight = fontPos, Palette::Black);
		if (tex)
		{
			tex->draw(Arg::leftCenter = Vec2(fontPos.x, fontPos.y + cfg.font.height() / 2));
		}

		auto circle = Circle(outSocket->calcPos(cfg), cfg.ConnectorSize / 2);
		if (outSocket->ConnectedSocket)
		{
			circle.draw(Palette::White);
		}
		else
		{
			circle.drawFrame(1, Palette::White);
		}
	}

	//子要素の描画
	if (ChildSize != SizeF(0, 0))
	{
		const Transformer2D transform(Mat3x2::Translate(m_childRect.pos), true);

		childDraw(cfg);
	}
}

NodeEditor::INode::~INode()
{
	for (auto& socket : m_inputSockets)
	{
		ISocket::disconnect(socket);
	}
	for (auto& socket : m_outputSockets)
	{
		ISocket::disconnect(socket);
	}
	for (auto& socket : m_prevNodeSockets)
	{
		ISocket::disconnect(socket);
	}
	for (auto& socket : m_nextNodeSockets)
	{
		ISocket::disconnect(socket);
	}
}

void NodeEditor::INode::serialize(JSONWriter& writer) const
{
	writer.startObject();
	{
		writer.key(U"id").write(ID);

		writer.key(U"location").write(Location);
		
		writer.key(U"inputSockets").startArray();
		{
			for (const auto& socket : m_inputSockets)
			{
				socket->serialize(writer);
			}
		}
		writer.endArray();

		writer.key(U"outputSockets").startArray();
		{
			for (const auto& socket : m_outputSockets)
			{
				socket->serialize(writer);
			}
		}
		writer.endArray();

		writer.key(U"prevNodeSockets").startArray();
		{
			for (const auto& socket : m_prevNodeSockets)
			{
				socket->serialize(writer);
			}
		}
		writer.endArray();

		writer.key(U"nextNodeSockets").startArray();
		{
			for (const auto& socket : m_nextNodeSockets)
			{
				socket->serialize(writer);
			}
		}
		writer.endArray();
	}
	writer.endObject();
}

void NodeEditor::INode::deserialize(const JSONValue& json)
{
}
